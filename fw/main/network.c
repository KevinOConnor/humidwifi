// Handle esp32 wifi connections
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <esp_log.h> // ESP_LOGI
#include <esp_wifi.h> // esp_wifi_init
#include <lwip/dns.h> // dns_setserver
#include <lwip/inet.h> // inet_aton
#include <nvs_flash.h> // nvs_flash_init
#include <tcpip_adapter.h> // tcpip_adapter_init
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

// Initialize tcp/ip
static int
ip_init(void)
{
#if CONFIG_USE_STATIC_IP
    int ret = tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
    if (ret)
        goto fail;
    tcpip_adapter_ip_info_t ipInfo;
    ret = inet_aton(CONFIG_IP_ADDRESS, &ipInfo.ip);
    if (ret != 1)
        goto fail;
    ret = inet_aton(CONFIG_NETMASK, &ipInfo.netmask);
    if (ret != 1)
        goto fail;
    ret = inet_aton(CONFIG_GATEWAY_IP, &ipInfo.gw);
    if (ret != 1)
        goto fail;
    ret = tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    if (ret)
        goto fail;

    ip_addr_t d;
    d.type = IPADDR_TYPE_V4;
    ret = inet_aton(CONFIG_DNS_IP, &d.u_addr.ip4);
    if (ret != 1)
        goto fail;
    dns_setserver(0, &d);

    return 0;

fail:
    ESP_LOGW(TAG, "Error in ip_init %d", ret);
    return ret;
#endif
}

static RTC_DATA_ATTR uint8_t last_channel;

static void
on_wifi_connect(void *arg, esp_event_base_t event_base
                , int32_t event_id, void *event_data)
{
    wifi_event_sta_connected_t *e = event_data;
    last_channel = e->channel;
}

static int no_sleep_on_disconnect;

static void
on_wifi_disconnect(void *arg, esp_event_base_t event_base
                   , int32_t event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *e = event_data;
    ESP_LOGW(TAG, "Wifi disconnect %d", e->reason);
    if (!no_sleep_on_disconnect)
        deepsleep_start_sleep();
}

int
network_start(void)
{
    int ret = esp32_init();
    if (ret)
        goto fail;
    tcpip_adapter_init();
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
            .channel = last_channel,
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
    int ret = esp_wifi_stop();
    if (ret != ESP_ERR_WIFI_NOT_INIT)
        esp_wifi_deinit();
}

void
network_note_ota_start(void)
{
    no_sleep_on_disconnect = 1;
}
