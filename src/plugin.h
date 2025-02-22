#ifndef POMELO_PLUGIN_WEBRTC_H
#define POMELO_PLUGIN_WEBRTC_H
#include <stddef.h>

#include "pomelo/plugin.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Task variant type
typedef union pomelo_webrtc_variant_u {
    void * ptr;
    bool b;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    size_t size;
    pomelo_address_t address;
} pomelo_webrtc_variant_t;


/// @brief Task callback
typedef void (*pomelo_webrtc_task_cb)(
    size_t argc,
    pomelo_webrtc_variant_t * args
);


/// @brief The WebRTC plugin
typedef struct pomelo_webrtc_context_s pomelo_webrtc_context_t;

/// @brief Store the information about the session of webrtc plugin
typedef struct pomelo_webrtc_session_s pomelo_webrtc_session_t;

/// @brief Store the information about the socket of webrtc plugin
typedef struct pomelo_webrtc_socket_s pomelo_webrtc_socket_t;

/// @brief Store the information about PC channel
typedef struct pomelo_webrtc_channel_s pomelo_webrtc_channel_t;

/// @brief A single task
typedef struct pomelo_webrtc_task_s pomelo_webrtc_task_t;

/// @brief Scheduled task
typedef struct pomelo_webrtc_scheduled_task_s pomelo_webrtc_scheduled_task_t;

/// @brief Worker task
typedef struct pomelo_webrtc_worker_task_s pomelo_webrtc_worker_task_t;

/// @brief A single received command
typedef struct pomelo_webrtc_recv_command_s pomelo_webrtc_recv_command_t;

/// @brief Information about the channel
typedef struct pomelo_webrtc_channel_info_s pomelo_webrtc_channel_info_t;

/// @brief Information about the session
typedef struct pomelo_webrtc_session_info_s pomelo_webrtc_session_info_t;

/// @brief Information about the socket
typedef struct pomelo_webrtc_socket_info_s pomelo_webrtc_socket_info_t;


/* -------------------------------------------------------------------------- */
/*                               PLugin APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Entry of plugin
void pomelo_webrtc_entry(pomelo_plugin_t * plugin);


/// @brief Unload handler for plugin
void POMELO_PLUGIN_CALL pomelo_webrtc_on_unload(pomelo_plugin_t * plugin);


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Log a message
void pomelo_webrtc_log(const char * format, ...);

/// @brief Log a message in debugging mode
void pomelo_webrtc_log_debug(const char * format, ...);

#ifdef __cplusplus
}
#endif
#endif // ~POMELO_PLUGIN_WEBRTC_H
