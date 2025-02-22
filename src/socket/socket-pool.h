#ifndef POMELO_WEBRTC_SOCKET_POOL_H
#define POMELO_WEBRTC_SOCKET_POOL_H
#include "socket.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Pool-compatible allocating callback
int pomelo_webrtc_socket_pool_allocate_callback(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_context_t * context
);

/// @brief Pool-compatible deallocating callback
int pomelo_webrtc_socket_pool_deallocate_callback(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_context_t * context
);

/// @brief Acquire callback for socket pool
int pomelo_webrtc_socket_pool_acquire_callback(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_context_t * context
);


#ifdef __cplusplus
}
#endif
#endif //POMELO_WEBRTC_SOCKET_POOL_H
