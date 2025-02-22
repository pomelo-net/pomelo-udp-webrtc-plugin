#ifndef POMELO_PLUGIN_WEBRTC_CONTEXT_H
#define POMELO_PLUGIN_WEBRTC_CONTEXT_H
#include "uv.h"
#include "plugin.h"
#include "utils/map.h"
#include "utils/pool.h"
#include "utils/list.h"
#include "utils/atomic.h"
#include "utils/string-buffer.h"
#include "rtc-api/rtc-api.h"


/// Maximum number of arguments of one task
#define POMELO_WEBRTC_TASK_MAX_ARGS 10

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_webrtc_context_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief Pointer to plugin
    pomelo_plugin_t * plugin;

    /// @brief RTC Context
    rtc_context_t * rtc_context;

    /// @brief Socket map of private data
    pomelo_map_t * socket_map;

    /// @brief Number of running sockets
    pomelo_atomic_uint64_t running_sockets;

    /// @brief Pool of string buffers
    pomelo_pool_t * string_buffer_pool;

    /// @brief Pool of connect token buffers
    pomelo_pool_t * connect_token_pool;

    /// @brief Pool of sockets
    pomelo_pool_t * socket_pool;

    /// @brief Pool of sessions
    pomelo_pool_t * session_pool;

    /// @brief Pool of channels
    pomelo_pool_t * channel_pool;

    /// @brief Running flag of thread
    pomelo_atomic_int64_t thread_running;

    /// @brief Working thread of this plugin
    uv_thread_t thread;

    /// @brief Event loop of this plugin
    uv_loop_t event_loop;

    /// @brief Task async
    uv_async_t async_task;

    /// @brief Shutdown async
    uv_async_t async_shutdown;

    /// @brief Tasks to execute
    pomelo_list_t * tasks;

    /// @brief Pool of async tasks
    pomelo_pool_t * async_tasks_pool;

    /// @brief Pool of scheduled tasks
    pomelo_pool_t * scheduled_tasks_pool;

    /// @brief Pool of worker tasks
    pomelo_pool_t * worker_tasks_pool;

    /// @brief Pool of received commands
    pomelo_pool_t * recv_command_pool;
};


struct pomelo_webrtc_task_s {
    /// @brief Callback function
    pomelo_webrtc_task_cb callback;

    /// @brief Number of arguments
    size_t argc;

    /// @brief Array of arguments
    pomelo_webrtc_variant_t args[POMELO_WEBRTC_TASK_MAX_ARGS];
};


struct pomelo_webrtc_scheduled_task_s {
    /// @brief Base task
    pomelo_webrtc_task_t base;

    /// @brief The timer for this task (For scheduling)
    uv_timer_t timer;
};


struct pomelo_webrtc_worker_task_s {
    /// @brief Base task
    pomelo_webrtc_task_t base;

    /// @brief Task to work
    pomelo_webrtc_task_cb work_fn;

    /// @brief UV work
    uv_work_t work;
};


struct pomelo_webrtc_recv_command_s {
    /// @brief RTC message
    rtc_buffer_t * message;

    /// @brief Session
    pomelo_session_t * native_session;

    /// @brief Channel
    pomelo_webrtc_channel_t * channel;
};

/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Create new context for webrtc plugin
pomelo_webrtc_context_t * pomelo_webrtc_context_create(
    pomelo_allocator_t * allocator,
    pomelo_plugin_t * plugin
);


/// @brief Destroy the context of webrtc plugin
void pomelo_webrtc_context_destroy(pomelo_webrtc_context_t * context);


/// @brief Submit task to run in main event loop
pomelo_webrtc_task_t * pomelo_webrtc_context_submit_task(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_task_cb callback,
    size_t argc,
    pomelo_webrtc_variant_t * args
);


/// @brief Schedule a task to run periodically. This function is non-threadsafe
pomelo_webrtc_task_t * pomelo_webrtc_context_schedule_task(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_task_cb callback,
    size_t argc,
    pomelo_webrtc_variant_t * args,
    uint64_t interval_ms
);


/// @brief Unschedule a scheduled task. This function is non-threadsafe
void pomelo_webrtc_context_unschedule_task(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_task_t * task
);


/// @brief Spawn a task to run in worker thread. After complete, call the
/// provided callback in main thread
pomelo_webrtc_task_t * pomelo_webrtc_context_spawn_task(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_task_cb work,
    pomelo_webrtc_task_cb callback,
    size_t argc,
    pomelo_webrtc_variant_t * args
);


/// @brief Acquire a string buffer from pool
pomelo_string_buffer_t * pomelo_webrtc_context_acquire_string_buffer(
    pomelo_webrtc_context_t * context
);


/// @brief Release a string buffer to pool
void pomelo_webrtc_context_release_string_buffer(
    pomelo_webrtc_context_t * context,
    pomelo_string_buffer_t * buffer
);


/// @brief Acquire a socket from pool
pomelo_webrtc_socket_t * pomelo_webrtc_context_acquire_socket(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_socket_info_t * info
);


/// @brief Release a socket to pool
void pomelo_webrtc_context_release_socket(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_socket_t * socket
);


/// @brief Acquire a session from pool
pomelo_webrtc_session_t * pomelo_webrtc_context_acquire_session(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_session_info_t * info
);


/// @brief Release a session to pool
void pomelo_webrtc_context_release_session(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_session_t * session
);


/// @brief Acquire a channel from pool
pomelo_webrtc_channel_t * pomelo_webrtc_context_acquire_channel(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_channel_info_t * info
);


/// @brief Release a channel to pool
void pomelo_webrtc_context_release_channel(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_channel_t * channel
);


/* -------------------------------------------------------------------------- */
/*                              Private APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Thread entry of this plugin
void pomelo_webrtc_thread_entry(pomelo_webrtc_context_t * context);


/// @brief Shutdown main thread
void pomelo_webrtc_thread_shutdown(pomelo_webrtc_context_t * context);


/// @brief Async tasks handler
void pomelo_webrtc_async_task_callback(uv_async_t * async);


/// @brief Async shutdown handler
void pomelo_webrtc_async_shutdown_callback(uv_async_t * async);


/// @brief Timer callback
void pomelo_webrtc_timer_callback(uv_timer_t * timer);


/// @brief Worker task process
void pomelo_webrtc_worker_task_process(uv_work_t * work);


/// @brief Worker task callback
void pomelo_webrtc_worker_task_callback(uv_work_t * work, int status);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLUGIN_WEBRTC_CONTEXT_H
