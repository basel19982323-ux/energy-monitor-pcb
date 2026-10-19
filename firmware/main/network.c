// network.c - Wi-Fi station + MQTT publishing.
#include "network.h"

#include <stdio.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "config.h"

static esp_mqtt_client_handle_t s_mqtt = NULL;   // NULL until MQTT is started
static volatile bool s_online = false;           // true while connected to the broker

// Called by the MQTT client when its state changes.
static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == MQTT_EVENT_CONNECTED) {
        s_online = true;
        printf("[mqtt] connected to %s\n", MQTT_BROKER_URI);
    } else if (id == MQTT_EVENT_DISCONNECTED) {
        s_online = false;
        printf("[mqtt] disconnected (retrying automatically)\n");
    }
}

static void mqtt_start(void)
{
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    s_mqtt = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(s_mqtt, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_mqtt);
}

// Called by ESP-IDF when the Wi-Fi state changes.
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        printf("[wifi] connecting to %s...\n", WIFI_SSID);
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("[wifi] disconnected, retrying\n");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        printf("[wifi] connected\n");
        if (s_mqtt == NULL) {
            mqtt_start();                        // first connection only; MQTT reconnects by itself
        }
    }
}

void network_start(void)
{
    // Wi-Fi keeps its calibration data in NVS flash. Erase and retry if the partition is full or outdated.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .channel  = WIFI_CHANNEL,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

bool network_is_online(void)
{
    return s_online;
}

void network_publish(const char *message)
{
    if (!s_online) {
        return;                                  // offline: skip, the next reading will try again
    }
    esp_mqtt_client_publish(s_mqtt, MQTT_TOPIC, message, 0, 0, 0);
    printf("[mqtt] sent %s\n", message);
}
