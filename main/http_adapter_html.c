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
#include <esp_http_server.h>
#include <time.h>
#include <esp_log.h>
#include <esp_err.h>

#include "http_adapter_html.h"
#include "relay_switch.h"

#define DEFAULT_HTML "<!DOCTYPE html>\n" \
"<html>\n" \
"<head>\n" \
"<title>\n" \
"Relay Switch\n" \
"</title>\n" \
"</head>\n" \
"<body>\n" \
"\n" \
"<h2>Relay Switch</h2>\n" \
"<p>\n" \
"Current switch state: %s<br/>\n" \
"Last change: %s\n" \
"Switch timeout: %d ms\n" \
"</p>\n" \
"\n" \
"<form action=\"/state\" method=\"POST\">\n" \
"  <input type=\"hidden\" name=\"switch_on\" value=\"%s\">\n" \
"  Timeout (ms): <input type=\"number\" name=\"timeout\" value=\"0\"><br/>\n" \
"  <input type=\"submit\" value=\"%s\">\n" \
"</form>\n" \
"\n" \
"</body>\n" \
"</html>"

#define POST_SUCCESS_RESPONSE_HTML "<!DOCTYPE html>\n" \
"<html>\n" \
"<head>\n" \
"<title>\n" \
"Success\n" \
"</title>\n" \
"</head>\n" \
"<body>\n" \
"\n" \
"<p>\n" \
"Action successfully executed!\n" \
"</p>\n" \
"\n" \
"<form action=\"/\" method=\"GET\">\n" \
"  <input type=\"submit\" value=\"Back\">\n" \
"</form>\n" \
"\n" \
"</body>\n" \
"</html>"

#define POST_ERROR_RESPONSE_HTML "<!DOCTYPE html>\n" \
"<html>\n" \
"<head>\n" \
"<title>\n" \
"Success\n" \
"</title>\n" \
"</head>\n" \
"<body>\n" \
"\n" \
"<p>\n" \
"Action failed!\n" \
"</p>\n" \
"\n" \
"<form action=\"/\" method=\"GET\">\n" \
"  <input type=\"submit\" value=\"Back\">\n" \
"</form>\n" \
"\n" \
"</body>\n" \
"</html>"

#define TAG "http_adapter_plain"

static const char* true_string = "true";
static const char* false_string = "false";
static const char* on_string = "on";
static const char* off_string = "off";
static const char* switch_on_string = "Switch on";
static const char* switch_off_string = "Switch off";

static const char* get_string_from_bool(bool value)
{
    return value ? true_string : false_string;
}

static const char* get_on_off_string_from_bool(bool value)
{
    return value ? on_string : off_string;
}

static bool get_bool_from_string(const char *value)
{
    if (strcmp(value, true_string) == 0)
    {
        return true;
    }
    return false;
}

static uint32_t get_uint_from_string(const char *value)
{
    uint32_t result = 0;
    if (sscanf(value, "%u", &result) != 1)
    {
        return 0;
    }
    return result;
}

static esp_err_t send_get_response(httpd_req_t *req, relay_switch_state_t switch_state)
{
    const char *is_switched_on_string = get_on_off_string_from_bool(switch_state.is_switched_on);
    const char *form_action_value = get_string_from_bool(!switch_state.is_switched_on);
    time_t epoch = switch_state.last_change_utc_millis / 1000;
    const char *formated_time_string = asctime(gmtime (&epoch));
    const char *submit_string = switch_state.is_switched_on ? switch_off_string : switch_on_string;

    size_t buf_len = strlen(DEFAULT_HTML) + strlen(is_switched_on_string) + strlen(form_action_value)
        + strlen(formated_time_string) + 1;
    char *resp = malloc(buf_len * sizeof(char));
    if (resp == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    size_t resp_len = snprintf(resp, buf_len, DEFAULT_HTML, is_switched_on_string, formated_time_string, switch_state.switch_timeout_millis, form_action_value, submit_string);
    httpd_resp_send(req, resp, resp_len);
    return ESP_OK;
}

static esp_err_t get_handler(httpd_req_t *req)
{
    relay_switch_state_t switch_state = relay_switch_get_state();
    send_get_response(req, switch_state);
    return ESP_OK;
}

static esp_err_t parse_switch_payload(httpd_req_t *req, bool *value, uint32_t *timeout)
{
    esp_err_t error = ESP_FAIL;
    size_t buf_len = req->content_len + 1;
    if (buf_len > 1)
    {
        char* buf = malloc(buf_len * sizeof(char));
        memset(buf, 0, buf_len);
        httpd_req_recv(req, buf, buf_len);
        ESP_LOGI(TAG, "/state URI called. Found query: %s", buf);
        char param[6];
        error = httpd_query_key_value(buf, "switch_on", param, sizeof(param));
        if (error == ESP_OK)
        {
            ESP_LOGI(TAG, "switch_on parameter: %s", param);
            bool switch_on = get_bool_from_string(param);
            *value = switch_on;
        }
        else
        {
            ESP_LOGW(TAG, "Failed to get switch_on value.");
        }
        char timeout_param[11];
        error = httpd_query_key_value(buf, "timeout", timeout_param, sizeof(timeout_param));
        if (error == ESP_OK)
        {
            ESP_LOGI(TAG, "timeout paramter: %s", timeout_param);
            *timeout = get_uint_from_string(timeout_param);
        }
        else
        {
            ESP_LOGI(TAG, "Failed to get timeout. Set to 0.");
            *timeout = 0;
        }
        free(buf);
    }
    else
    {
        ESP_LOGW(TAG, "Query is empty.");
    }
    return error;
}

static esp_err_t post_handler(httpd_req_t *req)
{
    bool switch_on;
    uint32_t timeout;
    esp_err_t error = parse_switch_payload(req, &switch_on, &timeout);
    const char* resp;
    if (error == ESP_OK)
    {
        relay_switch_set_state(switch_on, timeout);
        resp = POST_SUCCESS_RESPONSE_HTML;
    }
    else
    {
        const char* error_string = esp_err_to_name(error);
        ESP_LOGW(TAG, "Request parse failed: %s", error_string);
        resp = POST_ERROR_RESPONSE_HTML;
    }
    httpd_resp_send(req, resp, strlen(resp));
    return error;
}

esp_err_t http_adapter_html_init(httpd_handle_t* server)
{
    httpd_uri_t uri_get =
    {
        .uri = "/",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t uri_post =
    {
        .uri = "/state",
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
