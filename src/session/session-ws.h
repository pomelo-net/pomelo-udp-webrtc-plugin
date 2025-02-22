#ifndef POMELO_WEBRTC_SESSION_WS_H
#define POMELO_WEBRTC_SESSION_WS_H
#include "session-int.h"
#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                                WS Callbacks                                */
/* -------------------------------------------------------------------------- */

/// @brief Handle WSC opened
void pomelo_webrtc_ws_on_open(rtc_websocket_client_t * ws_client);

/// @brief Handle WSC closed
void pomelo_webrtc_ws_on_closed(rtc_websocket_client_t * ws_client);

/// @brief Handle WSC error
void pomelo_webrtc_ws_on_error(
    rtc_websocket_client_t * ws_client,
    const char * error
);

/// @brief Handle WSC message
void pomelo_webrtc_ws_on_message(
    rtc_websocket_client_t * ws_client,
    rtc_buffer_t * message
);


/* -------------------------------------------------------------------------- */
/*                                 Public APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize associated websocket client with session
int pomelo_webrtc_session_ws_init(
    pomelo_webrtc_session_t * session,
    rtc_websocket_client_t * ws_client
);


/// @brief Cleanup associated websocket client with session
void pomelo_webrtc_session_ws_cleanup(pomelo_webrtc_session_t * session);


/// @brief Close WS part of session
void pomelo_webrtc_session_ws_close(pomelo_webrtc_session_t * session);


/// @brief Send description to client
void pomelo_webrtc_session_ws_send_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
);


/// @brief Send candidate to client
void pomelo_webrtc_session_ws_send_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
);


/// @brief Send ready signal
void pomelo_webrtc_session_ws_send_ready(pomelo_webrtc_session_t * session);


/// @brief Send connected signal
void pomelo_webrtc_session_ws_send_connected(pomelo_webrtc_session_t * session);

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Receive authentication information
void pomelo_webrtc_session_ws_recv_auth(
    pomelo_webrtc_session_t * session,
    const char * auth,
    size_t auth_length
);


/// @brief Process auth result
void pomelo_webrtc_session_ws_auth_result(
    pomelo_webrtc_session_t * session,
    pomelo_plugin_token_info_t * info
);


/// @brief Send auth success
void pomelo_webrtc_session_ws_send_auth_success(
    pomelo_webrtc_session_t * session
);


/// @brief Process when received description from client
void pomelo_webrtc_session_ws_recv_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
);


/// @brief Process when receive candiate from client
void pomelo_webrtc_session_ws_recv_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
);


/// @brief Process the message
void pomelo_webrtc_session_ws_process_message(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t size
);


/// @brief Process WebSocket message when session state is authenticating
void pomelo_webrtc_session_ws_on_message_unauthenticated(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t message_length
);


/// @brief Process WebSocket message when session state is either exchanging or
/// connected
void pomelo_webrtc_session_ws_on_message_authenticated(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t message_length
);


/// @brief Process the description message
void pomelo_webrtc_session_ws_process_description_message(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t size
);


/// @brief Process the candidate message
void pomelo_webrtc_session_ws_process_candidate_message(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t size
);


/// @brief Process when WS is closed
void pomelo_webrtc_session_ws_on_closed(
    pomelo_webrtc_session_t * session
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_SESSION_WS_H
