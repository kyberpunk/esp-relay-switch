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

#include "http_adapter_json.h"
#include "boiler_controller.h"
#include "json_serializer.h"
#include "user_config.h"

#define TAG "http_adapter_json"

static esp_err_t send_get_response(httpd_req_t *req, boiler_controller_state_t boiler_state)
{
    size_t length = 0;
    char *serialized_string = NULL;
    esp_err_t error = json_serializer_serialize(&boiler_state, &serialized_string, &length);
    if (error != ESP_OK) return error;
    ESP_LOGI(TAG, "Response body: %s", serialized_string);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, serialized_string, length);
    json_serializer_free(serialized_string);
    return ESP_OK;
}

static esp_err_t get_handler(httpd_req_t *req)
{
    boiler_controller_state_t boiler_state = boiler_controller_get_state();
    return send_get_response(req, boiler_state);
}

static esp_err_t post_handler(httpd_req_t *req)
{
    esp_err_t error = ESP_FAIL;
    size_t buf_len = req->content_len + 1;
    bool switch_on = false;
    uint32_t timeout = 0;
    boiler_controller_state_t boiler_state;
    if (buf_len > 1)
    {
        char* buf = malloc(buf_len * sizeof(char));
        memset(buf, 0, buf_len);
        httpd_req_recv(req, buf, buf_len);
        ESP_LOGI(TAG, "/api/state URI called. Body:\n%s", buf);
        error = json_serializer_deserialize(buf, &switch_on, &timeout);
        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "JSON deserialization failed.");
            goto exit;
        }
        error = boiler_controller_set_state(switch_on, timeout);
        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "JSON boiler_controller_set_state failed.");
        }
        free(buf);

        boiler_state = boiler_controller_get_state();
        boiler_state.switch_timeout_millis = timeout;
        send_get_response(req, boiler_state);
        return ESP_OK;
    }
    else
    {
        ESP_LOGW(TAG, "Content is empty.");
    }

exit:
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    return error;
}

esp_err_t http_adapter_json_init(httpd_handle_t* server)
{
    httpd_uri_t uri_get =
    {
        .uri = "/api/state",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t uri_post =
    {
        .uri = "/api/state",
        .method = HTTP_POST,
        .handler = post_handler,
        .user_ctx = NULL
    };

    esp_err_t error = ESP_OK;
    error = httpd_register_uri_handler(server, &uri_get);
    if (error != ESP_OK)
        return error;
    return httpd_register_uri_handler(server, &uri_post);
}
