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

#include <esp_log.h>
#include <string.h>

#include "json_serializer.h"
#include "user_config.h"
#include "parson.h"

#define TAG "json_serializer"

esp_err_t json_serializer_deserialize(const char *received_data, bool *value, uint32_t *timeout)
{
    esp_err_t error = ESP_FAIL;
    JSON_Value *root_value;
    JSON_Object *switch_data;
    root_value = json_parse_string(received_data);
    if (root_value == NULL)
    {
        ESP_LOGE(TAG, "Cannot parse JSON.");
        return error;
    }
    switch_data = json_value_get_object(root_value);
    const char *switch_on_name = "switchedOn";
    if (json_object_has_value_of_type(switch_data, switch_on_name, JSONBoolean))
    {
        *value = json_object_get_boolean(switch_data, switch_on_name);
    }
    else
    {
        ESP_LOGE(TAG, "switchOn property not found in JSON.");
        error = ESP_ERR_NOT_FOUND;
        goto exit;
    }
    const char *timeout_name = "timeout";
    if (json_object_has_value_of_type(switch_data, timeout_name, JSONNumber))
    {
        *timeout = (uint32_t)json_object_get_number(switch_data, timeout_name);
        error = ESP_OK;
        goto exit;
    }
    else
    {
        ESP_LOGE(TAG, "timeout property not found in JSON.");
        error = ESP_ERR_NOT_FOUND;
    }
exit:
    json_value_free(root_value);
    return error;
}

esp_err_t json_serializer_serialize(const boiler_controller_state_t *boiler_state, char **serialized_string, size_t *length)
{
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    json_object_set_string(root_object, "id", BOILER_ID);
    json_object_set_boolean(root_object, "switchedOn", boiler_state->is_switched_on);
    json_object_set_number(root_object, "timeout", boiler_state->switch_timeout_millis);
    json_object_set_number(root_object, "lastChangeUtcMillis", boiler_state->last_change_utc_millis);

    *serialized_string = json_serialize_to_string_pretty(root_value);
    if (*serialized_string == NULL) return ESP_FAIL;
    *length = strlen(*serialized_string);
    json_value_free(root_value);
    return ESP_OK;
}

void json_serializer_free(char *serialized_string)
{
    json_free_serialized_string(serialized_string);
}
