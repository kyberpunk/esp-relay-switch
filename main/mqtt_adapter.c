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

#include <string.h>
#include <stdio.h>
#include <mqtt_client.h>
#include <esp_log.h>

#include "mqtt_adapter.h"
#include "json_serializer.h"
#include "relay_switch.h"
#include "user_config.h"

#define MQTT_STATE_TOPIC "switch/state"
#define MQTT_SWITCH_TOPIC "switch/" SWITCH_ID "/switch"
#define TAG "mqtt_adapter"

static esp_mqtt_client_handle_t mqtt_client = NULL;

esp_err_t get_switch_from_json(esp_mqtt_event_handle_t event, bool *value, uint32_t *timeout)
{
    char *received_data = malloc((event->data_len + 1) * sizeof(char));
    memcpy(received_data, event->data, event->data_len);
    received_data[event->data_len] = '\0';
    return json_serializer_deserialize(received_data, value, timeout);
}

static bool is_valid_switch_request(esp_mqtt_event_handle_t event)
{
    if (strlen(MQTT_SWITCH_TOPIC) != event->topic_len)
    {
        return false;
    }
    if (strncmp(MQTT_SWITCH_TOPIC, event->topic, event->topic_len) == 0)
    {
        return true;
    }
    return false;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        const char switch_topic[] = MQTT_SWITCH_TOPIC;
        ESP_LOGI(TAG, "Subscribing to topic %s", switch_topic);
        esp_mqtt_client_subscribe(mqtt_client, switch_topic, 2);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        ESP_LOGI(TAG, "Topic %.*s", event->topic_len, event->topic);
        if (is_valid_switch_request(event))
        {
            bool switch_on;
            uint32_t timeout;
            esp_err_t  error = get_switch_from_json(event, &switch_on, &timeout);
            if (error == ESP_OK)
            {
                relay_switch_set_state(switch_on, timeout);
            }
            else
            {
                const char* error_string = esp_err_to_name(error);
                ESP_LOGW(TAG, "Mqtt payload parse failed: %s", error_string);
            }
        }
        else
        {
            ESP_LOGW(TAG, "Publish received from unknown topic.");
        }
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        break;
    }
    return ESP_OK;
}

esp_err_t mqtt_adapter_init()
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .host = MQTT_BROKER_HOST,
        .event_handle = mqtt_event_handler,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL)
    {
        return ESP_FAIL;
    }
    esp_err_t error = esp_mqtt_client_start(mqtt_client);
    return error;
}

esp_err_t mqtt_adapter_notify_switch_status(const relay_switch_state_t* switch_state)
{
    char *serialized_string = NULL;
    size_t length = 0;
    esp_err_t error = json_serializer_serialize(switch_state, &serialized_string, &length);
    if (error != ESP_OK) return error;
    esp_mqtt_client_publish(mqtt_client, MQTT_STATE_TOPIC, serialized_string, length, 1, false);
    json_serializer_free(serialized_string);
    return ESP_OK;
}
