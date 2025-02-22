#include <assert.h>
#include "socket-pool.h"
#include "context.h"


#define POMELO_WEBRTC_CHANNELS_INIT_CAPACITY 128

int pomelo_webrtc_socket_pool_allocate_callback(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_context_t * context
) {
    pomelo_allocator_t * allocator = context->allocator;
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_webrtc_session_t *);
    socket->sessions = pomelo_list_create(&list_options);
    if (!socket->sessions) {
        return -1;
    }

    pomelo_array_options_t array_options;
    pomelo_array_options_init(&array_options);
    array_options.allocator = allocator;
    array_options.initial_capacity = POMELO_WEBRTC_CHANNELS_INIT_CAPACITY;
    array_options.element_size = sizeof(pomelo_channel_mode);
    socket->channel_modes = pomelo_array_create(&array_options);
    if (!socket->channel_modes) {
        return -1;
    }

    return 0;
}


int pomelo_webrtc_socket_pool_deallocate_callback(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_context_t * context
) {
    assert(socket != NULL);
    (void) context;

    if (socket->sessions) {
        pomelo_list_destroy(socket->sessions);
        socket->sessions = NULL;
    }

    if (socket->channel_modes) {
        pomelo_array_destroy(socket->channel_modes);
        socket->channel_modes = NULL;
    }

    return 0;
}


int pomelo_webrtc_socket_pool_acquire_callback(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_context_t * context
) {
    assert(socket != NULL);

    socket->context = context;
    socket->flags = 0;
    socket->native_socket = NULL;
    socket->ws_server = NULL;

    pomelo_list_clear(socket->sessions);
    pomelo_array_clear(socket->channel_modes);

    return 0;
}
