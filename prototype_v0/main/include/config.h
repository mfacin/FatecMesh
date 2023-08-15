#ifndef CONFIG_HEADER
#define CONFIG_HEADER

#include <inttypes.h>
#include <stdbool.h>
#include "esp_mesh.h"

typedef struct {
	char *MESH_TAG;
	uint8_t MESH_ID[6];
	uint8_t MAC[6];
	bool is_running;
	bool is_mesh_connected;
	bool is_tods_reachable;
	bool is_mqtt_connected;
	int mesh_layer;
	mesh_addr_t mesh_parent_addr;
	esp_netif_t *netif_sta;

	esp_err_t (*mqtt_task_start)(void);
} mesh_config_t;

#endif