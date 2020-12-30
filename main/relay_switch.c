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
 * @brief Implementation of switch state handling.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_err.h>
#include <time.h>
#include <sys/time.h>

#include "relay_switch.h"
#include "user_config.h"
#include "platform_time.h"

#define TAG "relay_switch"

typedef struct scheduled_switch
{
    uint32_t timeout;
    bool is_switched_on;
} scheduled_switch_t;

static scheduled_switch_t scheduled_switch = { 0, false };

static state_changed_cb_t state_changed_callback = NULL;
static void* state_changed_context = NULL;
static relay_switch_state_t current_state;
static uint64_t timeout_start = 0;
static TaskHandle_t timeout_task = NULL;

static esp_err_t relay_switch_set_state_internal(bool switch_on, uint32_t timeout);

/**
 * Get current UTC time in ms (synchronized by SNTP)
 */
static uint64_t get_utc_now()
{
    struct timeval tv;
    uint64_t millisecondsSinceEpoch = 0;
    gettimeofday(&tv, NULL);
    millisecondsSinceEpoch = (uint64_t) (tv.tv_sec) * 1000 + (uint64_t) (tv.tv_usec) / 1000;
    return millisecondsSinceEpoch;
}

esp_err_t relay_switch_init()
{
    current_state.is_switched_on = false;
    current_state.last_change_utc_millis = platform_get_utc_millis();
    current_state.switch_timeout_millis = 0;
	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; //disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT; //set as output mode
    io_conf.pin_bit_mask = (1ULL<<RELAY_GPIO_NUM);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    esp_err_t result = gpio_config(&io_conf);
#if !HIGH_ON
    if (result == ESP_OK)
        result = gpio_set_level(RELAY_GPIO_NUM, (uint32_t)1);
#endif
    return result;
}

static bool get_switch_value(bool switch_on)
{
#if HIGH_ON
    return switch_on;
#else
    return !switch_on;
#endif
}

static void relay_switch_timeout_task(void* pvParameters)
{
    scheduled_switch_t scheduled_switch = *((scheduled_switch_t*)pvParameters);
    timeout_start = get_utc_now();
    TickType_t now = xTaskGetTickCount();
    ESP_LOGI(TAG, "Scheduling timeout to: %u ms", scheduled_switch.timeout);
    vTaskDelayUntil(&now, scheduled_switch.timeout / portTICK_PERIOD_MS);
    relay_switch_set_state_internal(scheduled_switch.is_switched_on, 0);

    scheduled_switch.is_switched_on = false;
    scheduled_switch.timeout = 0;
    timeout_task = NULL;
    vTaskDelete(NULL);
}

static uint32_t relay_switch_get_expire_ms()
{
    uint64_t now = get_utc_now();
    if (scheduled_switch.timeout == 0 || now <= timeout_start)
    {
        return 0;
    }
    uint32_t diff = (uint32_t)(now - timeout_start);
    if (diff > scheduled_switch.timeout)
    {
        return 0;
    }
    return scheduled_switch.timeout - diff;
}

esp_err_t relay_switch_set_state(bool switch_on, uint32_t timeout)
{
    if (timeout_task != NULL)
    {
        scheduled_switch.is_switched_on = false;
        scheduled_switch.timeout = 0;
        vTaskDelete(timeout_task);
    }
	return relay_switch_set_state_internal(switch_on, timeout);
}

esp_err_t relay_switch_set_state_internal(bool switch_on, uint32_t timeout)
{
    ESP_LOGI(TAG, "Set new state: %s", switch_on ? "true" : "false");
    esp_err_t error = gpio_set_level(RELAY_GPIO_NUM, (uint32_t)get_switch_value(switch_on));
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_set_level failed: %d", error);
        return error;
    }
    current_state.is_switched_on = switch_on;
    current_state.last_change_utc_millis = platform_get_utc_millis();
    current_state.switch_timeout_millis = timeout;
    if (timeout > 0)
    {
        scheduled_switch.timeout = timeout;
        scheduled_switch.is_switched_on = !switch_on;
        // Schedule task which changes switch position after timeout elapsed
        xTaskCreate(relay_switch_timeout_task, "measurement_task_run", 4096, &scheduled_switch, tskIDLE_PRIORITY, &timeout_task);
    }
    if (state_changed_callback != NULL)
    {
        state_changed_callback(&current_state, state_changed_context);
    }
    return ESP_OK;
}

relay_switch_state_t relay_switch_get_state()
{
    current_state.switch_timeout_millis = relay_switch_get_expire_ms();
	return current_state;
}

void relay_switch_set_state_changed_cb(state_changed_cb_t callback, void* context)
{
	state_changed_callback = callback;
	state_changed_context = context;
}
