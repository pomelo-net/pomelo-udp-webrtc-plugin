#include <assert.h>
#include "context.h"
#include "channel/channel.h"
#include "socket/socket.h"
#include "session-plugin.h"


/* -------------------------------------------------------------------------- */
/*                           Plugin implementation                            */
/* -------------------------------------------------------------------------- */

static void pomelo_webrtc_plugin_session_create_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 2);
    assert(args != NULL);

    pomelo_webrtc_session_t * session = args[0].ptr;
    pomelo_session_t * native_session = args[1].ptr;

    pomelo_webrtc_session_plugin_on_created(session, native_session);
    pomelo_webrtc_session_unref(session);
}


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * native_socket,
    pomelo_session_t * native_session,
    void * callback_data,
    pomelo_plugin_error_t error
) {
    assert(plugin != NULL);
    (void) native_socket;
    (void) error;

    pomelo_webrtc_variant_t args[] = {
        { .ptr = callback_data },
        { .ptr = native_session }
    };

    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_plugin_session_create_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


static void pomelo_webrtc_plugin_session_disconnect_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 2);
    assert(args != NULL);
    
    pomelo_plugin_t * plugin = args[0].ptr;
    pomelo_session_t * native_session = args[1].ptr;

    pomelo_webrtc_session_t * session =
        plugin->session_get_private(plugin, native_session);

    pomelo_webrtc_session_plugin_process_disconnect(session);
}


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_disconnect(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session
) {
    assert(plugin != NULL);
    pomelo_webrtc_variant_t args[] = {
        { .ptr = plugin },
        { .ptr = native_session }
    };

    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_plugin_session_disconnect_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_get_rtt(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    uint64_t * mean,
    uint64_t * variance
) {
    // Call the function directly
    assert(plugin != NULL);
    assert(native_session != NULL);

    pomelo_webrtc_session_t * session =
        plugin->session_get_private(plugin, native_session);

    if (mean) {
        *mean = pomelo_atomic_uint64_load(&session->rtt.mean);
    }

    if (variance) {
        *variance = pomelo_atomic_uint64_load(&session->rtt.variance);
    }
}


static void pomelo_webrtc_plugin_session_set_mode_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 4);
    assert(args != NULL);

    pomelo_plugin_t * plugin = args[0].ptr;
    pomelo_session_t * native_session = args[1].ptr;
    size_t channel_index = args[2].size;
    pomelo_channel_mode channel_mode = args[3].i32;

    pomelo_webrtc_session_t * session =
        plugin->session_get_private(plugin, native_session);

    pomelo_webrtc_channel_t * channel = NULL;
    pomelo_array_get(session->channels, channel_index, &channel);
    if (channel) {
        pomelo_webrtc_channel_set_mode(channel, channel_mode);
    }
}


int POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_set_mode(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    size_t channel_index,
    pomelo_channel_mode channel_mode
) {
    pomelo_webrtc_variant_t args[] = {
        { .ptr = plugin },
        { .ptr = native_session },
        { .size = channel_index },
        { .i32 = channel_mode }
    };

    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    pomelo_webrtc_task_t * task = pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_plugin_session_set_mode_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
    return task ? 0 : -1;
}

/* -------------------------------------------------------------------------- */
/*                                 Public APIs                                */
/* -------------------------------------------------------------------------- */

int pomelo_webrtc_session_plugin_init(pomelo_webrtc_session_t * session) {
    (void) session;
    // Nothing to do yet
    return 0;
}


void pomelo_webrtc_session_plugin_finalize(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    pomelo_webrtc_session_plugin_destroy_native_session(session);
}


void pomelo_webrtc_session_plugin_close(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    pomelo_webrtc_session_plugin_destroy_native_session(session);
}


void pomelo_webrtc_session_plugin_open(
    pomelo_webrtc_session_t * session,
    pomelo_address_t * address
) {
    assert(session != NULL);
    assert(address != NULL);
    assert(!session->native_session);

    pomelo_plugin_t * plugin = session->context->plugin;
    
    // Ref this session until native plugin call done
    pomelo_webrtc_session_ref(session);

    plugin->session_create(
        plugin,
        session->socket->native_socket,
        session->client_id,
        address,
        session, // private data
        session  // callback data
    );

    // => pomelo_webrtc_session_plugin_on_created
}

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */


void pomelo_webrtc_session_plugin_on_created(
    pomelo_webrtc_session_t * session,
    pomelo_session_t * native_session
) {
    assert(session != NULL);
    if (!native_session) {
        // Failed to create new native session
        pomelo_webrtc_session_close(session);
        return;
    }

    // Set associated native session and dispatch connected
    session->native_session = native_session;
    pomelo_webrtc_session_on_connected(session);
}


void pomelo_webrtc_session_plugin_process_disconnect(
    pomelo_webrtc_session_t * session
) {
    assert(session != NULL);

    // Close session    
    pomelo_webrtc_session_close(session);
}


void pomelo_webrtc_session_plugin_destroy_native_session(
    pomelo_webrtc_session_t * session
) {
    assert(session != NULL);
    if (!session->native_session) {
        return;
    }

    pomelo_plugin_t * plugin = session->context->plugin;

    // Emit disconnect here
    plugin->session_destroy(plugin, session->native_session);
    session->native_session = NULL;
}
