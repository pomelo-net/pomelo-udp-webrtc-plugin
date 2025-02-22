#include <assert.h>
#include <stdio.h>
#include "utils/macro.h"
#include "context.h"
#include "session/session.h"
#include "channel-dc.h"
#include "socket/socket.h"


#define RTC_CHANNEL_NAME_CAPACITY (sizeof(POMELO_SERVER_CHANNEL_PREFIX) + 11)

#define pomelo_webrtc_channel_dc_is_receiving_enabled(channel)                 \
POMELO_CHECK_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_DC_RECEIVE)

#define pomelo_webrtc_channel_dc_set_receiving_enabled(channel)                \
POMELO_SET_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_DC_RECEIVE)


#define pomelo_webrtc_channel_dc_is_active(channel)                            \
POMELO_CHECK_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_DC_ACTIVE)

#define pomelo_webrtc_channel_dc_set_active(channel)                           \
POMELO_SET_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_DC_ACTIVE)

#define pomelo_webrtc_channel_dc_unset_active(channel)                         \
POMELO_UNSET_FLAG((channel)->flags, POMELO_WEBRTC_CHANNEL_FLAG_DC_ACTIVE)


/* -------------------------------------------------------------------------- */
/*                                 DC Callbacks                               */
/* -------------------------------------------------------------------------- */

static void pomelo_webrtc_dc_on_open_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 1);
    assert(args != NULL);

    rtc_data_channel_t * dc = args[0].ptr;
    pomelo_webrtc_channel_t * channel = rtc_data_channel_get_data(dc);
    if (!channel) {
        return;
    }

    if (dc == channel->outgoing_dc) {
        pomelo_webrtc_session_on_channel_opened(channel->session, channel);
    }
}


void pomelo_webrtc_dc_on_open(rtc_data_channel_t * dc) {
    assert(dc != NULL);
    rtc_context_t * rtc_context = rtc_data_channel_get_context(dc);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }

    pomelo_webrtc_variant_t args[] = {{ .ptr = dc }};
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_dc_on_open_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


static void pomelo_webrtc_dc_on_closed_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 1);
    assert(args != NULL);

    rtc_data_channel_t * dc = args[0].ptr;
    pomelo_webrtc_channel_t * channel = rtc_data_channel_get_data(dc);
    if (!channel) {
        return;
    }

    // Emit this event to session
    pomelo_webrtc_session_remove_channel(channel->session, channel);

    // And unref the channel
    pomelo_webrtc_channel_unref(channel);
}


void pomelo_webrtc_dc_on_closed(rtc_data_channel_t * dc) {
    assert(dc != NULL);
    rtc_context_t * rtc_context = rtc_data_channel_get_context(dc);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }
    
    pomelo_webrtc_variant_t args[] = {{ .ptr = dc }};
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_dc_on_closed_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


static void pomelo_webrtc_dc_on_error_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 1);
    assert(args != NULL);

    rtc_data_channel_t * dc = args[0].ptr;
    pomelo_webrtc_channel_t * channel = rtc_data_channel_get_data(dc);
    if (!channel) {
        return;
    }

    pomelo_webrtc_channel_close(channel);
}


void pomelo_webrtc_dc_on_error(
    rtc_data_channel_t * dc,
    const char * error
) {
    assert(dc != NULL);
    rtc_context_t * rtc_context = rtc_data_channel_get_context(dc);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }

    pomelo_webrtc_log_debug(error);

    pomelo_webrtc_variant_t args[] = {{ .ptr = dc }};
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_dc_on_error_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


static void pomelo_webrtc_dc_on_message_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 2);
    assert(args != NULL);

    rtc_data_channel_t * dc = args[0].ptr;
    rtc_buffer_t * message = args[1].ptr;
    uint64_t recv_time = uv_hrtime();

    pomelo_webrtc_channel_t * channel = rtc_data_channel_get_data(dc);
    if (!channel) {
        return;
    }
    
    if (dc != channel->incoming_dc) {
        return;
    }

    pomelo_webrtc_channel_dc_process_message(channel, message, recv_time);

    // Finally, unref the message
    rtc_buffer_unref(message);
}


