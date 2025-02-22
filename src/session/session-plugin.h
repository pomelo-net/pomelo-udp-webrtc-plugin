#ifndef POMELO_WEBRTC_SESSION_PLUGIN_H
#define POMELO_WEBRTC_SESSION_PLUGIN_H
#include "session-int.h"
#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                           Plugin implementation                            */
/* -------------------------------------------------------------------------- */

void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket,
    pomelo_session_t * native_session,
    void * callback_data,
    pomelo_plugin_error_t error
);


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_disconnect(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session
);


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_get_rtt(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    uint64_t * mean,
    uint64_t * variance
);


int POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_set_mode(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    size_t channel_index,
    pomelo_channel_mode channel_mode
);

/* -------------------------------------------------------------------------- */
/*                                 Public APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize plugin part of session
int pomelo_webrtc_session_plugin_init(pomelo_webrtc_session_t * session);

/// @brief Finalize plugin part of session
void pomelo_webrtc_session_plugin_finalize(pomelo_webrtc_session_t * session);

/// @brief Close plugin session. This will emit disconnected event on native
void pomelo_webrtc_session_plugin_close(pomelo_webrtc_session_t * session);

/// @brief Open plugin session. This will emit connected event on native
void pomelo_webrtc_session_plugin_open(
    pomelo_webrtc_session_t * session,
    pomelo_address_t * address
);

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize session with native session
void pomelo_webrtc_session_plugin_on_created(
    pomelo_webrtc_session_t * session,
    pomelo_session_t * native_session
);

/// @brief Process disconnect request from native side
void pomelo_webrtc_session_plugin_process_disconnect(
    pomelo_webrtc_session_t * session
);

/// @brief Destroy the native session
void pomelo_webrtc_session_plugin_destroy_native_session(
    pomelo_webrtc_session_t * session
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_SESSION_PLUGIN_H
