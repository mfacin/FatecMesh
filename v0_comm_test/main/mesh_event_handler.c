#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "config.h"

// these are all mesh events. a description can be found at
// https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/api-reference/network/esp-wifi-mesh.html#enumerations
void mesh_event_handler(void *arg, esp_event_base_t event_base,	int32_t event_id, void *event_data) {
	mesh_config_t* config = (mesh_config_t*) arg;

	mesh_addr_t id = { 0, };
	static uint16_t last_layer = 0;

	switch (event_id) {

	case MESH_EVENT_STARTED:
		esp_mesh_get_id(&id);
		ESP_LOGI(config->MESH_TAG, "> Mesh started with id: " MACSTR "", MAC2STR(id.addr));
		config->is_mesh_connected = false;
		config->mesh_layer = esp_mesh_get_layer();
	break;

	case MESH_EVENT_STOPPED:
		ESP_LOGI(config->MESH_TAG, "> Mesh stopped");
		config->is_mesh_connected = false;
		config->mesh_layer = esp_mesh_get_layer();
	break;

	case MESH_EVENT_CHILD_CONNECTED:
		mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Child (" MACSTR ") connected with aid: %d",
				MAC2STR(child_connected->mac), child_connected->aid);
	break;

	case MESH_EVENT_CHILD_DISCONNECTED:
		mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Child (" MACSTR ") disconnected with aid: %d",
				MAC2STR(child_disconnected->mac), child_disconnected->aid);
	break;

	case MESH_EVENT_PARENT_CONNECTED:
		mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
		esp_mesh_get_id(&id);
		config->mesh_layer = connected->self_layer;
		memcpy(&config->mesh_parent_addr.addr, connected->connected.bssid, 6);

		ESP_LOGI(config->MESH_TAG, "> Parent (" MACSTR "%s) connected: ID:" MACSTR ", duty:%d",
				MAC2STR(config->mesh_parent_addr.addr), esp_mesh_is_root() ? " ROOT" : "",
				MAC2STR(id.addr), connected->duty);
		ESP_LOGI(config->MESH_TAG, "> Layer change: %d --> %d%s", last_layer, config->mesh_layer, esp_mesh_is_root() ? " ROOT" : "");
	
		last_layer = config->mesh_layer;
		config->is_mesh_connected = true;

		// if is the new root, get an ip address
		if (esp_mesh_is_root())	{
			esp_netif_dhcpc_stop(config->netif_sta);
			esp_netif_dhcpc_start(config->netif_sta);
		}

		// starting the communication
		config->comm_task_start();
	break;

	case MESH_EVENT_PARENT_DISCONNECTED:
		mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Parent disconnected: %d", disconnected->reason);
		config->is_mesh_connected = false;
		config->mesh_layer = esp_mesh_get_layer();
	break;

	case MESH_EVENT_NO_PARENT_FOUND:
		mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> No parent found: scan times: %d", no_parent->scan_times);
	break;

	case MESH_EVENT_LAYER_CHANGE:
		mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
		config->mesh_layer = layer_change->new_layer;
		ESP_LOGI(config->MESH_TAG, "> Layer change: %d --> %d%s", last_layer, config->mesh_layer, esp_mesh_is_root() ? " ROOT" : "");
		last_layer = config->mesh_layer;
	break;

	case MESH_EVENT_VOTE_STARTED:
		mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Vote started: attempts:%d, reason:%d, rc_addr:" MACSTR "",
				vote_started->attempts, vote_started->reason, MAC2STR(vote_started->rc_addr.addr));
	break;

	case MESH_EVENT_VOTE_STOPPED:
		ESP_LOGI(config->MESH_TAG, "> Vote stopped");
	break;

	case MESH_EVENT_ROOT_ADDRESS:
		mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Root address: " MACSTR "", MAC2STR(root_addr->addr));
	break;

	case MESH_EVENT_ROOT_ASKED_YIELD:
		mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Root yield by " MACSTR ": rssi: %d, capacity: %d",
				MAC2STR(root_conflict->addr), root_conflict->rssi, root_conflict->capacity);
	break;

	case MESH_EVENT_ROOT_FIXED:
		mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Root %s", root_fixed->is_fixed ? "fixed" : "not fixed");
	break;

	case MESH_EVENT_ROOT_SWITCH_REQ:
		mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Root switch requested by " MACSTR ": reason: %d",
				MAC2STR(switch_req->rc_addr.addr), switch_req->reason);
	break;

	case MESH_EVENT_ROOT_SWITCH_ACK:
		config->mesh_layer = esp_mesh_get_layer();
		esp_mesh_get_parent_bssid(&config->mesh_parent_addr);
		ESP_LOGI(config->MESH_TAG, "> Root switch acknowledged: layer: %d, parent: " MACSTR "",
				config->mesh_layer, MAC2STR(config->mesh_parent_addr.addr));
	break;

	case MESH_EVENT_TODS_STATE:
		mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> ToDS state: %sREACHABLE", *toDs_state ? "" : "UN");

		config->is_tods_reachable = *toDs_state;
	break;

	case MESH_EVENT_ROUTER_SWITCH:
		mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> New router: %s (" MACSTR "), channel %d",
				router_switch->ssid, MAC2STR(router_switch->bssid), router_switch->channel);
	break;

	case MESH_EVENT_ROUTING_TABLE_ADD:
		mesh_event_routing_table_change_t *routing_table_add = (mesh_event_routing_table_change_t *)event_data;
		ESP_LOGW(config->MESH_TAG, "> Routing table change: add %d, new:%d, layer:%d",
				routing_table_add->rt_size_change, routing_table_add->rt_size_new, config->mesh_layer);
	break;

	case MESH_EVENT_ROUTING_TABLE_REMOVE:
		mesh_event_routing_table_change_t *routing_table_remove = (mesh_event_routing_table_change_t *)event_data;
		ESP_LOGW(config->MESH_TAG, "> Routing table change: remove %d, new:%d, layer:%d",
				routing_table_remove->rt_size_change, routing_table_remove->rt_size_new, config->mesh_layer);
	break;

	case MESH_EVENT_CHANNEL_SWITCH:
		mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Channel switched to : %d", channel_switch->channel);
	break;

	case MESH_EVENT_SCAN_DONE:
		mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Scan done: %d APs scanned", scan_done->number);
	break;

	case MESH_EVENT_NETWORK_STATE:
		mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Has a root: %d", !network_state->is_rootless);
	break;

	case MESH_EVENT_STOP_RECONNECTION:
		ESP_LOGI(config->MESH_TAG, "> Reconnection stopped");
	break;

	case MESH_EVENT_FIND_NETWORK:
		mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Find network in channel %d: router BSSID:" MACSTR "",
				find_network->channel, MAC2STR(find_network->router_bssid));
	break;

	case MESH_EVENT_PS_PARENT_DUTY:
		mesh_event_ps_duty_t *ps_parent_duty = (mesh_event_ps_duty_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Parent duty: %d", ps_parent_duty->duty);
	break;

	case MESH_EVENT_PS_CHILD_DUTY:
		mesh_event_ps_duty_t *ps_child_duty = (mesh_event_ps_duty_t *)event_data;
		ESP_LOGI(config->MESH_TAG, "> Child (" MACSTR " / %d) duty: %d", MAC2STR(ps_child_duty->child_connected.mac),
				ps_child_duty->child_connected.aid - 1, ps_child_duty->duty);
	break;

	default:
		ESP_LOGI(config->MESH_TAG, "> Event with unknown id: %" PRId32 "", event_id);
	break;
	
	}
}