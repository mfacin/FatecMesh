#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
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
#define TX_DELAY_MS	2 * 1000 	// 2 sec

// some of sdkconfig args for ease configuration
#define MESH_MAX_LAYER	6
#define MESH_TOPOLOGY	0
#define MESH_CHANNEL	0
#define ROUTER_SSID		"Windows-Matheus"
#define ROUTER_PASSWD	"password"

esp_err_t mqtt_task_start(void);

static mesh_config_t CONFIG = {
	.MESH_TAG = "fatec_mesh",
	.MESH_ID = {0x46, 0x61, 0x74, 0x65, 0x63, 0x4D},
	.is_running = true,
	.is_mesh_connected = false,
	.is_mqtt_connected = false,
	.is_tods_reachable = false,
	.mesh_layer = -1,
	.netif_sta = NULL,
	.mqtt_task_start = mqtt_task_start
};

esp_mqtt_client_handle_t mqtt_client;
static esp_mqtt_client_config_t mqtt_cfg = {
	// .broker.address.uri = "mqtt://20.226.34.95:4041"
	.broker.address.uri = "mqtt://192.168.137.1:5000"
};

// static uint8_t tx_buf[TX_SIZE] = { 0 };
static uint8_t rx_buf[RX_SIZE] = { 0 };


// mqtt publishing transmission
void mqtt_tx(void *arg) {
	CONFIG.is_running = true;

	// message sending loop
	while (CONFIG.is_running) 	{
		if (!CONFIG.is_tods_reachable) {
			vTaskDelay(TX_DELAY_MS / portTICK_PERIOD_MS);
			continue;
		}

		if (!CONFIG.is_mqtt_connected) {
			ESP_LOGE(CONFIG.MESH_TAG, "> MQTT not connected");
			vTaskDelay(TX_DELAY_MS / portTICK_PERIOD_MS);
			continue;
		}

		char* values = create_message_string();
		char* attr = strtok(values, ";");

		while (attr != NULL) {
			char* topic;

			// /iot/station/{number}/attrs/{tag|valor}
			int topic_str_result = asprintf(&topic, "/iot/station/" MACSTR "/attrs/%s", MAC2STR(CONFIG.MAC), attr);

			if (topic_str_result < 0) {
				ESP_LOGE(CONFIG.MESH_TAG, "> Error creating topic string: %s", topic);
				vTaskDelay(TX_DELAY_MS / portTICK_PERIOD_MS);
				continue;
			}

			ESP_LOGI(CONFIG.MESH_TAG, "> Sending MQTT message to %s", topic);

			// MQTT publish here
			// não funciona :(
			int publish_err = esp_mqtt_client_publish(mqtt_client, topic, NULL, 0, 0, 0);

			if (publish_err < 0) {
				ESP_LOGE(CONFIG.MESH_TAG, "> Error sending MQTT message");
				vTaskDelay(TX_DELAY_MS / portTICK_PERIOD_MS);
				continue;
			}

			ESP_LOGI(CONFIG.MESH_TAG, "> Message sent");

			attr = strtok(NULL, ";");
			vTaskDelay(TX_DELAY_MS / portTICK_PERIOD_MS);
		}
	}

	// deleting task after ending
	vTaskDelete(NULL);
}

