#ifndef POMELO_WEBRTC_CHANNEL_INT_H
#define POMELO_WEBRTC_CHANNEL_INT_H
#include "channel.h"
#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                               Module APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Increase reference counter of channel
void pomelo_webrtc_channel_ref(pomelo_webrtc_channel_t * channel);

/// @brief Decrease reference counter of channel
void pomelo_webrtc_channel_unref(pomelo_webrtc_channel_t * channel);

/// @brief Send a message through this channel
void pomelo_webrtc_channel_send_buffer(
    pomelo_webrtc_channel_t * channel,
    rtc_buffer_t * buffer
);

/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Delete the session
void pomelo_webrtc_channel_destroy(
    pomelo_webrtc_channel_t * channel
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_CHANNEL_INT_H
