#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "utils/macro.h"
#include "utils/array.h"
#include "utils/string-buffer.h"
#include "utils/common-macro.h"
#include "channel.h"
#include "context.h"
#include "session/session.h"
#include "channel-dc.h"
#include "channel-plugin.h"


#define pomelo_webrtc_channel_is_active(channel)                               \
POMELO_CHECK_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_ACTIVE)

#define pomelo_webrtc_channel_set_active(channel)                              \
POMELO_SET_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_ACTIVE)

#define pomelo_webrtc_channel_unset_active(channel)                            \
POMELO_UNSET_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_ACTIVE)


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */


int pomelo_webrtc_channel_on_alloc(
    pomelo_webrtc_channel_t * channel,
    pomelo_webrtc_context_t * context
) {
    assert(channel != NULL);
    assert(context != NULL);
    channel->context = context;
    return 0;
}


void pomelo_webrtc_channel_on_free(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    channel->context = NULL;
}


int pomelo_webrtc_channel_init(
    pomelo_webrtc_channel_t * channel,
    pomelo_webrtc_channel_info_t * info
) {
    assert(channel != NULL);
    assert(info != NULL);
    
    // Init reference
    pomelo_reference_init(
        &channel->ref,
        (pomelo_ref_finalize_cb) pomelo_webrtc_channel_on_finalize
    );
    
    // Reference the session
    channel->session = info->session;
    pomelo_webrtc_session_ref(info->session);

    channel->index = info->channel_index;
    channel->mode = info->channel_mode;
    pomelo_webrtc_channel_set_active(channel);

    // Initialize DC part of channel
    int ret = pomelo_webrtc_channel_dc_init(channel);
    if (ret < 0) return -1;
    return 0;
}


void pomelo_webrtc_channel_cleanup(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    // Cleanup dc part
    pomelo_webrtc_channel_dc_cleanup(channel);

    // Unref the session
    pomelo_webrtc_session_unref(channel->session);
    channel->session = NULL;

    channel->flags = 0;
    channel->index = 0;
    channel->mode = POMELO_CHANNEL_MODE_UNRELIABLE;
}


void pomelo_webrtc_channel_close(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    if (!pomelo_webrtc_channel_is_active(channel)) {
        return;
    }
    pomelo_webrtc_channel_unset_active(channel);

    // Close the channel
    pomelo_webrtc_channel_dc_close(channel);

    // Finally, unref itself
    pomelo_webrtc_channel_unref(channel);
}


void pomelo_webrtc_channel_enable_receiving(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    pomelo_webrtc_channel_dc_enable_receiving(channel);
}


void pomelo_webrtc_channel_send(
    pomelo_webrtc_channel_t * channel,
    const uint8_t * data,
    size_t length
) {
    assert(channel != NULL);
    assert(data != NULL);
    if (length == 0) return;

    // Send message and unref the buffer
    rtc_data_channel_send(channel->outgoing_dc, data, length);
}


void pomelo_webrtc_channel_set_mode(
    pomelo_webrtc_channel_t * channel,
    pomelo_channel_mode mode
) {
    assert(channel != NULL);
    (void) mode;

    // Before new channel is opened, keep the previous one

    // TODO: Implement me, destroy the previous one, create new one to replace
    // It a little bit complicated here
}


void pomelo_webrtc_channel_set_incoming_data_channel(
    pomelo_webrtc_channel_t * channel,
    rtc_data_channel_t * incoming_dc
) {
    pomelo_webrtc_channel_dc_set_incoming_data_channel(channel, incoming_dc);
}


/* -------------------------------------------------------------------------- */
/*                               Module APIs                                  */
/* -------------------------------------------------------------------------- */


void pomelo_webrtc_channel_ref(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    pomelo_reference_ref(&channel->ref);
}


void pomelo_webrtc_channel_unref(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    pomelo_reference_unref(&channel->ref);
}


void pomelo_webrtc_channel_send_buffer(
    pomelo_webrtc_channel_t * channel,
    rtc_buffer_t * buffer
) {
    assert(channel != NULL);
    assert(buffer != NULL);

    // Send message and unref the buffer
    rtc_data_channel_send_buffer(channel->outgoing_dc, buffer);
}


void pomelo_webrtc_channel_receive(
    pomelo_webrtc_channel_t * channel,
    rtc_buffer_t * message
) {
    assert(channel != NULL);
    assert(message != NULL);

    // Ref this channel until the messasge is processed completely
    pomelo_webrtc_channel_ref(channel);

    // Keep the reference of message and submit command
    rtc_buffer_ref(message);

    pomelo_webrtc_context_t * context = channel->context;

    pomelo_webrtc_recv_command_t * command =
        pomelo_pool_acquire(context->recv_command_pool, NULL);
    if (!command) {
        // Failed to allocate command
        rtc_buffer_unref(message);
        pomelo_webrtc_channel_unref(channel);
        return;
    }

    command->message = message;
    command->native_session = channel->session->native_session;
    command->channel = channel;

    int ret = context->plugin->executor_submit(
        context->plugin,
        (pomelo_plugin_task_callback) pomelo_webrtc_plugin_session_receive,
        command
    );
    if (ret < 0) {
        // Failed to submit command
        rtc_buffer_unref(message);
        pomelo_webrtc_channel_unref(channel);
        pomelo_pool_release(context->recv_command_pool, command);
        return;
    }

    // => pomelo_webrtc_channel_receive_complete
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_channel_receive_complete(
    pomelo_webrtc_channel_t * channel,
    pomelo_webrtc_recv_command_t * command
) {
    assert(channel != NULL);
    assert(command != NULL);

    // Unref the message and channel, then release the command
    pomelo_webrtc_context_t * context = channel->context;

    rtc_buffer_unref(command->message);
    pomelo_webrtc_channel_unref(channel);
    pomelo_pool_release(context->recv_command_pool, command);
}


void pomelo_webrtc_channel_on_finalize(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    // Release the channel
    pomelo_webrtc_context_release_channel(channel->context, channel);
}
