#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "utils/string-buffer.h"
#include "context.h"
#include "session/session.h"
#include "session/session-pool.h"
#include "socket/socket.h"
#include "socket/socket-pool.h"
#include "channel/channel.h"
#include "socket/socket-plugin.h"
#include "socket/socket-wss.h"
#include "session/session-plugin.h"
#include "session/session-ws.h"
#include "session/session-pc.h"
#include "channel/channel-plugin.h"
#include "channel/channel-dc.h"
#include "channel/channel-pool.h"


#define POMELO_PLUGIN_WEBRTC_LOG_LEVEL RTC_LOG_LEVEL_DEBUG
#define DATE_TIME_BUFFER_LENGTH 30


static void rtc_log_handler(rtc_log_level level, const char * message) {
    if (level >= POMELO_PLUGIN_WEBRTC_LOG_LEVEL) {
        time_t tm = time(NULL);
        char datetime_buffer[DATE_TIME_BUFFER_LENGTH];
        strftime(
            datetime_buffer,
            DATE_TIME_BUFFER_LENGTH,
            "%c GMT",
            gmtime(&tm)
        );
        pomelo_webrtc_log("%10s [L%d] - %s\n", datetime_buffer, (int) level, message);
    }
}


pomelo_webrtc_context_t * pomelo_webrtc_context_create(
    pomelo_allocator_t * allocator,
    pomelo_plugin_t * plugin
) {
    assert(allocator != NULL);
    pomelo_webrtc_context_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_webrtc_context_t);
    if (!context) {
        // Failed to create instance
        return NULL;
    }
    memset(context, 0, sizeof(pomelo_webrtc_context_t));
    context->plugin = plugin;

    // Initialize rtc
    rtc_options_t options;
    memset(&options, 0, sizeof(rtc_options_t));
    options.log_level = POMELO_PLUGIN_WEBRTC_LOG_LEVEL;
    options.log_callback = rtc_log_handler;

    // Set Websocket callbacks
    options.wss_client_callback = pomelo_webrtc_socket_wss_on_client;
    options.ws_open_callback = pomelo_webrtc_ws_on_open;
    options.ws_closed_callback = pomelo_webrtc_ws_on_closed;
    options.ws_error_callback = pomelo_webrtc_ws_on_error;
    options.ws_message_callback = pomelo_webrtc_ws_on_message;

    // Set Peer connection callbacks
    options.pc_local_candidate_callback = pomelo_webrtc_pc_on_local_candidate;
    options.pc_state_change_callback = pomelo_webrtc_pc_on_state_changed;
    options.pc_data_channel_callback = pomelo_webrtc_pc_on_data_channel;

    // Set Data channel callbacks
    options.dc_open_callback = pomelo_webrtc_dc_on_open;
    options.dc_closed_callback = pomelo_webrtc_dc_on_closed;
    options.dc_error_callback = pomelo_webrtc_dc_on_error;
    options.dc_message_callback = pomelo_webrtc_dc_on_message;

    context->rtc_context = rtc_context_create(&options);
    if (!context->rtc_context) {
        // Failed to create RTC context
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }
    rtc_context_set_data(context->rtc_context, context);

    // Configure the plugin
    plugin->configure_callbacks(
        plugin,
        pomelo_webrtc_on_unload,
        /* socket_on_create = */ NULL,
        /* socket_on_destroyed = */ NULL,
        pomelo_webrtc_socket_plugin_on_listening,
        /* socket_on_connecting = */ NULL,
        pomelo_webrtc_socket_plugin_on_stopped,
        pomelo_webrtc_plugin_session_create,
        pomelo_webrtc_plugin_session_receive,
        pomelo_webrtc_plugin_session_disconnect,
        pomelo_webrtc_plugin_session_get_rtt,
        pomelo_webrtc_plugin_session_set_mode,
        pomelo_webrtc_plugin_session_send
    );

    // Create socket private map
    pomelo_map_options_t map_options;
    pomelo_map_options_init(&map_options);

    map_options.allocator = allocator;
    map_options.key_size = sizeof(pomelo_socket_t *);
    map_options.value_size = sizeof(void *);

    context->socket_map = pomelo_map_create(&map_options);
    if (!context->socket_map) {
        // Failed to create socket map
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Create string buffers pool
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_string_buffer_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_string_buffer_initialize;
    pool_options.deallocate_callback = (pomelo_pool_callback)
        pomelo_string_buffer_finalize;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_string_buffer_reset;
    pool_options.callback_context = allocator;

    context->string_buffer_pool = pomelo_pool_create(&pool_options);
    if (!context->string_buffer_pool) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Create connect tokens pool
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = POMELO_CONNECT_TOKEN_BYTES;
    pool_options.zero_initialized = true;
    context->connect_token_pool = pomelo_pool_create(&pool_options);
    if (!context->connect_token_pool) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }


    // Create sockets pool
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_webrtc_socket_t);
    pool_options.callback_context = context;
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_webrtc_socket_pool_allocate_callback;
    pool_options.deallocate_callback = (pomelo_pool_callback)
        pomelo_webrtc_socket_pool_deallocate_callback;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_webrtc_socket_pool_acquire_callback;
    
    context->socket_pool = pomelo_pool_create(&pool_options);
    if (!context->socket_pool) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Create sessions pool
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_webrtc_session_t);
    pool_options.callback_context = context;
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_webrtc_session_alloc_callback;
    pool_options.deallocate_callback = (pomelo_pool_callback)
        pomelo_webrtc_session_dealloc_callback;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_webrtc_session_acquire_callback;
    context->session_pool = pomelo_pool_create(&pool_options);
    if (!context->session_pool) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Create channels pool
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_webrtc_channel_t);
    pool_options.zero_initialized = true;
    pool_options.callback_context = context;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_webrtc_channel_acquire_callback;
    context->channel_pool = pomelo_pool_create(&pool_options);
    if (!context->channel_pool) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Create list of tasks
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_webrtc_task_t *);
    list_options.synchronized = true;
    context->tasks = pomelo_list_create(&list_options);
    if (!context->tasks) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Create pool of async tasks
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_webrtc_task_t);
    pool_options.synchronized = true;
    pool_options.zero_initialized = true;
    context->async_tasks_pool = pomelo_pool_create(&pool_options);
    if (!context->async_tasks_pool) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Create pool of scheduled tasks
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_webrtc_scheduled_task_t);
    pool_options.zero_initialized = true;
    context->scheduled_tasks_pool = pomelo_pool_create(&pool_options);
    if (!context->scheduled_tasks_pool) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Create pool of worker task
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_webrtc_worker_task_t);
    pool_options.zero_initialized = true;
    context->worker_tasks_pool = pomelo_pool_create(&pool_options);
    if (!context->worker_tasks_pool) {
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    // Initialize thread
    uv_loop_t * loop = &context->event_loop;
    uv_async_t * async_task = &context->async_task;
    uv_async_t * async_shutdown = &context->async_shutdown;

    uv_loop_init(loop);
    uv_async_init(loop, async_task, pomelo_webrtc_async_task_callback);
    uv_async_init(loop, async_shutdown, pomelo_webrtc_async_shutdown_callback);

    async_task->data = context;
    async_shutdown->data = context;

    // Start the plugin thread
    pomelo_atomic_int64_store(&context->thread_running, true);
    int ret = uv_thread_create(
        &context->thread,
        (uv_thread_cb) pomelo_webrtc_thread_entry,
        context
    );
    if (ret < 0) { // Failed to start thread
        pomelo_atomic_int64_store(&context->thread_running, false);
        pomelo_webrtc_context_destroy(context);
        return NULL;
    }

    return context;
}


