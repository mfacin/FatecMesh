#ifndef CONFIG_HEADER
#define CONFIG_HEADER

#include <inttypes.h>
#include <stdbool.h>
#include "esp_mesh.h"

#define SUBSCRIBE_TOPIC_FAIL	-1
#define SUBSCRIBE_TOPIC_PENDING	0
#define SUBSCRIBE_TOPIC_SUCCESS 1

typedef struct {
	// mesh config
	char* mesh_tag;
	uint8_t mesh_id[6];
	int mesh_layer;
	bool is_running;
	bool is_mesh_connected;
	bool is_tods_reachable;
	mesh_addr_t mesh_parent_addr;

	// wifi config
	esp_netif_t* netif_sta;
	uint8_t mac_addr[6];

	// mqtt config
	bool is_mqtt_connected;
	char* mqtt_device_id;
	char* mqtt_subscribed_topic;
	int mqtt_subscribe_status;
	void (*node_task_start)(void);
	void (*mqtt_node_start)(void);
	void (*mqtt_receive_handler)(char* topic, char* data);
	
	uint32_t led_status;
} fatec_mesh_config_t;

#endif