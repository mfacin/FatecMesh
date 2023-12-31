#ifndef MQTT_HANDLER_HEADER
#define MQTT_HANDLER_HEADER

#include "esp_event.h"

typedef struct {
    char* topic;
    char* data;
} fatec_mqtt_data;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#endif