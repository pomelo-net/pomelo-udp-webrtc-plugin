#ifndef POMELO_PLUGIN_WEBRTC_CHANNEL_DC_H
#define POMELO_PLUGIN_WEBRTC_CHANNEL_DC_H
#include "channel-int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*                                 DC Callbacks                               */
/* -------------------------------------------------------------------------- */

/// @brief Handle DC on opened
void pomelo_webrtc_dc_on_open(rtc_data_channel_t * dc);

/// @brief Handle DC on closed
void pomelo_webrtc_dc_on_closed(rtc_data_channel_t * dc);

/// @brief Handle DC on error
void pomelo_webrtc_dc_on_error(
    rtc_data_channel_t * dc,
    const char * error
);

/// @brief Handle DC message
void pomelo_webrtc_dc_on_message(
    rtc_data_channel_t * dc,
    rtc_buffer_t * message
);

/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Initialize DC part of channel
int pomelo_webrtc_channel_dc_init(pomelo_webrtc_channel_t * channel);

/// @brief Finalize DC part of channel
void pomelo_webrtc_channel_dc_finalize(pomelo_webrtc_channel_t * channel);

/// @brief Close DC part of channel
void pomelo_webrtc_channel_dc_close(pomelo_webrtc_channel_t * channel);

/// @brief Enable DC receiving
void pomelo_webrtc_channel_dc_enable_receiving(
    pomelo_webrtc_channel_t * channel
);

/// @brief Set the incoming DC
void pomelo_webrtc_channel_dc_set_incoming_data_channel(
    pomelo_webrtc_channel_t * channel,
    rtc_data_channel_t * incoming_dc
);

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Message callback for data channel
void pomelo_webrtc_channel_dc_process_message(
    pomelo_webrtc_channel_t * channel,
    rtc_buffer_t * message,
    uint64_t recv_time
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PLUGIN_WEBRTC_CHANNEL_DC_H
