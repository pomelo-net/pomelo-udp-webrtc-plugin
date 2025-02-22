#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "channel/channel.h"
#include "socket/socket.h"
#include "session-pc.h"
#include "context.h"


#define ADDRESS_BUFFER_LENGTH 64

#define pomelo_webrtc_session_pc_is_active(session)                            \
POMELO_CHECK_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_PC_ACTIVE)

#define pomelo_webrtc_session_pc_set_active(session)                           \
POMELO_SET_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_PC_ACTIVE)

#define pomelo_webrtc_session_pc_unset_active(session)                         \
POMELO_UNSET_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_PC_ACTIVE)


/* -------------------------------------------------------------------------- */
/*                                RTC Callbacks                               */
/* -------------------------------------------------------------------------- */


static void pomelo_webrtc_handle_pc_on_local_candidate_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 3);
    assert(args != NULL);

    rtc_peer_connection_t * pc = args[0].ptr;
    rtc_buffer_t * cand = args[1].ptr;
    rtc_buffer_t * mid = args[2].ptr;

    pomelo_webrtc_session_t * session = rtc_peer_connection_get_data(pc);
    if (pomelo_webrtc_session_pc_is_active(session)) {
        // Only accept local candidate when WS is active
        pomelo_webrtc_session_send_local_candidate(
            session,
            (const char *) rtc_buffer_data(cand),
            (const char *) rtc_buffer_data(mid)
        );
    }

    rtc_buffer_unref(cand);
    rtc_buffer_unref(mid);
}


void pomelo_webrtc_pc_on_local_candidate(
    rtc_peer_connection_t * pc,
    rtc_buffer_t * cand,
    rtc_buffer_t * mid
) {
    assert(pc != NULL);
    assert(cand != NULL);
    assert(mid != NULL);
    rtc_context_t * rtc_context = rtc_peer_connection_get_context(pc);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }

    pomelo_webrtc_variant_t args[] = {
        { .ptr = pc },
        { .ptr = cand },
        { .ptr = mid }
    };

    // Increase ref count of sdp and type
    rtc_buffer_ref(cand);
    rtc_buffer_ref(mid);

    pomelo_webrtc_task_t * task = pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_handle_pc_on_local_candidate_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
    if (!task) {
        // Failed to submit task
        rtc_buffer_unref(cand);
        rtc_buffer_unref(mid);
    }
}


static void pomelo_webrtc_pc_on_state_changed_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 2);
    assert(args != NULL);

    rtc_peer_connection_t * pc = args[0].ptr;
    rtc_peer_connection_state state = args[1].i32;
    pomelo_webrtc_session_t * session = rtc_peer_connection_get_data(pc);

    switch (state) {
        case RTC_PEER_CONNECTION_STATE_CONNECTED:
            // Established new connection
            pomelo_webrtc_session_pc_on_connected(session);
            break;
        
        case RTC_PEER_CONNECTION_STATE_DISCONNECTED:
        case RTC_PEER_CONNECTION_STATE_FAILED:
            // Close the peer connection in case of failure
            rtc_peer_connection_close(session->pc);
            break;

        case RTC_PEER_CONNECTION_STATE_CLOSED:
            pomelo_webrtc_session_pc_on_closed(session);
            break;
        
        default:
            // We do nothing on other states
            break;
    }
}


void pomelo_webrtc_pc_on_state_changed(
    rtc_peer_connection_t * pc,
    rtc_peer_connection_state state
) {
    assert(pc != NULL);
    rtc_context_t * rtc_context = rtc_peer_connection_get_context(pc);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }

    pomelo_webrtc_variant_t args[] = {
        { .ptr = pc },
        { .i32 = state }
    };

    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_pc_on_state_changed_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


static void pomelo_webrtc_pc_on_data_channel_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 4);
    assert(args != NULL);

    rtc_peer_connection_t * pc = args[0].ptr;
    pomelo_webrtc_session_t * session = rtc_peer_connection_get_data(pc);

    bool valid = args[1].b;
    if (!valid) {
        pomelo_webrtc_session_close(session);
        return; // Invalid channel label, close immediately
    }

    size_t index = args[2].size;
    rtc_data_channel_t * dc = args[3].ptr;
    
    // Get channel of session, and set incoming data channel
    pomelo_webrtc_channel_t * channel = NULL;
    pomelo_array_get(session->channels, index, &channel);
    if (!channel) {
        return; // Just ignore
    }

    // Set incoming data channel
    pomelo_webrtc_channel_set_incoming_data_channel(channel, dc);
}


