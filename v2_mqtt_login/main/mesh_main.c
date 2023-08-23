#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

#include "config.h"
#include "sensor_simulator.h"
#include "mesh_event_handler.h"
#include "mqtt_event_handler.h"

// sizes of rx and tx buffers
// #define TX_SIZE		1460
#define RX_SIZE		1500
#define ERROR_MS	3 * 1000		// 3 seg
#define TX_DELAY_MS	1 * 60 * 1000 	// 1 min

// some of sdkconfig args for ease configuration
#define MESH_MAX_LAYER	6
#define MESH_TOPOLOGY	0
#define MESH_CHANNEL	0
#define ROUTER_SSID		"Windows-Matheus"
#define ROUTER_PASSWD	"password"
#define MQTT_PASSWD		"fatecmesh@"
#define MQTT_URI		"mqtt://20.226.34.95:1883"

// led related
#define LED_PIN		GPIO_NUM_2
#define LED_HIGH	1
#define LED_LOW		0

// function signatures
void node_task_start(void);
void mqtt_node_start(void);
void mqtt_receive_handler(char* topic, char* data);
void mqtt_publish(mesh_addr_t source, char* payload);
void mqtt_subscribe(mesh_addr_t source, char* topic);

// global variables
static fatec_mesh_config_t config = {
	.mesh_tag = "fatec_mesh",
	.mesh_id = {0x46, 0x61, 0x74, 0x65, 0x63, 0x4D},
	.mesh_layer = -1,
	.is_running = true,
	.is_mesh_connected = false,
	.is_mqtt_connected = false,
	.is_tods_reachable = false,
	.netif_sta = NULL,
	.node_task_start = node_task_start,
	.mqtt_receive_handler = mqtt_receive_handler,
	.mqtt_node_start = mqtt_node_start,
	.mqtt_subscribe_status = SUBSCRIBE_TOPIC_FAIL, // starts as fail
	.led_status = LED_LOW
};

// static uint8_t tx_buf[TX_SIZE] = { 0 };
static uint8_t rx_buf[RX_SIZE] = { 0 };

// mqtt client: configuration is done on start_mqtt_client()
esp_mqtt_client_handle_t mqtt_client;


//
// ----------- NODE -----------
//


// task: get sensor data and foward to root
void send_sensor_data(void *arg) {
	while (config.is_running) {
		if (!config.is_tods_reachable) {
			ESP_LOGE(config.mesh_tag, "> ToDS Unreachable");
			vTaskDelay(ERROR_MS / portTICK_PERIOD_MS);
			continue;
		}

		// sensor data
		char* sensor_values = create_sensor_data_string();

		// checking if payload do not exceed maximum size
		if (sizeof(sensor_values) > MESH_MPS) {
			ESP_LOGE(config.mesh_tag, "> Generated payload is too long: %d, max is %d", sizeof(sensor_values), MESH_MPS);
			vTaskDelay(ERROR_MS / portTICK_PERIOD_MS);
			continue;
		}

		// creating data structure
		mesh_data_t sensor_data = {
			.data = (uint8_t*) sensor_values,
			.size = strlen(sensor_values),
			.proto = MESH_PROTO_BIN,
			.tos = MESH_TOS_P2P
		};

		// send data to root
		ESP_LOGI(config.mesh_tag, "> Sending sensor data: %s", sensor_data.data);
		int err = esp_mesh_send(NULL, &sensor_data, MESH_DATA_TODS, NULL, 0);

		// checking for error		
		if (err) {
			ESP_LOGE(config.mesh_tag, "> Error sending sensor data: 0x%x", err);
			vTaskDelay(ERROR_MS / portTICK_PERIOD_MS);
			continue;
		} 
		
		// message sent, putting a delay to send again
		// ESP_LOGI(config.mesh_tag, "> Message sent");
		vTaskDelay(TX_DELAY_MS / portTICK_PERIOD_MS);
	}

	// deleting task after ending
	vTaskDelete(NULL);
}