void pomelo_webrtc_context_destroy(pomelo_webrtc_context_t * context) {
    assert(context != NULL);
    pomelo_webrtc_thread_shutdown(context);

    context->plugin = NULL;

    if (context->rtc_context) {
        rtc_context_destroy(context->rtc_context);
        context->rtc_context = NULL;
    }

    if (context->socket_map) {
        pomelo_map_destroy(context->socket_map);
        context->socket_map = NULL;
    }

    if (context->string_buffer_pool) {
        pomelo_pool_destroy(context->string_buffer_pool);
        context->string_buffer_pool = NULL;
    }

    if (context->connect_token_pool) {
        pomelo_pool_destroy(context->connect_token_pool);
        context->connect_token_pool = NULL;
    }

    if (context->socket_pool) {
        pomelo_pool_destroy(context->socket_pool);
        context->socket_pool = NULL;
    }

    if (context->session_pool) {
        pomelo_pool_destroy(context->session_pool);
        context->session_pool = NULL;
    }

    if (context->channel_pool) {
        pomelo_pool_destroy(context->channel_pool);
        context->channel_pool = NULL;
    }

    if (context->tasks) {
        pomelo_list_destroy(context->tasks);
        context->tasks = NULL;
    }

    if (context->async_tasks_pool) {
        pomelo_pool_destroy(context->async_tasks_pool);
        context->async_tasks_pool = NULL;
    }

    if (context->scheduled_tasks_pool) {
        pomelo_pool_destroy(context->scheduled_tasks_pool);
        context->scheduled_tasks_pool = NULL;
    }

    if (context->worker_tasks_pool) {
        pomelo_pool_destroy(context->worker_tasks_pool);
        context->worker_tasks_pool = NULL;
    }

    // Free itself
    pomelo_allocator_free(context->allocator, context);
}


