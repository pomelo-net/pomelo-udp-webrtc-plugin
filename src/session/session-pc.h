#ifndef POMELO_WERBTC_SESSION_PC_H
#define POMELO_WERBTC_SESSION_PC_H
#include "session-int.h"
#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                                 PC Callbacks                               */
/* -------------------------------------------------------------------------- */

/// @brief Handle PC local candidate
void pomelo_webrtc_pc_on_local_candidate(
    rtc_peer_connection_t * pc,
    rtc_buffer_t * cand,
    rtc_buffer_t * mid
);

/// @brief Handle PC state changed
void pomelo_webrtc_pc_on_state_changed(
    rtc_peer_connection_t * pc,
    rtc_peer_connection_state state
);

/// @brief Handle new data channel
void pomelo_webrtc_pc_on_data_channel(
    rtc_peer_connection_t * pc,
    rtc_data_channel_t * dc
);


/* -------------------------------------------------------------------------- */
/*                                 Public APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize peer connection part of session
int pomelo_webrtc_session_pc_init(pomelo_webrtc_session_t * session);

/// @brief Finalize peer connection part of session
void pomelo_webrtc_session_pc_finalize(pomelo_webrtc_session_t * session);

/// @brief Close the peer connection
void pomelo_webrtc_session_pc_close(pomelo_webrtc_session_t * session);

/// @brief Get the address
int pomelo_webrtc_session_pc_address(
    pomelo_webrtc_session_t * session,
    pomelo_address_t * address
);

/// @brief Start negotiating
void pomelo_webrtc_session_pc_negotiate(pomelo_webrtc_session_t * session);

/// @brief Set remote description
void pomelo_webrtc_session_pc_set_remote_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
);

/// @brief Add remote candidate
void pomelo_webrtc_session_pc_add_remote_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
);


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize RTC configuration
void pomelo_webrtc_session_pc_init_rtc_pc_options(
    rtc_peer_connection_options_t * options
);

/// @brief Handle connected state of rtc
void pomelo_webrtc_session_pc_on_connected(pomelo_webrtc_session_t * session);

/// @brief Process when PC is closed
void pomelo_webrtc_session_pc_on_closed(
    pomelo_webrtc_session_t * session
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WERBTC_SESSION_PC_H
