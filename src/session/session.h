#ifndef POMELO_PLUGIN_WEBRTC_SESSION_H
#define POMELO_PLUGIN_WEBRTC_SESSION_H
#include "rtc-api/rtc-api.h"
#include "plugin.h"
#include "base/ref.h"
#include "utils/list.h"
#include "utils/array.h"
#include "utils/mutex.h"
#include "utils/rtt.h"
#ifdef __cplusplus
extern "C" {
#endif


#define POMELO_WEBRTC_SESSION_FLAG_ACTIVE                (1 << 0)
#define POMELO_WEBRTC_SESSION_FLAG_WS_ACTIVE             (1 << 1)
#define POMELO_WEBRTC_SESSION_FLAG_WS_AUTHENTICATED      (1 << 2)
#define POMELO_WEBRTC_SESSION_FLAG_PC_ACTIVE             (1 << 3)
#define POMELO_WEBRTC_SESSION_FLAG_READY_SIGNAL_RECEIVED (1 << 4)
#define POMELO_WEBRTC_SESSION_FLAG_ALL_CHANNELS_OPENED   (1 << 5)
#define POMELO_WEBRTC_SESSION_FLAG_CONNECTED (                                 \
    POMELO_WEBRTC_SESSION_FLAG_READY_SIGNAL_RECEIVED |                         \
    POMELO_WEBRTC_SESSION_FLAG_ALL_CHANNELS_OPENED                             \
)


struct pomelo_webrtc_session_info_s {
    /// @brief Socket
    pomelo_webrtc_socket_t * socket;

    /// @brief WebSocket client
    rtc_websocket_client_t * ws_client;
};


struct pomelo_webrtc_session_s {
    /// @brief Reference
    pomelo_reference_t ref;

    // @brief Context
    pomelo_webrtc_context_t * context;

    /// @brief Flags of session
    uint8_t flags;

    /// @brief Associated socket
    pomelo_webrtc_socket_t * socket;

    /// @brief Data channels of this session. (exclude system channel)
    pomelo_array_t * channels;

    /// @brief The system channels
    pomelo_webrtc_channel_t * system_channel;
    
    /// @brief Position of this session in sessions list
    pomelo_list_entry_t * list_entry;

    /// @brief The number of opened channels
    size_t opened_channels;
    
    /// @brief Pinging task
    pomelo_webrtc_task_t * ping_task;

    /// @brief Round trip time calculator
    pomelo_rtt_calculator_t rtt;

    /// @brief Native session
    pomelo_session_t * native_session;

    /// @brief Client ID
    int64_t client_id;

    /// @brief The address of this session
    pomelo_address_t address;

    /// @brief RTC peer connection
    rtc_peer_connection_t * pc;

    /// @brief WebSocket client
    rtc_websocket_client_t * ws_client;

    /// @brief Connect timeout
    pomelo_webrtc_task_t * task_timeout;
};


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


/// @brief On alloc the session
int pomelo_webrtc_session_on_alloc(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_context_t * context
);


/// @brief On free the session
void pomelo_webrtc_session_on_free(pomelo_webrtc_session_t * session);


/// @brief New session
int pomelo_webrtc_session_init(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_session_info_t * info
);


/// @brief Cleanup the session
void pomelo_webrtc_session_cleanup(pomelo_webrtc_session_t * session);


/// @brief Close the connection
void pomelo_webrtc_session_close(pomelo_webrtc_session_t * session);


/// @brief Remove a channel when it is going to be deleted
void pomelo_webrtc_session_remove_channel(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_channel_t * channel
);


/// @brief Process when a data channel of this session has opened
void pomelo_webrtc_session_on_channel_opened(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_channel_t * channel
);


/// @brief Process the system message
void pomelo_webrtc_session_process_system_message(
    pomelo_webrtc_session_t * session,
    rtc_buffer_t * message,
    uint64_t recv_time
);


/// @brief Increase reference counter of session
void pomelo_webrtc_session_ref(pomelo_webrtc_session_t * session);


/// @brief Decrease reference counter of session
void pomelo_webrtc_session_unref(pomelo_webrtc_session_t * session);


#ifdef __cplusplus
}
#endif
#endif // ~POMELO_PLUGIN_WEBRTC_SESSION_H
