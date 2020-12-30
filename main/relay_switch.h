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
 * @brief This file contains function for controlling relay switch state.
 */

#ifndef MAIN_RELAY_SWITCH_H_
#define MAIN_RELAY_SWITCH_H_

#include <stdbool.h>
#include <inttypes.h>

#include <esp_err.h>

/**
 * Switch state data.
 */
typedef struct relay_switch_state
{
	/** Determines if switch switched on or off. When it is true then it is switched on. */
	bool is_switched_on;
	/** Switch timeout in milliseconds. After this timeout switch position will be reverted. When 0 then switch state is permanent. */
	uint32_t switch_timeout_millis;
	/** UTC timestamp in milliseconds of last switch position change. It is set to startup time after device startup. */
	uint64_t last_change_utc_millis;
} relay_switch_state_t;

/**
 * Declaration of function for notifying about switch state changes.
 * @param[in]  relay_switch_state Current switch state.
 * @param[in]  context Context of callback.
 */
typedef void (*state_changed_cb_t)(const relay_switch_state_t* relay_switch_state, void* context);

/**
 * Initialize relay switch. Pin for relay signaling is configured as output. Switch is switched off by default.
 * @return Return ESP_OK if succeeded.
 */
esp_err_t relay_switch_init(void);

/**
 * Change relay switch position.
 * @param[in]  switch_on New switch value. Set to true to switch on.
 * @param[in] Switch timeout in milliseconds. After this timeout switch position will be reverted. When 0 then switch state is permanent.
 * @return Return ESP_OK if succeeded.
 */
esp_err_t relay_switch_set_state(bool switch_on, uint32_t timeout);

/**
 * Get current switch state.
 * @return Return current switch state.
 */
relay_switch_state_t relay_switch_get_state(void);

/**
 * Set callback fuction which will be called when switch state is changed.
 * @param[in] callback Switch changed callback function.
 * @param[in] context Callback context.
 */
void relay_switch_set_state_changed_cb(state_changed_cb_t callback, void* context);

#endif /* MAIN_RELAY_SWITCH_H_ */