void pomelo_webrtc_dc_on_message(
    rtc_data_channel_t * dc,
    rtc_buffer_t * message
) {
    assert(dc != NULL);
    rtc_context_t * rtc_context = rtc_data_channel_get_context(dc);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }

    pomelo_webrtc_variant_t args[] = {
        { .ptr = dc },
        { .ptr = message }
    };
    rtc_buffer_ref(message);
    pomelo_webrtc_task_t * task = pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_dc_on_message_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );

    if (!task) {
        // Failed to submit task
        rtc_buffer_unref(message);
    }
}


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

int pomelo_webrtc_channel_dc_init(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    
    rtc_data_channel_options_t options;
    memset(&options, 0, sizeof(rtc_data_channel_options_t));
    
    char channel_name[RTC_CHANNEL_NAME_CAPACITY];
    options.data = channel;

    switch (channel->mode) {
        case POMELO_CHANNEL_MODE_SEQUENCED:
            options.reliability.unreliable = true;
            options.reliability.unordered = false;
            break;
        
        case POMELO_CHANNEL_MODE_RELIABLE:
            options.reliability.unreliable = false;
            options.reliability.unordered = false;
            break;

        default: // UNRELIABLE
            options.reliability.unreliable = true;
            options.reliability.unordered = true;
    }
    
    // Generate label for channel
    if (channel->index == POMELO_WEBRTC_CHANNEL_SYSTEM_INDEX) {
        // System channel
        options.label = POMELO_SYSTEM_CHANNEL_LABEL;
    } else {
        // Data channel
        sprintf(channel_name, POMELO_SERVER_CHANNEL_PREFIX "%zu", channel->index);
        options.label = channel_name;
    }

    // Create outgoing data channel
    channel->outgoing_dc = 
        rtc_peer_connection_create_data_channel(channel->session->pc, &options);
    if (!channel->outgoing_dc) {
        // Failed to create data channel
        return -1;
    }

    // Set channel as active
    pomelo_webrtc_channel_dc_set_active(channel);

    pomelo_webrtc_channel_ref(channel);
    return 0;
}


void pomelo_webrtc_channel_dc_finalize(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    if (channel->outgoing_dc) {
        rtc_data_channel_destroy(channel->outgoing_dc);
        channel->outgoing_dc = NULL;
    }
    
    if (channel->incoming_dc) {
        rtc_data_channel_destroy(channel->incoming_dc);
        channel->incoming_dc = NULL;
    }
}


void pomelo_webrtc_channel_dc_close(pomelo_webrtc_channel_t * channel) {
    assert(channel != NULL);
    if (!pomelo_webrtc_channel_dc_is_active(channel)) {
        return;
    }
    pomelo_webrtc_channel_dc_unset_active(channel);
    
    rtc_data_channel_close(channel->outgoing_dc);
    if (channel->incoming_dc) {
        rtc_data_channel_close(channel->incoming_dc);
    }
}


void pomelo_webrtc_channel_dc_enable_receiving(
    pomelo_webrtc_channel_t * channel
) {
    assert(channel != NULL);
    pomelo_webrtc_channel_dc_set_receiving_enabled(channel);
}


void pomelo_webrtc_channel_dc_set_incoming_data_channel(
    pomelo_webrtc_channel_t * channel,
    rtc_data_channel_t * incoming_dc
) {
    assert(channel != NULL);
    assert(incoming_dc != NULL);

    if (channel->incoming_dc) {
        return; // Incoming DC has been set
    }

    // Set associated data
    rtc_data_channel_set_data(incoming_dc, channel);
    channel->incoming_dc = incoming_dc;
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_channel_dc_process_message(
    pomelo_webrtc_channel_t * channel,
    rtc_buffer_t * message,
    uint64_t recv_time
) {
    assert(channel != NULL);
    assert(message != NULL);

    if (!pomelo_webrtc_channel_dc_is_active(channel)) {
        return; // Channel is not active
    }

    if (!pomelo_webrtc_channel_dc_is_receiving_enabled(channel)) {
        return; // Channel is not enabled
    }

    pomelo_webrtc_session_t * session = channel->session;
    if (channel == session->system_channel) {
        // Process system channel message
        pomelo_webrtc_session_process_system_message(
            session, message, recv_time
        );
        return;
    }

    // Ref this channel until the messasge is processed completely
    pomelo_webrtc_channel_ref(channel);

    // Keep the reference of message and submit command
    rtc_buffer_ref(message);

    pomelo_plugin_t * plugin = channel->context->plugin;
    plugin->session_receive(
        plugin,
        session->native_session,
        channel->index,
        message
    );
    // => pomelo_webrtc_plugin_session_receive
}