// task: subscribe to mqtt topic
void subscribe_to_command(void *arg) {
	int tries = 0;

	while (config.is_running && config.mqtt_subscribe_status != SUBSCRIBE_TOPIC_SUCCESS) {
		if (!config.is_tods_reachable) {
			vTaskDelay(ERROR_MS / portTICK_PERIOD_MS);
			continue;
		}

		// still pending, wait more
		if (config.mqtt_subscribe_status == SUBSCRIBE_TOPIC_PENDING) {
			// waited too much, try again
			if (tries >= 3) {
				tries = 0;
				config.mqtt_subscribe_status = SUBSCRIBE_TOPIC_FAIL;
			}

			vTaskDelay(ERROR_MS / portTICK_PERIOD_MS);
			continue;
		}

		char* topic;
		asprintf(&topic, "SUBSCRIBE_MQTT;%s", config.mqtt_subscribed_topic);

		// creating data structure
		mesh_data_t data = {
			.data = (uint8_t*) topic,
			.size = strlen(topic),
			.proto = MESH_PROTO_BIN,
			.tos = MESH_TOS_P2P
		};

		// send subscription request
		ESP_LOGI(config.mesh_tag, "> Sending subscribing request: %s", data.data);
		int err = esp_mesh_send(NULL, &data, MESH_DATA_TODS, NULL, 0);
		free(topic);

		// checking for error		
		if (err) {
			ESP_LOGE(config.mesh_tag, "> Error sending subscribing request: 0x%x", err);
			vTaskDelay(ERROR_MS / portTICK_PERIOD_MS);
			continue;
		} 
		
		// message sent, putting a delay to send again
		// ESP_LOGI(config.mesh_tag, "> Subscribe request sent");
		tries++;
		vTaskDelay(ERROR_MS / portTICK_PERIOD_MS);
	}

	// deleting task after ending
	vTaskDelete(NULL);
}

// update led status and send it to mqtt
void update_led_status(uint32_t status) {
	gpio_set_level(LED_PIN, status);
	config.led_status = status;
	ESP_LOGI(config.mesh_tag, "> Updated led status to %s", config.led_status == LED_HIGH ? "HIGH" : "LOW");

	char* led_status;
	asprintf(&led_status, "DATA_TO_MQTT;switch_info|LED status on the station|switch_status|%s",
			config.led_status == LED_HIGH ? "ON" : "OFF");

	// checking if payload do not exceed maximum size
	if (sizeof(led_status) > MESH_MPS) {
		ESP_LOGE(config.mesh_tag, "> Generated payload is too long: %d, max is %d", sizeof(led_status), MESH_MPS);
		return;
	}

	// creating data structure
	mesh_data_t led_data = {
		.data = (uint8_t*) led_status,
		.size = strlen(led_status),
		.proto = MESH_PROTO_BIN,
		.tos = MESH_TOS_P2P
	};

	// send data to root
	ESP_LOGI(config.mesh_tag, "> Sending switch status: %s", led_data.data);
	int err = esp_mesh_send(NULL, &led_data, MESH_DATA_TODS, NULL, 0);

	// checking for error		
	if (err) {
		ESP_LOGE(config.mesh_tag, "> Error sending switch status: 0x%x", err);
		return;
	} 
}

// handle a mqtt command
void handle_command(char* command) {
	char* cmd = strtok(command, "@");
	cmd = strtok(NULL, "@");
	char* type = strtok(cmd, "|");

	if (strncmp(type, "switch", strlen("switch")) == 0) {
		update_led_status(config.led_status == LED_HIGH ? LED_LOW : LED_HIGH);
		return;
	}

	ESP_LOGE(config.mesh_tag, "> Received unknow command from mqtt: %s", type);
}

