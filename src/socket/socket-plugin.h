#ifndef POMELO_WEBRTC_SOCKET_PLUGIN_H
#define POMELO_WEBRTC_SOCKET_PLUGIN_H
#include "socket-int.h"
#ifdef __cplusplus
extern "C" {
#endif



/* -------------------------------------------------------------------------- */
/*                           Plugin implementation                            */
/* -------------------------------------------------------------------------- */

/// @brief Listening callback
void POMELO_PLUGIN_CALL pomelo_webrtc_socket_plugin_on_listening(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket,
    pomelo_address_t * address
);


/// @brief Connecting callback
void POMELO_PLUGIN_CALL pomelo_webrtc_socket_plugin_on_connecting(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket,
    uint8_t * connect_token
);


/// @brief Stopped callback
void POMELO_PLUGIN_CALL pomelo_webrtc_socket_plugin_on_stopped(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket
);


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Initialize plugin part of socket
int pomelo_webrtc_socket_plugin_init(
    pomelo_webrtc_socket_t * socket,
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket
);


/// @brief Cleanup plugin part of socket
void pomelo_webrtc_socket_plugin_cleanup(pomelo_webrtc_socket_t * socket);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Process when native socket is started
void pomelo_webrtc_socket_plugin_on_started(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_SOCKET_PLUGIN_H
