#include "config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

// mqtt event handler
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
	mesh_config_t* config = (mesh_config_t*) handler_args;
    esp_mqtt_event_handle_t event = event_data;
    // esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t) event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(config->MESH_TAG, "> MQTT connected");
		config->is_mqtt_connected = true;
	break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(config->MESH_TAG, "> MQTT disconnected");
		config->is_mqtt_connected = false;
	break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(config->MESH_TAG, "> MQTT subscribed: %d", event->msg_id);
	break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(config->MESH_TAG, "> MQTT unsubscribed: %d", event->msg_id);
	break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(config->MESH_TAG, "> MQTT published: %d", event->msg_id);
	break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(config->MESH_TAG, "> MQTT received data on topic %.*s: %.*s",
				event->topic_len, event->topic, event->data_len, event->data);
	break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(config->MESH_TAG, "> MQTT error");
	break;

    default:
        ESP_LOGI(config->MESH_TAG, "> MQTT event with unknown id: %d", event->event_id);
	break;

    }
}