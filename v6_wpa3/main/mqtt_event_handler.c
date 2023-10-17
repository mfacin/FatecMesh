#include "config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

typedef struct {
    char* topic;
    char* data;
} fatec_mqtt_data;

// mqtt event handler
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
	fatec_mesh_config_t* config = (fatec_mesh_config_t*) handler_args;
    esp_mqtt_event_handle_t event = event_data;
    // esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t) event_id) {

    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(config->mesh_tag, "> MQTT client initialized, about to start connection");
    break;

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(config->mesh_tag, "> MQTT connected");
		config->is_mqtt_connected = true;
	break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGE(config->mesh_tag, "> MQTT connection aborted");
		config->is_mqtt_connected = false;
	break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(config->mesh_tag, "> MQTT subscribed: %d", event->msg_id);
	break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(config->mesh_tag, "> MQTT unsubscribed: %d", event->msg_id);
	break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(config->mesh_tag, "> MQTT published: %d", event->msg_id);
	break;

    case MQTT_EVENT_DATA: {
        ESP_LOGI(config->mesh_tag, "> MQTT received data on topic %.*s: %.*s",
				event->topic_len, event->topic, event->data_len, event->data);
        
        // getting only the topic
        char* topic;
        asprintf(&topic, "%.*s", event->topic_len, event->topic);
                
        config->mqtt_receive_handler(topic, event->data);

        free(topic);
    }
	break;

    case MQTT_EVENT_ERROR:
        switch (event->error_handle->error_type) {
        case MQTT_ERROR_TYPE_NONE:
            ESP_LOGE(config->mesh_tag, "> MQTT error: MQTT_ERROR_TYPE_NONE");
        break;

        case MQTT_ERROR_TYPE_TCP_TRANSPORT:
            ESP_LOGE(config->mesh_tag, "> MQTT error: MQTT_ERROR_TYPE_TCP_TRANSPORT");
            ESP_LOGE(config->mesh_tag, "\treturn code: %d", event->error_handle->connect_return_code);
            ESP_LOGE(config->mesh_tag, "\ttls stack error: %d", event->error_handle->esp_tls_stack_err);
            ESP_LOGE(config->mesh_tag, "\tsocket error: %d", event->error_handle->esp_transport_sock_errno);
            ESP_LOGE(config->mesh_tag, "\tlast esp error: %d", event->error_handle->esp_tls_last_esp_err);
        break;

        case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
            ESP_LOGE(config->mesh_tag, "> MQTT error: MQTT_ERROR_TYPE_CONNECTION_REFUSED");
        break;

        case MQTT_ERROR_TYPE_SUBSCRIBE_FAILED:
            ESP_LOGE(config->mesh_tag, "> MQTT error: MQTT_ERROR_TYPE_SUBSCRIBE_FAILED");
        break;
        
        default:
            ESP_LOGE(config->mesh_tag, "> MQTT error: unknown type");
        break;
        }
	break;

    default:
        ESP_LOGI(config->mesh_tag, "> MQTT event with unknown id: %d", event->event_id);
	break;

    }
}