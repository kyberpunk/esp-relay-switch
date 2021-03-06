/*
 *  Copyright (c) 2019, Vit Holasek.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @author Vit Holasek
 * @brief Main file with application entrypoint.
 */

#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs_flash.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include <esp_sntp.h>
#include <esp_http_server.h>

#include "http_adapter_html.h"
#include "http_adapter_json.h"
#include "mqtt_adapter.h"
#include "platform_time.h"
#include "relay_switch.h"
#include "user_config.h"

#define TAG "main"
#define DHT_GPIO_NUM GPIO_NUM_27
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
const int SNTP_SYNCHRONIZED_BIT = BIT1;

#if HTTP_PLAIN_ENABLE || HTTP_JSON_ENABLE
static httpd_handle_t server;
static httpd_config_t config = HTTPD_DEFAULT_CONFIG();
#endif

/**
 * Handle Wi-Fi connection events and set related bits for tasks synchronization.
 */
static void event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "retry to connect to the AP");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init()
{
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASSWORD);
}

static void nvs_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void time_sync_notification_cb(struct timeval *tv)
{
    uint64_t utcMs = ((uint64_t)tv->tv_sec) * 1000 + tv->tv_usec;
    ESP_LOGI(TAG, "UTC time synchronized: %llu", utcMs);
    xEventGroupSetBits(wifi_event_group, SNTP_SYNCHRONIZED_BIT);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTP_SERVER);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    sntp_init();
}

void switch_state_changed(const relay_switch_state_t* relay_switch_state, void* context)
{
#if MQTT_ADAPTER_ENABLE
    mqtt_adapter_notify_switch_status(relay_switch_state);
#endif
}

void init_httpd()
{

}

void app_main(void)
{
    wifi_event_group = xEventGroupCreate();
    nvs_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "WiFi init");
    wifi_init();
    ESP_LOGI(TAG, "Connecting to WiFi...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE,
        pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to WiFi");
    ESP_LOGI(TAG, "SNTP init");
    initialize_sntp();
    ESP_LOGI(TAG, "Waiting for SNTP synchronize...");
    xEventGroupWaitBits(wifi_event_group, SNTP_SYNCHRONIZED_BIT, pdFALSE,
        pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "SNTP synchronized");
    ESP_LOGI(TAG, "Connecting to MQTT...");
#if MQTT_ADAPTER_ENABLE
    ESP_ERROR_CHECK(mqtt_adapter_init());
#endif

#if HTTP_HTML_ENABLE || HTTP_JSON_ENABLE
    ESP_ERROR_CHECK(httpd_start(&server, &config));
#endif
#if HTTP_HTML_ENABLE
    ESP_ERROR_CHECK(http_adapter_html_init(server));
#endif
#if HTTP_JSON_ENABLE
    ESP_ERROR_CHECK(http_adapter_json_init(server));
#endif
    ESP_ERROR_CHECK(relay_switch_init());
    relay_switch_set_state_changed_cb(switch_state_changed, NULL);
}

uint64_t platform_get_utc_millis()
{
    struct timeval tv;
    uint64_t millisecondsSinceEpoch = 0;

    gettimeofday(&tv, NULL);
    millisecondsSinceEpoch = (uint64_t) (tv.tv_sec) * 1000 + (uint64_t) (tv.tv_usec) / 1000;

    return millisecondsSinceEpoch;
}
