#ifndef POMELO_PLUGIN_WEBRTC_SOCKET_H
#define POMELO_PLUGIN_WEBRTC_SOCKET_H
#include <stdbool.h>
#include "rtc-api/rtc-api.h"
#include "plugin.h"
#include "base/ref.h"
#include "utils/list.h"
#include "utils/array.h"


#ifdef __cplusplus
extern "C" {
#endif

#define POMELO_WEBRTC_SOCKET_FLAG_ACTIVE     (1 << 0)
#define POMELO_WEBRTC_SOCKET_FLAG_WSS_ACTIVE (1 << 1)


struct pomelo_webrtc_socket_s {
    /// @brief Reference of this socket
    pomelo_reference_t ref;

    // @brief Context
    pomelo_webrtc_context_t * context;

    /// @brief Flags of socket
    uint8_t flags;

    /// @brief All the channels
    pomelo_array_t * channel_modes;

    /// @brief All sessions
    pomelo_list_t * sessions;

    /// @brief Native socket
    pomelo_socket_t * native_socket;

    /// @brief Websocker server
    rtc_websocket_server_t * ws_server;
};

/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Create new WebRTC socket
pomelo_webrtc_socket_t * pomelo_webrtc_socket_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket,
    pomelo_address_t * address
);

/// @brief Increase reference count of socket
void pomelo_webrtc_socket_ref(pomelo_webrtc_socket_t * socket);

/// @brief Decrease reference count of socket
void pomelo_webrtc_socket_unref(pomelo_webrtc_socket_t * socket);

/// @brief Close the socket
void pomelo_webrtc_socket_close(pomelo_webrtc_socket_t * socket);

/// @brief Remove a session from controlling list
void pomelo_webrtc_socket_remove_session(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_session_t * session
);

#ifdef __cplusplus
}
#endif
#endif // ~POMELO_PLUGIN_WEBRTC_SOCKET_H
