#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "utils/macro.h"
#include "utils/array.h"
#include "utils/string-buffer.h"
#include "channel.h"
#include "context.h"
#include "session/session.h"
#include "channel-dc.h"



#define pomelo_webrtc_channel_is_active(channel)                               \
POMELO_CHECK_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_ACTIVE)

#define pomelo_webrtc_channel_set_active(channel)                              \
POMELO_SET_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_ACTIVE)

#define pomelo_webrtc_channel_unset_active(channel)                            \
POMELO_UNSET_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_ACTIVE)


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

pomelo_webrtc_channel_t * pomelo_webrtc_channel_create(
    pomelo_webrtc_session_t * session,
    size_t channel_index,
    pomelo_channel_mode channel_mode
) {
    assert(session != NULL);

    pomelo_webrtc_channel_t * channel =
        pomelo_pool_acquire(session->context->channel_pool);
    if (!channel) {
        return NULL; // Failed to acquire new channel
    }

    // Init reference
    pomelo_reference_init(
        &channel->ref,
        (pomelo_ref_finalize_cb) pomelo_webrtc_channel_destroy
    );
    
    // Reference the session
    pomelo_webrtc_session_ref(session);

    channel->session = session;
    channel->index = channel_index;
    channel->mode = channel_mode;
    pomelo_webrtc_channel_set_active(channel);

    // Initialize DC part of channel
    int ret = pomelo_webrtc_channel_dc_init(channel);
    if (ret < 0) {
        pomelo_webrtc_channel_destroy(channel);
        return NULL;
    }

    return channel;
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
    if (length == 0) {
        return;
    }

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


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_channel_destroy(
    pomelo_webrtc_channel_t * channel
) {
    assert(channel != NULL);
    pomelo_webrtc_session_t * session = channel->session;

    // Finalize dc part
    pomelo_webrtc_channel_dc_finalize(channel);

    // Release the channel
    pomelo_pool_release(channel->context->channel_pool, channel);

    // Finally, unref the session
    pomelo_webrtc_session_unref(session);
}
