#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "session/session.h"
#include "context.h"
#include "socket.h"
#include "socket-plugin.h"
#include "socket-wss.h"

#define POMELO_WEBRTC_CHANNELS_INIT_CAPACITY 64

#define pomelo_webrtc_socket_is_active(socket)                                 \
POMELO_CHECK_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_ACTIVE)

#define pomelo_webrtc_socket_set_active(socket)                                \
POMELO_SET_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_ACTIVE)

#define pomelo_webrtc_socket_unset_active(socket)                                \
POMELO_UNSET_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_ACTIVE)

/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


int pomelo_webrtc_socket_on_alloc(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_context_t * context
) {
    assert(socket != NULL);
    assert(context != NULL);
    socket->context = context;

    pomelo_allocator_t * allocator = context->allocator;
    pomelo_list_options_t list_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_webrtc_session_t *)
    };
    socket->sessions = pomelo_list_create(&list_options);
    if (!socket->sessions) return -1;

    pomelo_array_options_t array_options = {
        .allocator = allocator,
        .initial_capacity = POMELO_WEBRTC_CHANNELS_INIT_CAPACITY,
        .element_size = sizeof(pomelo_channel_mode)
    };
    socket->channel_modes = pomelo_array_create(&array_options);
    if (!socket->channel_modes) return -1;

    return 0;
}


void pomelo_webrtc_socket_on_free(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    socket->context = NULL;

    if (socket->sessions) {
        pomelo_list_destroy(socket->sessions);
        socket->sessions = NULL;
    }

    if (socket->channel_modes) {
        pomelo_array_destroy(socket->channel_modes);
        socket->channel_modes = NULL;
    }
}


int pomelo_webrtc_socket_init(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_socket_info_t * info
) {
    assert(socket != NULL);
    assert(info != NULL);
    
    // Init reference
    pomelo_reference_init(
        &socket->ref,
        (pomelo_ref_finalize_cb) pomelo_webrtc_socket_on_finalize
    );

    int ret = pomelo_webrtc_socket_plugin_init(
        socket,
        info->plugin,
        info->native_socket
    );
    if (ret < 0) return -1;

    ret = pomelo_webrtc_socket_wss_init(socket, info->address);
    if (ret < 0) return -1;

    // Attach socket
    pomelo_webrtc_socket_set_active(socket);
    return 0;
}


void pomelo_webrtc_socket_cleanup(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    // Finalize all parts of socket
    pomelo_webrtc_socket_wss_cleanup(socket);
    pomelo_webrtc_socket_plugin_cleanup(socket);

    // Clear sessions
    pomelo_list_clear(socket->sessions);
    pomelo_array_clear(socket->channel_modes);

    socket->flags = 0;
    socket->native_socket = NULL;
    socket->ws_server = NULL;
}


void pomelo_webrtc_socket_ref(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    pomelo_reference_ref(&socket->ref);
}


void pomelo_webrtc_socket_unref(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    pomelo_reference_unref(&socket->ref);
}


void pomelo_webrtc_socket_close(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    if (!pomelo_webrtc_socket_is_active(socket)) {
        return; // Socket has already been deactivated
    }

    // Deactivate this socket
    pomelo_webrtc_socket_unset_active(socket);

    // Close all sessions
    pomelo_webrtc_session_t * session = NULL;
    while (pomelo_list_pop_front(socket->sessions, &session) == 0) {
        session->list_entry = NULL;
        pomelo_webrtc_session_close(session);
    }

    pomelo_webrtc_socket_wss_close(socket);

    // Unref itself
    pomelo_webrtc_socket_unref(socket);
}


void pomelo_webrtc_socket_remove_session(
    pomelo_webrtc_socket_t * socket,
    pomelo_webrtc_session_t * session
) {
    assert(socket != NULL);
    assert(session != NULL);

    if (session->list_entry) {
        pomelo_list_remove(socket->sessions, session->list_entry);
        session->list_entry = NULL;
    }
}


/* -------------------------------------------------------------------------- */
/*                               Module APIs                                  */
/* -------------------------------------------------------------------------- */

pomelo_webrtc_session_t * pomelo_webrtc_socket_create_session(
    pomelo_webrtc_socket_t * socket,
    rtc_websocket_client_t * ws_client
) {
    assert(socket != NULL);
    assert(ws_client != NULL);

    pomelo_webrtc_session_info_t info = {
        .socket = socket,
        .ws_client = ws_client
    };

    // Create new session
    pomelo_webrtc_session_t * session =
        pomelo_webrtc_context_acquire_session(socket->context, &info);
    if (!session) return NULL; // Failed to create new session

    // Add session to sessions list
    session->list_entry = pomelo_list_push_back(socket->sessions, session);
    if (!session->list_entry) {
        // Failed to append new session to list
        pomelo_webrtc_session_close(session);
        return NULL;
    }

    return session;
}


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_socket_on_finalize(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    // Release socket
    pomelo_webrtc_context_release_socket(socket->context, socket);
}