// message receiving to external network (root)
void comm_tods_rx(void *arg) {
	esp_err_t err;
	mesh_addr_t source;
	mesh_addr_t dest;
	mesh_data_t data;
	data.data = rx_buf;
	int flag = 0;
	CONFIG.is_running = true;

	while (CONFIG.is_running)
	{
		data.size = RX_SIZE;
		err = esp_mesh_recv_toDS(&source, &dest, &data, portMAX_DELAY, &flag, NULL, 0);

		// check if received anything
		if (err != ESP_OK || !data.size) {
			ESP_LOGE(CONFIG.MESH_TAG, "> Error receiving a message: 0x%x, size:%d", err, data.size);
			continue;
		}

		struct sockaddr_in dest_addr;
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons((in_port_t) dest.mip.port);
		dest_addr.sin_addr.s_addr = (in_addr_t) dest.mip.ip4.addr;

		int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		int sock_err;

		if (sock < 0) {
			ESP_LOGE(CONFIG.MESH_TAG, "> Error creating socket: %d", errno);
			close(sock);
			continue;
		}

		mesh_addr_t* dest_pointer = &dest;
		sock_err = connect(sock, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

		if (sock_err < 0) {
			ESP_LOGI(CONFIG.MESH_TAG, "%d", dest_addr.sin_port);
			ESP_LOGE(CONFIG.MESH_TAG, "> Error connecting to %s:%d: %d, ",
					inet_ntoa(dest_addr.sin_addr), dest_addr.sin_port, errno);
			close(sock);
			continue;
		}

		sock_err = send(sock, data.data, data.size, 0);

		if (sock_err < 0) {
			ESP_LOGE(CONFIG.MESH_TAG, "> Error sending message to %s:%d: %d",
					inet_ntoa(dest_addr.sin_addr), dest_addr.sin_port, errno);
			close(sock);
			continue;
		}
		
		ESP_LOGI(CONFIG.MESH_TAG, "> Fowarded message from " MACSTR " to %s:%d: %s",
				MAC2STR(source.addr), inet_ntoa(dest_addr.sin_addr), dest_addr.sin_port, data.data);
		close(sock);
	}

	// deleting task after ending
	vTaskDelete(NULL);
}

// starts the mqtt communication
esp_err_t mqtt_task_start(void) {
	static bool is_mqtt_started = false;

	if (!is_mqtt_started) {
		is_mqtt_started = true;
		xTaskCreate(mqtt_tx, "MQTT_TX", 3072, NULL, 5, NULL);
	}

	return ESP_OK;
}

// starts the root communication with external network
esp_err_t root_task_start(void) {
	static bool is_comm_tods_started = false;

	if (!is_comm_tods_started) {
		is_comm_tods_started = true;
		xTaskCreate(comm_tods_rx, "COMM_TODS_RX", 3072, NULL, 5, NULL);
	}

	return ESP_OK;
}

// ip event handler, handling only ip acquiring
void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

	ESP_LOGI(CONFIG.MESH_TAG, "> Got IP: " IPSTR " (" IPSTR "), gw: " IPSTR,
			IP2STR(&event->ip_info.ip), IP2STR(&event->ip_info.netmask), IP2STR(&event->ip_info.gw));
	
	esp_mesh_post_toDS_state(true);
	root_task_start();
}

// this is the main function
void app_main(void) {
	// wifi related functions
	ESP_ERROR_CHECK(nvs_flash_init()); // flash storage
	ESP_ERROR_CHECK(esp_netif_init()); // tcp/ip
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&CONFIG.netif_sta, NULL)); // network interfaces
	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&config)); // wifi configs init
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
	ESP_ERROR_CHECK(esp_wifi_start()); // wifi start

	// mesh initialization
	ESP_ERROR_CHECK(esp_mesh_init());
	ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, &CONFIG));
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
	memcpy((uint8_t *)&cfg.mesh_id, CONFIG.MESH_ID, 6);	// mesh id
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
	ESP_ERROR_CHECK(esp_read_mac(CONFIG.MAC, ESP_MAC_WIFI_STA));

	// started
	ESP_LOGI(CONFIG.MESH_TAG, "> Successfully started mesh, heap:%" PRId32 ", %s<%d>%s, ps:%d\n",
			esp_get_minimum_free_heap_size(),
			esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed",
			esp_mesh_get_topology(),
			esp_mesh_get_topology() ? "(chain)" : "(tree)",
			esp_mesh_is_ps_enabled());
	
	// mqtt starter
	mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, &CONFIG);
    esp_mqtt_client_start(mqtt_client);
}