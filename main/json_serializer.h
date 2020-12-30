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
 * @brief This file contains functions for serialization and deserialization of JSON model data for MQTT and HTTP API.
 */

#ifndef JSON_SERIALIZER_H_
#define JSON_SERIALIZER_H_

#include <stdio.h>

#include "relay_switch.h"

/**
 * Deserialize switching request data from JSON serialized string.
 * @param[in] received_data A pointer to string with JSON payload.
 * @param[out] value A pointer to switch value variable to be set.
 * @param[out] timeout A pointer to timeout variable to be set.
 * @return Return ESP_OK if succeeded.
 */
esp_err_t json_serializer_deserialize(const char *received_data, bool *value, uint32_t *timeout);

/**
 * Serialize data about current switch state to JSON. Output serialized string must be freed when it is not needed anymore.
 * @param[in] switch_state A pointer to switch state dtaa to be serialized.
 * @param[out] serialized_string A pointer to string valiable for setting serialized string.
 * @param[out] length A pointer to variable with serialized string length to be set.
 * @return Return ESP_OK if succeeded.
 */
esp_err_t json_serializer_serialize(const relay_switch_state_t *switch_state, char **serialized_string, size_t *length);

/**
 * Free allocated string with serialized JSON data.
 * @param[in] serialized_string A pointer to string with serialized data to be freed.
 */
void json_serializer_free(char *serialized_string);

#endif /* JSON_SERIALIZER_H_ */
