#ifndef POMELO_WEBRTC_SESSION_INT_H
#define POMELO_WEBRTC_SESSION_INT_H
#include "utils/macro.h"
#include "session.h"
#ifdef __cplusplus
extern "C" {
#endif


#define MESSAGE_SEPARATOR  '|'

#define OPCODE_AUTH        "AUTH"
#define OPCODE_DESCRIPTION "DESC"
#define OPCODE_CANDIDATE   "CAND"
#define OPCODE_READY       "READY"
#define OPCODE_CONNECTED   "CONN"

#define RESULT_AUTH_OK          "AUTH|OK"
#define RESULT_AUTH_FAILED      "AUTH|FAILED"

#define CLOSE_REASON_INTERNAL_ERROR  "CLOSE|INTERNAL_ERROR"
#define CLOSE_REASON_FAILED          "CLOSE|PC_FAILED"
#define CLOSE_REASON_DISCONNECTED    "CLOSE|PC_DISCONNECTED"
#define CLOSE_REASON_CLOSED          "CLOSE|PC_CLOSED"

// Current protocol is only supporting maximum 4 opcodes
#define SYS_OPCODE_PING 0
#define SYS_OPCODE_PONG 1

// Wait for 5 seconds for sending authenticating
#define POMELO_AUTH_TIMEOUT_MS 5000


/* -------------------------------------------------------------------------- */
/*                               Module APIs                                  */
/* -------------------------------------------------------------------------- */

/// Check if session is active
#define pomelo_webrtc_session_is_active(session)                               \
POMELO_CHECK_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_ACTIVE)


/// @brief Process auth result
void pomelo_webrtc_session_on_auth_result(
    pomelo_webrtc_session_t * session,
    pomelo_plugin_token_info_t * info
);


/// @brief Send the local candidate
void pomelo_webrtc_session_send_local_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
);

/// @brief Send the local description
void pomelo_webrtc_session_send_local_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
);

/// @brief Handle remote description
void pomelo_webrtc_session_recv_remote_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
);

/// @brief Handle remote candidate
void pomelo_webrtc_session_recv_remote_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
);

/// @brief Handle ready signal from client
void pomelo_webrtc_session_recv_ready(pomelo_webrtc_session_t * session);

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Delete session
void pomelo_webrtc_session_destroy(pomelo_webrtc_session_t * session);

/// @brief Start sending ping
void pomelo_webrtc_session_start_ping(pomelo_webrtc_session_t * session);

/// @brief Stop sending ping
void pomelo_webrtc_session_stop_ping(pomelo_webrtc_session_t * session);

/// @brief Process when all data channels have opened
void pomelo_webrtc_session_on_all_channels_opened(
    pomelo_webrtc_session_t * session
);

/// @brief Send ping message
void pomelo_webrtc_session_send_ping(pomelo_webrtc_session_t * session);

/// @brief Send pong message
void pomelo_webrtc_session_send_pong(
    pomelo_webrtc_session_t * session,
    uint64_t pong_sequence,
    uint64_t recv_time,
    uint64_t socket_time
);

/// @brief Receive ping message
void pomelo_webrtc_session_recv_pong(
    pomelo_webrtc_session_t * session,
    uint64_t ping_sequence,
    uint64_t recv_time
);

/// @brief Process ping request
void pomelo_webrtc_session_process_ping(
    pomelo_webrtc_session_t * session,
    const uint8_t * message,
    size_t length,
    uint64_t recv_time
);

/// @brief Process ping response
void pomelo_webrtc_session_process_pong(
    pomelo_webrtc_session_t * session,
    const uint8_t * message,
    size_t length,
    uint64_t recv_time
);

/// @brief Create channels
int pomelo_webrtc_session_create_channels(pomelo_webrtc_session_t * session);

/// @brief Process when all data channels are opened and received ready signal
/// from client
void pomelo_webrtc_session_on_ready(pomelo_webrtc_session_t * session);

/// @brief Process when session is ready and native session is created
void pomelo_webrtc_session_on_connected(pomelo_webrtc_session_t * session);

/// @brief Process connect timeout
void pomelo_webrtc_session_on_timeout(
    size_t argc,
    pomelo_webrtc_variant_t * args
);

/// @brief Schedule timeout for specific amount of time
pomelo_webrtc_task_t * pomelo_webrtc_session_schedule_timeout(
    pomelo_webrtc_session_t * session,
    uint64_t timeout_ms
);

/// @brief Unschedule timeout
void pomelo_webrtc_session_unschedule_timeout(
    pomelo_webrtc_session_t * session
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_SESSION_INT_H
