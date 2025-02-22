#ifndef POMELO_WEBRTC_SOCKET_WSS_H
#define POMELO_WEBRTC_SOCKET_WSS_H
#include "socket-int.h"
#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                        Websocket server callbacks                          */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_socket_wss_on_client(
    rtc_websocket_server_t * ws_server,
    rtc_websocket_client_t * ws_client
);


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Initialize websocket server part of socket
int pomelo_webrtc_socket_wss_init(
    pomelo_webrtc_socket_t * socket,
    pomelo_address_t * address
);


/// @brief Cleanup websocket server part of socket
void pomelo_webrtc_socket_wss_cleanup(pomelo_webrtc_socket_t * socket);


/// @brief Close the websocket server
void pomelo_webrtc_socket_wss_close(pomelo_webrtc_socket_t * socket);


/* -------------------------------------------------------------------------- */
/*                              Private APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Process close WSS in worker thread
void pomelo_webrtc_socket_wss_close_process(pomelo_webrtc_socket_t * socket);

/// @brief The last callback of WSS when it is closed
void pomelo_webrtc_socket_wss_on_closed(pomelo_webrtc_socket_t * socket);

#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_SOCKET_WSS_H
