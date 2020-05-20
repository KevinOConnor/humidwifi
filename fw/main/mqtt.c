// Handle mqtt connections
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <string.h> // memcmp
#include <esp_log.h> // ESP_LOGI
#include <freertos/FreeRTOS.h> // xEventGroupCreate
#include <freertos/event_groups.h> // xEventGroupCreate
#include <mqtt_client.h> // esp_mqtt_client_init
#include "datalog.h" // datalog_format
#include "deepsleep.h" // deepsleep_note_ota_start
#include "network.h" // network_note_ota_start
#include "ota.h" // ota_start
#include "sdkconfig.h" // CONFIG_TOPIC

#define DATA_TOPIC CONFIG_MQTT_TOPIC_PREFIX "/data"
#define OTA_TOPIC CONFIG_MQTT_TOPIC_PREFIX "/ota_url"

static const char *TAG = "MQTT";


/****************************************************************
 * Command download
 ****************************************************************/

static void
mqtt_hdl_connected(void *handler_args, esp_event_base_t base
                 , int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    int msg_id = esp_mqtt_client_subscribe(event->client, OTA_TOPIC, 1);
    ESP_LOGI(TAG, "sent subscribe, msg_id=%d", msg_id);
}

static void
mqtt_hdl_subscribed(void *handler_args, esp_event_base_t base
                    , int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    int msg_id = esp_mqtt_client_publish(event->client, OTA_TOPIC, "", 0, 1, 0);
    ESP_LOGI(TAG, "sent publish, msg_id=%d", msg_id);
}

#define OTA_CHECK_EVENT 1
static int ota_in_progress;

static void
mqtt_hdl_data(void *handler_args, esp_event_base_t base
              , int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    if (ota_in_progress || event->topic_len != strlen(OTA_TOPIC)
        || memcmp(event->topic, OTA_TOPIC, strlen(OTA_TOPIC)) != 0)
        return;
    ESP_LOGI(TAG, "Got ota_update response len=%d", event->data_len);
    if (event->data_len) {
        // OTA request
        ota_in_progress = 1;
        deepsleep_note_ota_start();
        network_note_ota_start();
        int msg_id = esp_mqtt_client_publish(event->client, OTA_TOPIC, ""
                                             , 0, 0, 1);
        ESP_LOGI(TAG, "sent publish clear, msg_id=%d", msg_id);
        ota_start(event->data, event->data_len);
    }
    EventGroupHandle_t ota_event_group = handler_args;
    xEventGroupSetBits(ota_event_group, OTA_CHECK_EVENT);
}


/****************************************************************
 * Startup
 ****************************************************************/

// Start connection signal
static void
on_got_ip(void *arg, esp_event_base_t event_base
          , int32_t event_id, void *event_data)
{
    esp_mqtt_client_handle_t client = arg;
    esp_mqtt_client_start(client);
}

static void
mqtt_hdl_published(void *handler_args, esp_event_base_t base
                   , int32_t event_id, void *event_data)
{
    TaskHandle_t publish_task = handler_args;
    xTaskNotifyGive(publish_task);
}

static void
mqtt_hdl_error(void *handler_args, esp_event_base_t base
               , int32_t event_id, void *event_data)
{
    ESP_LOGW(TAG, "Got MQTT error. Entering deep sleep now.");
    deepsleep_start_sleep();
}

void
mqtt_start(void)
{
    EventGroupHandle_t ota_event_group = xEventGroupCreate();
    TaskHandle_t publish_task = xTaskGetCurrentTaskHandle();

    // Connect to mqtt server
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
        .disable_auto_reconnect = true,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DISCONNECTED
                                   , mqtt_hdl_error, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ERROR
                                   , mqtt_hdl_error, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_CONNECTED
                                   , mqtt_hdl_connected, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_SUBSCRIBED
                                   , mqtt_hdl_subscribed, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DATA
                                   , mqtt_hdl_data, ota_event_group);
    esp_mqtt_client_register_event(client, MQTT_EVENT_PUBLISHED
                                   , mqtt_hdl_published, publish_task);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP
                               , &on_got_ip, client);

    // Upload pending datalog entries
    int pos = -1, count = 0;
    for (;;) {
        char buf[256];
        int ret = datalog_format(&pos, buf, sizeof(buf));
        if (ret < 0)
            break;
        ESP_LOGW(TAG, "data publish %d '%.*s'", count, ret, buf);
        esp_mqtt_client_publish(client, DATA_TOPIC, buf, ret, 1, 1);
        count++;
    }

    // Wait for ota publish ack
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

    // Wait for acks from sent data
    while (count--) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        datalog_expire();
    }

    // Wait for ota check to complete
    xEventGroupWaitBits(ota_event_group, OTA_CHECK_EVENT
                        , true, true, portMAX_DELAY);
    esp_mqtt_client_stop(client);
    esp_mqtt_client_disconnect(client);
    if (ota_in_progress)
        vTaskDelay(portMAX_DELAY);
}