pomelo_webrtc_task_t * pomelo_webrtc_context_submit_task(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_task_cb callback,
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(context != NULL);
    assert(callback != NULL);
    assert(argc == 0 || args != NULL);

    if (!pomelo_atomic_int64_load(&context->thread_running)) {
        return NULL; // Thread is stopped
    }

    if (argc > POMELO_WEBRTC_TASK_MAX_ARGS) {
        return NULL;
    }

    pomelo_webrtc_task_t * task =
        pomelo_pool_acquire(context->async_tasks_pool);
    if (!task) {
        return NULL;
    }

    task->callback = callback;
    task->argc = argc;
    if (argc > 0) {
        memcpy(task->args, args, argc * sizeof(pomelo_webrtc_variant_t));
    }

    // Add task to list and send running signal
    pomelo_list_push_back(context->tasks, task);
    uv_async_send(&context->async_task);

    return task;
}


pomelo_webrtc_task_t * pomelo_webrtc_context_schedule_task(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_task_cb callback,
    size_t argc,
    pomelo_webrtc_variant_t * args,
    uint64_t interval_ms
) {
    assert(context != NULL);
    assert(callback != NULL);
    assert(argc == 0 || args != NULL);

    if (!pomelo_atomic_int64_load(&context->thread_running)) {
        return NULL; // Thread is stopped
    }

    if (argc > POMELO_WEBRTC_TASK_MAX_ARGS || interval_ms == 0) {
        return NULL;
    }

    pomelo_webrtc_scheduled_task_t * task =
        pomelo_pool_acquire(context->scheduled_tasks_pool);
    if (!task) {
        return NULL;
    }

    pomelo_webrtc_task_t * base = &task->base;
    base->callback = callback;
    base->argc = argc;
    if (argc > 0) {
        memcpy(base->args, args, argc * sizeof(pomelo_webrtc_variant_t));
    }

    // Init and start the timer
    task->timer.data = base;
    uv_timer_init(&context->event_loop, &task->timer);
    int ret = uv_timer_start(
        &task->timer,
        pomelo_webrtc_timer_callback,
        interval_ms, // Timeout
        interval_ms  // Repeat
    );
    if (ret < 0) {
        pomelo_pool_release(context->scheduled_tasks_pool, task);
        return NULL;
    }

    return base;
}


