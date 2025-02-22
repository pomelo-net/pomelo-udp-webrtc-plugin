#ifndef POMELO_WEBRTC_SOCKET_INT_H
#define POMELO_WEBRTC_SOCKET_INT_H
#include "socket.h"
#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*                               Module APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Create new session
pomelo_webrtc_session_t * pomelo_webrtc_socket_create_session(
    pomelo_webrtc_socket_t * socket,
    rtc_websocket_client_t * ws_client
);


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Destroy a WebRTC socket
void pomelo_webrtc_socket_destroy(pomelo_webrtc_socket_t * socket);


#ifdef __cplusplus
}
#endif
#endif //POMELO_WEBRTC_SOCKET_INT_H
