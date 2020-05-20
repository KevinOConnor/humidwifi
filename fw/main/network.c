// Handle esp32 wifi connections
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <esp_log.h> // ESP_LOGI
#include <esp_netif.h> // esp_netif_init
#include <esp_wifi.h> // esp_wifi_init
#include <nvs_flash.h> // nvs_flash_init
#include "deepsleep.h" // deepsleep_start_sleep()
#include "network.h" // network_connect
#include "sdkconfig.h" // CONFIG_WIFI_SSID

static const char *TAG = "NETWORK";

// Initialize esp32 core functions
static int
esp32_init(void)
{
    int ret = nvs_flash_init();
    if (ret) {
        ret = nvs_flash_erase();
        if (ret)
            goto fail;
        ret = nvs_flash_init();
        if (ret)
            goto fail;
    }
    ret = esp_event_loop_create_default();
    if (ret)
        goto fail;
    return 0;
fail:
    ESP_LOGW(TAG, "Error in esp32_init %d", ret);
    return ret;
}

static RTC_DATA_ATTR esp_netif_ip_info_t Last_ip_info;
static RTC_DATA_ATTR esp_netif_dns_info_t Last_dns_info;
static RTC_DATA_ATTR uint64_t Last_ip_valid_time;

static void
on_got_ip(void *arg, esp_event_base_t event_base
          , int32_t event_id, void *event_data)
{
    esp_netif_t *netif = arg;
    int ret = esp_netif_get_ip_info(netif, &Last_ip_info);
    if (ret)
        goto fail;
    ret = esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &Last_dns_info);
    if (ret)
        goto fail;
    uint64_t vtime = CONFIG_DHCP_LEASE_HOURS * 60ULL * 60 * 1000000;
    Last_ip_valid_time = deepsleep_get_wake_time() + vtime;
    return;

fail:
    ESP_LOGW(TAG, "Error in on_got_ip %d", ret);
}

// Initialize tcp/ip
static int
ip_init(void)
{
    int ret = esp_netif_init();
    if (ret)
        goto fail;
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    if (!netif)
        goto fail;
    if (deepsleep_get_wake_time() >= Last_ip_valid_time) {
        ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP
                                         , &on_got_ip, netif);
        if (ret)
            goto fail;
        return 0;
    }

    ret = esp_netif_dhcpc_stop(netif);
    if (ret)
        goto fail;
    ret = esp_netif_set_ip_info(netif, &Last_ip_info);
    if (ret)
        goto fail;
    ret = esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &Last_dns_info);
    if (ret)
        goto fail;

    return 0;

fail:
    ESP_LOGW(TAG, "Error in ip_init %d", ret);
    return ret;
}

static RTC_DATA_ATTR uint8_t Last_channel;

static void
on_wifi_connect(void *arg, esp_event_base_t event_base
                , int32_t event_id, void *event_data)
{
    wifi_event_sta_connected_t *e = event_data;
    Last_channel = e->channel;
}

static int no_sleep_on_disconnect;

static void
on_wifi_disconnect(void *arg, esp_event_base_t event_base
                   , int32_t event_id, void *event_data)
{
    if (no_sleep_on_disconnect)
        return;
    wifi_event_sta_disconnected_t *e = event_data;
    ESP_LOGW(TAG, "Wifi disconnect %d", e->reason);
    deepsleep_start_sleep();
}

int
network_start(void)
{
    int ret = esp32_init();
    if (ret)
        goto fail;
    ret = ip_init();
    if (ret)
        goto fail;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret)
        goto fail;
    ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED
                                     , &on_wifi_connect, NULL);
    if (ret)
        goto fail;
    ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED
                                     , &on_wifi_disconnect, NULL);
    if (ret)
        goto fail;
    ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (ret)
        goto fail;
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .channel = Last_channel,
        },
    };
    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ret = esp_wifi_set_ps(WIFI_PS_NONE);
    if (ret)
        goto fail;
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret)
        goto fail;
    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    if (ret)
        goto fail;
    ret = esp_wifi_start();
    if (ret)
        goto fail;
    ret = esp_wifi_connect();
    if (ret)
        goto fail;
    return 0;

fail:
    ESP_LOGW(TAG, "Error in network_start %d", ret);
    return ret;
}

void
network_disconnect(void)
{
    no_sleep_on_disconnect = 1;
    int ret = esp_wifi_disconnect();
    if (ret)
        ESP_LOGW(TAG, "Error in network_disconnect %d", ret);
}

void
network_note_ota_start(void)
{
    no_sleep_on_disconnect = 1;
}
