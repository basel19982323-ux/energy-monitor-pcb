// network.h - Wi-Fi station + MQTT publishing.
#pragma once

#include <stdbool.h>

// Start Wi-Fi; MQTT starts by itself once an IP address is received.
// Both reconnect automatically in the background.
void network_start(void);

// True while connected to the MQTT broker.
bool network_is_online(void);

// Publish one message to MQTT_TOPIC. Does nothing while offline.
void network_publish(const char *message);
