#ifndef EVENT_HANDLER_HEADER
#define EVENT_HANDLER_HEADER

#include "esp_event.h"

void mesh_event_handler(void *arg, esp_event_base_t event_base,	int32_t event_id, void *event_data);

#endif