// task: receive a message
void receive_message(void *arg) {
	esp_err_t err;
	mesh_addr_t source;
	mesh_data_t received_data;
	received_data.data = rx_buf;
	int flag = 0;

	while (config.is_running)
	{
		// reseting rx buffer
		memset(rx_buf, 0x00, sizeof(rx_buf));
		
		received_data.size = RX_SIZE;
		err = esp_mesh_recv(&source, &received_data, portMAX_DELAY, &flag, NULL, 0);

		// check if received anything
		if (err != ESP_OK || !received_data.size) {
			ESP_LOGE(config.mesh_tag, "> Error receiving a message: 0x%x, size:%d", err, received_data.size);
			continue;
		}

		// root only - publish to mqtt
		// only root can receive this type of flag, comming from bellow
		if (strncmp((char*) received_data.data, "DATA_TO_MQTT", strlen("DATA_TO_MQTT")) == 0) {
			char* data_to_mqtt = strtok((char*) received_data.data, ";");
			data_to_mqtt = strtok(NULL, ";");

			mqtt_publish(source, data_to_mqtt);
			continue;
		}

		// root only - subscribes to mqtt
		// only root can receive this type of flag, comming from bellow
		if (strncmp((char*) received_data.data, "SUBSCRIBE_MQTT", strlen("SUBSCRIBE_MQTT")) == 0) {
			char* topic = strtok((char*) received_data.data, ";");
			topic = strtok(NULL, ";");

			mqtt_subscribe(source, topic);
			continue;
		}

		// subscribe request was ok
		if (strncmp((char*) received_data.data, "SUBSCRIBE_OK", strlen("SUBSCRIBE_OK")) == 0) {
			config.mqtt_subscribe_status = SUBSCRIBE_TOPIC_SUCCESS;
			continue;
		}

		// subscribe request failed
		if (strncmp((char*) received_data.data, "SUBSCRIBE_FAIL", strlen("SUBSCRIBE_FAIL")) == 0) {
			config.mqtt_subscribe_status = SUBSCRIBE_TOPIC_FAIL;
			continue;
		}

		// reveives a message from mqtt
		// root fowarded the message to this node
		if (strncmp((char*) received_data.data, "DATA_FROM_MQTT", strlen("DATA_FROM_MQTT")) == 0) {
			// spliting the string
			char* mqtt_message = strtok((char*) received_data.data, ";"); // DATA_FROM_MQTT
			mqtt_message = strtok(NULL, ";"); // topic

			if (strcmp(config.mqtt_subscribed_topic, mqtt_message) == 0) {
				mqtt_message = strtok(NULL, ";"); // commmand
				handle_command(mqtt_message);
			}
			
			continue;
		}

		ESP_LOGI(config.mesh_tag, "> Received unhandled message from " MACSTR ": %s", MAC2STR(source.addr), received_data.data);
	}

	// deleting task after ending
	vTaskDelete(NULL);
}


//
// ----------- ROOT -----------
//

// starts the mqtt client
void start_mqtt_client(void) {
	if (config.is_mqtt_connected) {
		ESP_LOGW(config.mesh_tag, "> MQTT client already started");
		return;
	}
	
	esp_mqtt_client_config_t mqtt_cfg;

	// configuring mqtt credentials
	mqtt_cfg.credentials.username = config.mqtt_device_id;
	mqtt_cfg.broker.address.uri = MQTT_URI;
	mqtt_cfg.credentials.authentication.password = MQTT_PASSWD;

	mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
	ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, &config));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
}

// handle a mqtt command received from the server
void mqtt_receive_handler(char* topic, char* data) {
	// if message is for it, just handle the command
	if (strcmp(config.mqtt_subscribed_topic, topic) == 0) {
		handle_command(data);
		return;
	}

	// if not, broadcasts the message
	char* mqtt_message;
	asprintf(&mqtt_message, "DATA_FROM_MQTT;%s;%s", topic, data);

	// checking if payload do not exceed maximum size
	if (sizeof(mqtt_message) > MESH_MPS) {
		ESP_LOGE(config.mesh_tag, "> Generated payload is too long: %d, max is %d", sizeof(mqtt_message), MESH_MPS);
		return;
	}

	// creating data structure
	mesh_data_t send_data = {
		.data = (uint8_t*) mqtt_message,
		.size = strlen(mqtt_message),
		.proto = MESH_PROTO_BIN,
		.tos = MESH_TOS_P2P
	};

	mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
	int route_table_size = 0;

	esp_mesh_get_routing_table((mesh_addr_t*) &route_table, CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &route_table_size);

	// broadcasting data
	ESP_LOGI(config.mesh_tag, "> Broadcasting MQTT command: %s", mqtt_message);

	for (int i = 0; i < route_table_size; i++) {
		esp_err_t err = esp_mesh_send(&route_table[i], &send_data, MESH_DATA_FROMDS, NULL, 0);

		// checking for error		
		if (err) {
			ESP_LOGE(config.mesh_tag, "> Error broadcasting command: %s", esp_err_to_name(err));
			return;
		} 
	}
	
	free(mqtt_message);
}

// sends a mqtt message from a specific node below it
void mqtt_publish(mesh_addr_t source, char* payload) {
	if (!config.is_tods_reachable) {
		return;
	}

	if (!config.is_mqtt_connected) {
		ESP_LOGE(config.mesh_tag, "> MQTT not connected");
		return;
	}

	char* topic;
	asprintf(&topic, "/ul/fatecmeshiot/Station%02X%02X%02X/attrs", source.addr[3], source.addr[4], source.addr[5]);

	ESP_LOGI(config.mesh_tag, "> Sending MQTT message to %s: %s", topic, payload);

	// sending message
	int publish_err = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
	free(topic);

	if (publish_err < 0) {
		ESP_LOGE(config.mesh_tag, "> Error sending MQTT message");
		vTaskDelay(ERROR_MS / portTICK_PERIOD_MS);
		return;
	}

	ESP_LOGI(config.mesh_tag, "> Message sent");
}

