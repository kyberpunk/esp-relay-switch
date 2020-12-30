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

#ifndef MAIN_USER_CONFIG_H_
#define MAIN_USER_CONFIG_H_

/**
 * Wi-Fi SSID
 */
#ifndef WIFI_SSID
#define WIFI_SSID "wifi"
#endif

/**
 * Wi-Fi password
 */
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "password"
#endif

/**
 * GPIO pin used as signal for relay. It must be configurable as output.
 */
#ifndef RELAY_GPIO_NUM
#define RELAY_GPIO_NUM 4
#endif

/**
 * Set to 1 to enable HTML web interface or 0 to disable.
 */
#ifndef HTTP_HTML_ENABLE
#define HTTP_HTML_ENABLE 1
#endif

/**
 * Set to 1 to enable HTTP API or 0 to disable.
 */
#ifndef HTTP_JSON_ENABLE
#define HTTP_JSON_ENABLE 1
#endif

/**
 * Set to 1 to enable MQTT interface or 0 to disable.
 */
#ifndef MQTT_ADAPTER_ENABLE
#define MQTT_ADAPTER_ENABLE 1
#endif

/**
 * Set to 1 if relay is connected by high input or 0 otherwise.
 */
#ifndef HIGH_ON
#define HIGH_ON 0
#endif

/**
 * IP address or DNS name of MQTT broker.
 */
#ifndef MQTT_BROKER_HOST
#define MQTT_BROKER_HOST "192.168.100.46"
#endif

/**
 * Unique device ID - important for MQTT.
 */
#ifndef SWITCH_ID
#define SWITCH_ID "SWITCH1"
#endif

/**
 * NTP server DNS name or IP.
 */
#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

#endif /* MAIN_USER_CONFIG_H_ */
