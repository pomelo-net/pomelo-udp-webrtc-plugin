#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "session/session.h"
#include "context.h"
#include "socket.h"
#include "socket-plugin.h"
#include "socket-wss.h"


#define pomelo_webrtc_socket_is_active(socket)                                 \
POMELO_CHECK_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_ACTIVE)

#define pomelo_webrtc_socket_set_active(socket)                                \
POMELO_SET_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_ACTIVE)

#define pomelo_webrtc_socket_unset_active(socket)                                \
POMELO_UNSET_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_ACTIVE)

/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

pomelo_webrtc_socket_t * pomelo_webrtc_socket_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket,
    pomelo_address_t * address
) {
    assert(plugin != NULL);
    assert(native_socket != NULL);

    // Create new webrtc socket
    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    pomelo_webrtc_socket_t * socket = pomelo_pool_acquire(context->socket_pool);
    if (!socket) {
        return NULL;
    }

    // Init reference
    pomelo_reference_init(
        &socket->ref,
        (pomelo_ref_finalize_cb) pomelo_webrtc_socket_destroy
    );

    int ret = pomelo_webrtc_socket_plugin_init(socket, plugin, native_socket);
    if (ret < 0) {
        pomelo_webrtc_socket_destroy(socket);
        return NULL;
    }

    ret = pomelo_webrtc_socket_wss_init(socket, address);
    if (ret < 0) {
        pomelo_webrtc_socket_destroy(socket);
        return NULL;
    }

    // Attach socket
    plugin->socket_attach(plugin, native_socket);
    pomelo_webrtc_socket_set_active(socket);

    return socket;
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
        session->list_node = NULL;
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

    if (session->list_node) {
        pomelo_list_remove(socket->sessions, session->list_node);
        session->list_node = NULL;
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

    // Create new session
    pomelo_webrtc_session_t * session =
        pomelo_webrtc_session_create(socket, ws_client);
    if (!session) {
        // Failed to create new session
        return NULL;
    }

    // Add session to sessions list
    session->list_node = pomelo_list_push_back(socket->sessions, session);
    if (!session->list_node) {
        // Failed to append new session to list
        pomelo_webrtc_session_close(session);
    }

    return session;
}


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_socket_destroy(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    pomelo_webrtc_context_t * context = socket->context;
    
    // Finalize all parts of socket
    pomelo_webrtc_socket_wss_finalize(socket);
    pomelo_webrtc_socket_plugin_finalize(socket);

    // Clear sessions
    pomelo_list_clear(socket->sessions);

    // Release itself
    pomelo_pool_release(context->socket_pool, socket);
}