// sends a mqtt subscribe request, returns ESP_OK if ok
void mqtt_subscribe(mesh_addr_t source, char* topic) {
	// creating data structure
	mesh_data_t data = {
		.proto = MESH_PROTO_BIN,
		.tos = MESH_TOS_P2P,
		.data = (uint8_t*) "SUBSCRIBE_FAIL",
		.size = strlen("SUBSCRIBE_FAIL")
	};

	if (!config.is_tods_reachable || !config.is_mqtt_connected) {
		esp_mesh_send(&source, &data, MESH_DATA_P2P, NULL, 0);
		return;
	}

	int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 0);

	if (msg_id == -1) {
		esp_mesh_send(&source, &data, MESH_DATA_P2P, NULL, 0);
		return;
	}

	data.data = (uint8_t*) "SUBSCRIBE_OK";
	data.size = strlen("SUBSCRIBE_OK");

	ESP_LOGI(config.mesh_tag, "> Subscribed to topic %s: msg_id=%d", topic, msg_id);
	esp_mesh_send(&source, &data, MESH_DATA_P2P, NULL, 0);
}

// task: message receiving to external network (root)
void comm_tods_rx(void *arg) {
	esp_err_t err;
	mesh_addr_t source;
	mesh_addr_t dest;
	mesh_data_t data;
	data.data = rx_buf;
	int flag = 0;
	config.is_running = true;

	while (config.is_running)
	{
		data.size = RX_SIZE;
		err = esp_mesh_recv_toDS(&source, &dest, &data, portMAX_DELAY, &flag, NULL, 0);

		// check if received anything
		if (err != ESP_OK || !data.size) {
			ESP_LOGE(config.mesh_tag, "> Error receiving a message: 0x%x, size:%d", err, data.size);
			continue;
		}

		struct sockaddr_in dest_addr;
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons((in_port_t) dest.mip.port);
		dest_addr.sin_addr.s_addr = (in_addr_t) dest.mip.ip4.addr;

		int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		int sock_err;

		if (sock < 0) {
			ESP_LOGE(config.mesh_tag, "> Error creating socket: %d", errno);
			close(sock);
			continue;
		}

		sock_err = connect(sock, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

		if (sock_err < 0) {
			ESP_LOGE(config.mesh_tag, "> Error connecting to %s:%d: %d, ",
					inet_ntoa(dest_addr.sin_addr), dest_addr.sin_port, errno);
			close(sock);
			continue;
		}

		sock_err = send(sock, data.data, data.size, 0);

		if (sock_err < 0) {
			ESP_LOGE(config.mesh_tag, "> Error sending message to %s:%d: %d",
					inet_ntoa(dest_addr.sin_addr), dest_addr.sin_port, errno);
			close(sock);
			continue;
		}
		
		ESP_LOGI(config.mesh_tag, "> Fowarded message from " MACSTR " to %s:%d: %s",
				MAC2STR(source.addr), inet_ntoa(dest_addr.sin_addr), dest_addr.sin_port, data.data);
		close(sock);
	}

	// deleting task after ending
	vTaskDelete(NULL);
}


//
// ----------- TASKS -----------
//


// starts the root communication with external network
// gets called by ip_event_handler()
void root_task_start(void) {
	static bool is_comm_tods_started = false;

	// starts mqtt client if not started yet
	if (!config.is_mqtt_connected) start_mqtt_client();

	if (!is_comm_tods_started) {
		ESP_LOGI(config.mesh_tag, "> Starting root tasks");
		xTaskCreate(comm_tods_rx, "COMM_TODS_RX", 3072, NULL, 5, NULL);
		is_comm_tods_started = true;
	}
}

// starts node sensor sending and command receiving
// gets called by MESH_EVENT_PARENT_CONNECTED event
void mqtt_node_start(void) {
	static bool is_mqtt_node_tasks_started = false;

	// need to subscribe again
	if (config.mqtt_subscribe_status == SUBSCRIBE_TOPIC_SUCCESS) {
		config.mqtt_subscribe_status = SUBSCRIBE_TOPIC_FAIL;
		xTaskCreate(subscribe_to_command, "COMMANDS", 2000, NULL, 4, NULL);
	}

	if (!is_mqtt_node_tasks_started) {
		ESP_LOGI(config.mesh_tag, "> Starting mqtt node tasks");

		xTaskCreate(send_sensor_data, "SENSORS", 2000, NULL, 4, NULL);
		xTaskCreate(subscribe_to_command, "COMMANDS", 2000, NULL, 4, NULL);

		is_mqtt_node_tasks_started = true;
	}
}

// starts the node communication
// gets called by start_mesh()
void node_task_start(void) {
	static bool is_node_tasks_started = false;

	if (!is_node_tasks_started) {
		ESP_LOGI(config.mesh_tag, "> Starting node tasks");
		xTaskCreate(receive_message, "COMM_RX", RX_SIZE * 2, NULL, 5, NULL);
		is_node_tasks_started = true;
	}
}


// ip event handler, handling only ip acquiring
void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

	ESP_LOGI(config.mesh_tag, "> Got IP: " IPSTR " (" IPSTR "), gw: " IPSTR,
			IP2STR(&event->ip_info.ip), IP2STR(&event->ip_info.netmask), IP2STR(&event->ip_info.gw));
	
	esp_mesh_post_toDS_state(true);
	root_task_start();
}