void pomelo_webrtc_context_unschedule_task(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_task_t * task
) {
    assert(context != NULL);
    assert(task != NULL);

    uv_timer_stop(&((pomelo_webrtc_scheduled_task_t *) task)->timer);
    pomelo_pool_release(context->scheduled_tasks_pool, task);
}


pomelo_webrtc_task_t * pomelo_webrtc_context_spawn_task(
    pomelo_webrtc_context_t * context,
    pomelo_webrtc_task_cb work,
    pomelo_webrtc_task_cb callback,
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(context != NULL);
    assert(work != NULL);
    assert(argc == 0 || args != NULL);
    // Callback could be NULL

    if (!pomelo_atomic_int64_load(&context->thread_running)) {
        return NULL; // Thread is stopped
    }

    if (argc > POMELO_WEBRTC_TASK_MAX_ARGS) {
        return NULL;
    }

    pomelo_webrtc_worker_task_t * task =
        pomelo_pool_acquire(context->worker_tasks_pool);
    if (!task) {
        return NULL;
    }

    pomelo_webrtc_task_t * base = &task->base;
    base->callback = callback;
    base->argc = argc;
    if (argc > 0) {
        memcpy(base->args, args, argc * sizeof(pomelo_webrtc_variant_t));
    }

    task->work_fn = work;
    task->work.data = task;

    // Submit worker task
    uv_queue_work(
        &context->event_loop,
        &task->work,
        pomelo_webrtc_worker_task_process,
        pomelo_webrtc_worker_task_callback
    );

    return base;
}


/* -------------------------------------------------------------------------- */
/*                              Private APIs                                  */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_thread_entry(pomelo_webrtc_context_t * context) {
    assert(context != NULL);

    // Set as running and run the event loop
    uv_run(&context->event_loop, UV_RUN_DEFAULT);

    // Set running flag to false
    pomelo_atomic_int64_store(&context->thread_running, false);
}


void pomelo_webrtc_thread_shutdown(pomelo_webrtc_context_t * context) {
    assert(context != NULL);

    bool thread_running = pomelo_atomic_int64_compare_exchange(
        &context->thread_running, true, false
    );
    if (!thread_running) {
        return; // Thread is not running anymore
    }

    uv_async_send(&context->async_shutdown);

    // Join the thread
    uv_thread_join(&context->thread);
}


void pomelo_webrtc_async_task_callback(uv_async_t * async) {
    assert(async != NULL);
    pomelo_webrtc_context_t * context = async->data;
    pomelo_list_t * tasks = context->tasks;
    pomelo_pool_t * tasks_pool = context->async_tasks_pool;

    pomelo_webrtc_task_t * task = NULL;
    while (pomelo_list_pop_front(tasks, &task) == 0) {
        pomelo_webrtc_variant_t * args = task->args;
        size_t argc = task->argc;
        pomelo_webrtc_task_cb callback = task->callback;
        pomelo_pool_release(tasks_pool, task);
        callback(argc, args);
    }
}


void pomelo_webrtc_async_shutdown_callback(uv_async_t * async) {
    assert(async != NULL);
    pomelo_webrtc_context_t * context = async->data;
    uv_stop(&context->event_loop);
}


void pomelo_webrtc_timer_callback(uv_timer_t * timer) {
    assert(timer != NULL);
    // Call the timer callback
    pomelo_webrtc_task_t * task = timer->data;
    task->callback(task->argc, task->args);
}


void pomelo_webrtc_worker_task_process(uv_work_t * work) {
    pomelo_webrtc_worker_task_t * task = work->data;
    pomelo_webrtc_task_t * base = &task->base;
    task->work_fn(base->argc, base->args);
}


void pomelo_webrtc_worker_task_callback(uv_work_t * work, int status) {
    if (status == UV_ECANCELED) {
        return; // Canceled
    }

    pomelo_webrtc_worker_task_t * task = work->data;
    pomelo_webrtc_task_t * base = &task->base;
    if (base->callback) {
        base->callback(base->argc, base->args);
    }
}
