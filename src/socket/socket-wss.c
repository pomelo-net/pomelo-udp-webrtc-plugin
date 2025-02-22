#include <assert.h>
#include <string.h>
#include "utils/common-macro.h"
#include "session/session.h"
#include "socket-wss.h"
#include "context.h"


#define pomelo_webrtc_socket_wss_is_active(socket)                             \
POMELO_CHECK_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_WSS_ACTIVE)

#define pomelo_webrtc_socket_wss_set_active(socket)                            \
POMELO_SET_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_WSS_ACTIVE)

#define pomelo_webrtc_socket_wss_unset_active(socket)                          \
POMELO_UNSET_FLAG((socket)->flags, POMELO_WEBRTC_SOCKET_FLAG_WSS_ACTIVE)


/* -------------------------------------------------------------------------- */
/*                        Websocket server callbacks                          */
/* -------------------------------------------------------------------------- */

static void pomelo_webrtc_socket_wss_on_client_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 2);
    assert(args != NULL);

    rtc_websocket_server_t * ws_server = args[0].ptr;
    rtc_websocket_client_t * ws_client = args[1].ptr;

    pomelo_webrtc_socket_t * socket = rtc_websocket_server_get_data(ws_server);
    if (!pomelo_webrtc_socket_wss_is_active(socket)) {
        // Socket is deactivated
        rtc_websocket_client_destroy(ws_client);
        return;
    }

    pomelo_webrtc_socket_create_session(socket, ws_client);
}


void pomelo_webrtc_socket_wss_on_client(
    rtc_websocket_server_t * ws_server,
    rtc_websocket_client_t * ws_client
) {
    assert(ws_server != NULL);
    rtc_context_t * rtc_context = rtc_websocket_server_get_context(ws_server);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) return;

    pomelo_webrtc_variant_t args[] = {
        { .ptr = ws_server },
        { .ptr = ws_client }
    };

    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_socket_wss_on_client_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

int pomelo_webrtc_socket_wss_init(
    pomelo_webrtc_socket_t * socket,
    pomelo_address_t * address
) {
    assert(socket != NULL);
    assert(address != NULL);

    // Create websocket server
    rtc_websocket_server_options_t options;
    memset(&options, 0, sizeof(rtc_websocket_server_options_t));

    // Websocket use the same port as main socket. This is allowed, because they
    // use different protocols.
    options.context = socket->context->rtc_context;
    options.port = pomelo_address_port(address);
    options.data = socket;

    socket->ws_server = rtc_websocket_server_create(&options);
    if (!socket->ws_server) {
        // Failed to create websocket server
        return -1;
    }

    // Set wss as active
    pomelo_webrtc_socket_wss_set_active(socket);

    // Open WSS: WSS references socket until WSS is closed
    pomelo_webrtc_socket_ref(socket);

    return 0;
}


void pomelo_webrtc_socket_wss_cleanup(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    if (socket->ws_server) {
        rtc_websocket_server_destroy(socket->ws_server);
        socket->ws_server = NULL;
    }
}


static void pomelo_webrtc_socket_wss_close_process_task(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 1);
    assert(args != NULL);
    pomelo_webrtc_socket_wss_close_process(args[0].ptr);
}


void pomelo_webrtc_socket_wss_close(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);

    if (!pomelo_webrtc_socket_wss_is_active(socket)) {
        return; // WSS is deactivated
    }
    pomelo_webrtc_socket_wss_unset_active(socket);

    // Spawn a task to close the websocket server.
    // The current implementation of WSS will block the caller until WSS is
    // completely stopped. So that why we should spawn it as worker task.
    pomelo_webrtc_variant_t args[] = {
        { .ptr = socket }
    };
    pomelo_webrtc_context_spawn_task(
        socket->context,
        pomelo_webrtc_socket_wss_close_process_task,
        /* callbacl */ NULL, // No callback is need
        /* argc = */ 1,
        args
    );
}


static void pomelo_webrtc_socket_wss_on_closed_task(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 1);
    assert(args != NULL);
    pomelo_webrtc_socket_wss_on_closed(args[0].ptr);
}


void pomelo_webrtc_socket_wss_close_process(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    pomelo_webrtc_context_t * context = socket->context;

    // Close the Websocket server
    rtc_websocket_server_close(socket->ws_server);

    pomelo_webrtc_variant_t args[] = {{ .ptr = socket }};

    // We need to submit an `closed` event here to make sure it will be the last
    // event of WSS.
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_socket_wss_on_closed_task,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


void pomelo_webrtc_socket_wss_on_closed(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);

    // WSS has completely stopped. No more callbacks will be called
    pomelo_webrtc_socket_unref(socket);
}
