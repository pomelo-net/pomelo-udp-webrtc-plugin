#ifndef POMELO_PLUGIN_WEBRTC_CHANNEL_H
#define POMELO_PLUGIN_WEBRTC_CHANNEL_H
#include "plugin.h"
#include "rtc-api/rtc-api.h"
#include "base/ref.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POMELO_WEBRTC_CHANNEL_SYSTEM_INDEX SIZE_MAX
#define POMELO_WEBRTC_CHANNEL_FLAG_ACTIVE     (1 << 0)
#define POMELO_WEBRTC_CHANNEL_FLAG_DC_ACTIVE  (1 << 1)
#define POMELO_WEBRTC_CHANNEL_FLAG_DC_RECEIVE (1 << 2)

#define POMELO_SERVER_CHANNEL_PREFIX "server-channel-"
#define POMELO_CLIENT_CHANNEL_PREFIX "client-channel-"
#define POMELO_SYSTEM_CHANNEL_LABEL "system"


struct pomelo_webrtc_channel_info_s {
    /// @brief Containing session
    pomelo_webrtc_session_t * session;

    /// @brief Channel index
    size_t channel_index;

    /// @brief Channel mode
    pomelo_channel_mode channel_mode;
};


struct pomelo_webrtc_channel_s {
    /// @brief Reference
    pomelo_reference_t ref;

    // @brief Context
    pomelo_webrtc_context_t * context;

    /// @brief Flag of channel
    uint8_t flags;

    /// @brief Channel index
    size_t index;

    /// @brief Containing session
    pomelo_webrtc_session_t * session;

    /// @brief Channel mode
    pomelo_channel_mode mode;

    /// @brief Incoming RTC data channel
    rtc_data_channel_t * incoming_dc;

    /// @brief Outgoing RTC data channel
    rtc_data_channel_t * outgoing_dc;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief On alloc the channel
int pomelo_webrtc_channel_on_alloc(
    pomelo_webrtc_channel_t * channel,
    pomelo_webrtc_context_t * context
);


/// @brief On free the channel
void pomelo_webrtc_channel_on_free(pomelo_webrtc_channel_t * channel);


/// @brief Initialize the channel
int pomelo_webrtc_channel_init(
    pomelo_webrtc_channel_t * channel,
    pomelo_webrtc_channel_info_t * info
);


/// @brief Cleanup the channel
void pomelo_webrtc_channel_cleanup(pomelo_webrtc_channel_t * channel);


/// @brief Close the session
void pomelo_webrtc_channel_close(pomelo_webrtc_channel_t * channel);


/// @brief Enable receiving messages
void pomelo_webrtc_channel_enable_receiving(pomelo_webrtc_channel_t * channel);


/// @brief Send message through this channel
void pomelo_webrtc_channel_send(
    pomelo_webrtc_channel_t * channel,
    const uint8_t * data,
    size_t length
);


/// @brief Set the channel mode
void pomelo_webrtc_channel_set_mode(
    pomelo_webrtc_channel_t * channel,
    pomelo_channel_mode mode
);


/// @brief Set the incoming DC
void pomelo_webrtc_channel_set_incoming_data_channel(
    pomelo_webrtc_channel_t * channel,
    rtc_data_channel_t * incoming_dc
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLUGIN_WEBRTC_CHANNEL_H
