#include <assert.h>
#include "context.h"
#include "socket-plugin.h"
#include "utils/macro.h"


/* -------------------------------------------------------------------------- */
/*                           Plugin implementation                            */
/* -------------------------------------------------------------------------- */

static void pomelo_webrtc_plugin_socket_on_listening_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 3);
    assert(args != NULL);
    
    // Create WebRTC socket
    pomelo_webrtc_socket_create(args[0].ptr, args[1].ptr, args[2].ptr);
}


void POMELO_PLUGIN_CALL pomelo_webrtc_socket_plugin_on_listening(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket,
    pomelo_address_t * address
) {
    assert(plugin != NULL);

    pomelo_webrtc_variant_t args[] = {
        { .ptr = plugin },
        { .ptr = native_socket },
        { .ptr = address }
    };

    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_plugin_socket_on_listening_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


static void pomelo_webrtc_plugin_socket_on_stopped_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 2);
    assert(args != NULL);

    pomelo_plugin_t * plugin = args[0].ptr;
    pomelo_socket_t * native_socket = args[1].ptr;
    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    pomelo_webrtc_socket_t * socket = NULL;

    int result =
        pomelo_map_get(context->socket_map, native_socket, (void *) &socket);
    if (result < 0) {
        // Nothing to do
        return;
    }

    pomelo_webrtc_socket_close(socket);
}


void POMELO_PLUGIN_CALL pomelo_webrtc_socket_plugin_on_stopped(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket
) {
    assert(plugin != NULL);
    pomelo_webrtc_variant_t args[] = {
        { .ptr = plugin },
        { .ptr = native_socket }
    };

    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_plugin_socket_on_stopped_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

int pomelo_webrtc_socket_plugin_init(
    pomelo_webrtc_socket_t * socket,
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket
) {
    assert(socket != NULL);
    assert(native_socket != NULL);

    socket->native_socket = native_socket;

    // Get the channels information
    size_t nchannels = plugin->socket_get_nchannels(plugin, native_socket);
    pomelo_array_t * channel_modes = socket->channel_modes;
    int ret = pomelo_array_ensure_size(channel_modes, nchannels);
    if (ret < 0) {
        pomelo_webrtc_socket_destroy(socket);
        return -1;
    }

    pomelo_channel_mode mode;
    for (size_t i = 0; i < nchannels; i++) {
        mode = plugin->socket_get_channel_mode(plugin, native_socket, i);
        pomelo_array_append(channel_modes, mode);
    }

    // Create association between 2 sockets
    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    ret = pomelo_map_set(context->socket_map, native_socket, socket);
    if (ret < 0) {
        pomelo_webrtc_socket_destroy(socket);
        return -1;
    }

    return 0;
}


void pomelo_webrtc_socket_plugin_finalize(pomelo_webrtc_socket_t * socket) {
    assert(socket != NULL);
    pomelo_socket_t * native_socket = socket->native_socket;
    if (!native_socket) {
        return;
    }

    pomelo_webrtc_context_t * context = socket->context;
    pomelo_plugin_t * plugin = context->plugin;
    
    socket->native_socket = NULL;

    // Remove association between native socket with plugin socket
    pomelo_map_del(context->socket_map, native_socket);
    
    // Finally, detach the socket
    plugin->socket_detach(plugin, native_socket);
}