//
// ----------- MAIN -----------
//


// wifi related functions
void start_wifi(void) {
	ESP_ERROR_CHECK(nvs_flash_init()); // flash storage
	ESP_ERROR_CHECK(esp_netif_init()); // tcp/ip
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&config.netif_sta, NULL)); // network interfaces
	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&config)); // wifi configs init
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
	ESP_ERROR_CHECK(esp_wifi_start()); // wifi start
}

// mesh initialization
void start_mesh() {
	ESP_ERROR_CHECK(esp_mesh_init());
	ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, &config));
	ESP_ERROR_CHECK(esp_mesh_set_topology(MESH_TOPOLOGY));
	ESP_ERROR_CHECK(esp_mesh_set_max_layer(MESH_MAX_LAYER));
	ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
	ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));

	// power save functions
	#ifdef CONFIG_MESH_ENABLE_PS
		ESP_ERROR_CHECK(esp_mesh_enable_ps());
		ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(60));
		ESP_ERROR_CHECK(esp_mesh_set_announce_interval(600, 3300));
	#else
		ESP_ERROR_CHECK(esp_mesh_disable_ps());
		ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
	#endif

	// mesh configs
	mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();	// default
	memcpy((uint8_t *)&cfg.mesh_id, config.mesh_id, 6);	// mesh id
	cfg.channel = MESH_CHANNEL;						// channel

	// router ssid and passwd
	cfg.router.ssid_len = strlen(ROUTER_SSID);
	memcpy((uint8_t *)&cfg.router.ssid, ROUTER_SSID, cfg.router.ssid_len);
	memcpy((uint8_t *)&cfg.router.password, ROUTER_PASSWD, strlen(ROUTER_PASSWD));

	// soft ap
	ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
	cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
	cfg.mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
	memcpy((uint8_t *)&cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD, strlen(CONFIG_MESH_AP_PASSWD));

	// setting configs and starting mesh
	ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
	ESP_ERROR_CHECK(esp_mesh_start());

	// power save setting
	#ifdef CONFIG_MESH_ENABLE_PS
		ESP_ERROR_CHECK(esp_mesh_set_active_duty_cycle(CONFIG_MESH_PS_DEV_DUTY, CONFIG_MESH_PS_DEV_DUTY_TYPE));
		ESP_ERROR_CHECK(esp_mesh_set_network_duty_cycle(CONFIG_MESH_PS_NWK_DUTY, CONFIG_MESH_PS_NWK_DUTY_DURATION, CONFIG_MESH_PS_NWK_DUTY_RULE));
	#endif

	// gets self mac address
	ESP_ERROR_CHECK(esp_read_mac(config.mac_addr, ESP_MAC_WIFI_STA));

	// started
	ESP_LOGI(config.mesh_tag, "> Successfully started mesh, heap:%" PRId32 ", %s<%d>%s, ps:%d\n",
			esp_get_minimum_free_heap_size(),
			esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed",
			esp_mesh_get_topology(),
			esp_mesh_get_topology() ? "(chain)" : "(tree)",
			esp_mesh_is_ps_enabled());
	
	node_task_start();
}

// this is the main function
void app_main(void) {
	start_wifi();
	start_mesh();

	// setting led_pin as output
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

	// configuring device id and topic
	asprintf(&config.mqtt_device_id, "Station%02X%02X%02X", config.mac_addr[3], config.mac_addr[4], config.mac_addr[5]);
	asprintf(&config.mqtt_subscribed_topic, "/fatecmeshiot/%s/cmd", config.mqtt_device_id);
}