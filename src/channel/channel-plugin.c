#include <assert.h>
#include "session/session.h"
#include "channel-plugin.h"
#include "context.h"
#include "utils/macro.h"


/* -------------------------------------------------------------------------- */
/*                           Plugin implementation                            */
/* -------------------------------------------------------------------------- */

static void pomelo_webrtc_plugin_session_receive_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 4);
    assert(args != NULL);

    pomelo_plugin_t * plugin = args[0].ptr;
    pomelo_session_t * native_session = args[1].ptr;

    pomelo_webrtc_session_t * session =
        plugin->session_get_private(plugin, native_session);
    size_t channel_index = args[2].size;
    rtc_buffer_t * buffer = args[3].ptr;

    pomelo_webrtc_channel_t * channel = NULL;
    pomelo_array_get(session->channels, channel_index, &channel);
    assert(channel != NULL);

    // Unref the buffer and channel
    rtc_buffer_unref(buffer);
    pomelo_webrtc_channel_unref(channel);
}


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_receive(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    size_t channel_index,
    void * callback_data,
    pomelo_message_t * message,
    pomelo_plugin_error_t error
) {
    // `message` is only available inside this function
    assert(plugin != NULL);
    assert(callback_data != NULL);

    if (error == POMELO_PLUGIN_ERROR_OK) {
        assert(message != NULL);

        rtc_buffer_t * buffer = callback_data;
        plugin->message_write(
            plugin,
            message,
            rtc_buffer_size(buffer),
            rtc_buffer_data(buffer)
        );
    }

    pomelo_webrtc_variant_t args[] = {
        { .ptr = plugin },
        { .ptr = native_session },
        { .size = channel_index },
        { .ptr = callback_data }
    };

    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_plugin_session_receive_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


static void pomelo_webrtc_plugin_session_send_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 4);
    assert(args != NULL);

    pomelo_plugin_t * plugin = args[0].ptr;
    pomelo_session_t * native_session = args[1].ptr;
    size_t channel_index = args[2].size;
    rtc_buffer_t * buffer = args[3].ptr;

    pomelo_webrtc_session_t * session =
        plugin->session_get_private(plugin, native_session);
    
    pomelo_webrtc_channel_t * channel = NULL;
    pomelo_array_get(session->channels, channel_index, &channel);

    if (channel != NULL && channel != session->system_channel) {
        pomelo_webrtc_channel_send_buffer(channel, buffer);
    }

    rtc_buffer_unref(buffer);
}


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_send(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    size_t channel_index,
    pomelo_message_t * message
) {
    assert(plugin != NULL);
    // `message` is only available inside this function

    // Prepare a RTC data buffer here, copy the data of message into it.
    size_t length = plugin->message_length(plugin, message);
    if (length == 0) {
        return; // Empty message
    }

    uint8_t * data = NULL;
    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    rtc_buffer_t * buffer =
        rtc_buffer_prepare(context->rtc_context, length, &data);
    if (!buffer) {
        return; // Failed to acquire new buffer
    }

    if (plugin->message_read(plugin, message, length, data) < 0) {
        return; // Failed to read
    }

    pomelo_webrtc_variant_t args[] = {
        { .ptr = plugin },
        { .ptr = native_session },
        { .size = channel_index },
        { .ptr = buffer }
    };

    pomelo_webrtc_task_t * task = pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_plugin_session_send_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
    if (!task) {
        // Failed to submit new task
        rtc_buffer_unref(buffer);
    }
}

