#ifndef POMELO_WEBRTC_SOCKET_PLUGIN_H
#define POMELO_WEBRTC_SOCKET_PLUGIN_H
#include "socket-int.h"
#ifdef __cplusplus
extern "C" {
#endif



/* -------------------------------------------------------------------------- */
/*                           Plugin implementation                            */
/* -------------------------------------------------------------------------- */

void POMELO_PLUGIN_CALL pomelo_webrtc_socket_plugin_on_listening(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket,
    pomelo_address_t * address
);


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


/// @brief Finalize plugin part of socket
void pomelo_webrtc_socket_plugin_finalize(pomelo_webrtc_socket_t * socket);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_SOCKET_PLUGIN_H