#define POMELO_CLIENT_CHANNEL_PREFIX_LENGTH                                    \
    (sizeof(POMELO_CLIENT_CHANNEL_PREFIX) - 1)
static bool pomelo_webrtc_pc_parse_channel_label(
    const char * label,
    size_t * channel_index
) {
    size_t label_len = strlen(label);
    if (label_len <= POMELO_CLIENT_CHANNEL_PREFIX_LENGTH) {
        return false;
    }

    int cmp_ret = memcmp(
        label,
        POMELO_CLIENT_CHANNEL_PREFIX,
        POMELO_CLIENT_CHANNEL_PREFIX_LENGTH
    );
    if (cmp_ret != 0) {
        return false; // Invalid channel name
    }

    size_t index = 0;
    for (size_t i = POMELO_CLIENT_CHANNEL_PREFIX_LENGTH; i < label_len; i++) {
        char c = label[i];
        if (c < '0' || c > '9') {
            return false;
        }

        index = index * 10 + (c - '0');
    }

    *channel_index = index;
    return true;
}


void pomelo_webrtc_pc_on_data_channel(
    rtc_peer_connection_t * pc,
    rtc_data_channel_t * dc
) {
    assert(pc != NULL);
    assert(dc != NULL);
    
    rtc_context_t * rtc_context = rtc_peer_connection_get_context(pc);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }

    const char * label = rtc_data_channel_get_label(dc);
    size_t channel_index = 0;
    bool valid = pomelo_webrtc_pc_parse_channel_label(label, &channel_index);

    pomelo_webrtc_variant_t args[] = {
        { .ptr = pc },
        { .b = valid },
        { .size = channel_index },
        { .ptr = dc }
    };

    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_pc_on_data_channel_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


/* -------------------------------------------------------------------------- */
/*                                 Public APIs                                */
/* -------------------------------------------------------------------------- */

int pomelo_webrtc_session_pc_init(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    rtc_peer_connection_options_t options;
    pomelo_webrtc_session_pc_init_rtc_pc_options(&options);

    // Set associated data
    options.context = session->context->rtc_context;
    options.data = session;

    // Create new connection
    session->pc = rtc_peer_connection_create(&options);
    if (!session->pc) {
        // Failed to create new peer connection,
        return -1;
    }

    pomelo_webrtc_session_pc_set_active(session);
    // PC references the session until PC is closed
    pomelo_webrtc_session_ref(session);
    return 0;
}


void pomelo_webrtc_session_pc_finalize(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    if (session->pc) {
        rtc_peer_connection_destroy(session->pc);
        session->pc = NULL;
    }
}


void pomelo_webrtc_session_pc_close(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    if (!pomelo_webrtc_session_pc_is_active(session)) {
        return;
    }
    pomelo_webrtc_session_pc_unset_active(session);
    rtc_peer_connection_close(session->pc);
}


int pomelo_webrtc_session_pc_address(
    pomelo_webrtc_session_t * session,
    pomelo_address_t * address
) {
    assert(session != NULL);
    assert(address != NULL);

    // Parse address
    char address_str[ADDRESS_BUFFER_LENGTH];
    rtc_peer_connection_remote_address(
        session->pc,
        address_str,
        ADDRESS_BUFFER_LENGTH
    );

    return pomelo_address_from_string(address, address_str);
}


void pomelo_webrtc_session_pc_negotiate(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    // Create offer by passing NULL to description type
    rtc_peer_connection_set_local_description(session->pc, NULL);

    // Send the local description
    pomelo_webrtc_session_send_local_description(
        session,
        rtc_peer_connection_get_local_description_sdp(session->pc),
        rtc_peer_connection_get_local_description_type(session->pc)
    );
}


void pomelo_webrtc_session_pc_set_remote_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
) {
    assert(session != NULL);
    rtc_peer_connection_set_remote_description(session->pc, sdp, type);
}


void pomelo_webrtc_session_pc_add_remote_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
) {
    assert(session != NULL);
    rtc_peer_connection_add_remote_candidate(session->pc, cand, mid);
}


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_session_pc_init_rtc_pc_options(
    rtc_peer_connection_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(rtc_peer_connection_options_t));

    // TODO: Load RTC configuration from file or somewhere
}


void pomelo_webrtc_session_pc_on_connected(pomelo_webrtc_session_t * session) {
    (void) session;
    // Nothing to do
}


void pomelo_webrtc_session_pc_on_closed(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    pomelo_webrtc_session_close(session);
    pomelo_webrtc_session_unref(session);
}
