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

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_err.h>

#include "boiler_controller.h"
#include "user_config.h"
#include "platform_time.h"

#define TAG "boiler_controller"

static state_changed_cb_t state_changed_callback = NULL;
static void* state_changed_context = NULL;
static boiler_controller_state_t current_state;
static TickType_t timeout_start = 0;
static uint32_t switch_timeout = 0;
static TaskHandle_t timeout_task = NULL;

esp_err_t boiler_controller_init()
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

static bool get_switch_value(switch_on)
{
#if HIGH_ON
    return switch_on;
#else
    return !switch_on;
#endif
}

static void boiler_controller_timeout_task(void* pvParameters)
{
    uint32_t timeout = *((uint32_t*)pvParameters);
    TickType_t timeout_start = xTaskGetTickCount();
    ESP_LOGI(TAG, "Scheduling timeout to: %lu ms", timeout);
    vTaskDelayUntil(&timeout_start, timeout / portTICK_PERIOD_MS);
    boiler_controller_set_state(!current_state.is_switched_on, 0);
    vTaskDelete(NULL);
}

static uint32_t boiler_controller_get_expire_ms()
{
    TickType_t now = xTaskGetTickCount();
    if (now <= timeout_start)
    {
        return 0;
    }
    uint32_t diff = (now - timeout_start) * portTICK_PERIOD_MS;
    if (diff > switch_timeout)
    {
        return 0;
    }
    return switch_timeout - diff;
}

esp_err_t boiler_controller_set_state(bool switch_on, uint32_t timeout)
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
	current_state.switch_timeout_millis = boiler_controller_get_expire_ms();
	if (timeout > 0)
	{
	    switch_timeout = timeout;
	    xTaskCreate(boiler_controller_timeout_task, "measurement_task_run", 4096, &switch_timeout, tskIDLE_PRIORITY, &timeout_task);
	}
	if (state_changed_callback != NULL)
	{
		state_changed_callback(&current_state, state_changed_context);
	}
	return ESP_OK;
}

boiler_controller_state_t boiler_controller_get_state()
{
    current_state.switch_timeout_millis = boiler_controller_get_expire_ms();
	return current_state;
}

void boiler_controller_set_state_changed_cb(state_changed_cb_t callback, void* context)
{
	state_changed_callback = callback;
	state_changed_context = context;
}
