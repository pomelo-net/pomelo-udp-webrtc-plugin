#include <assert.h>
#include <string.h>
#include "uv.h"
#include "base/payload.h"
#include "utils/common-macro.h"
#include "session.h"
#include "socket/socket.h"
#include "context.h"
#include "channel/channel.h"
#include "session-ws.h"
#include "session-pc.h"
#include "session-plugin.h"


// Ping inteval
#define PING_INTERVAL_MS 100 // ms, 10Hz

// 1 byte for header and 8 bytes for sequence
#define SYS_PING_DATA_CAPACITY 9

// 1 byte for header and 8 bytes for sequence and 8 bytes for time
#define SYS_PONG_DATA_CAPACITY 17

// Maximum number of ping sequence. After reaching this value, next sequence
// will be zero
#define MAX_PING_SEQUENCE_VALUE 0xFFFF

#define SYS_PING_MIN_LENGTH 2
#define SYS_PING_MAX_LENGTH SYS_PING_DATA_CAPACITY
#define SYS_PONG_MIN_LENGTH 3
#define SYS_PONG_MAX_LENGTH SYS_PONG_DATA_CAPACITY

#define SESSIONS_INIT_CHANNELS_CAPACITY 64

/// Activate the session
#define pomelo_webrtc_session_set_active(session)                              \
POMELO_SET_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_ACTIVE)


/// Deactivate the session
#define pomelo_webrtc_session_unset_active(session)                            \
POMELO_UNSET_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_ACTIVE)


#define pomelo_webrtc_session_is_connected(session)                            \
    (((session)->flags & POMELO_WEBRTC_SESSION_FLAG_CONNECTED) ==              \
        POMELO_WEBRTC_SESSION_FLAG_CONNECTED)


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


int pomelo_webrtc_session_on_alloc(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_context_t * context
) {
    assert(session != NULL);
    assert(context != NULL);
    session->context = context;

    pomelo_allocator_t * allocator = context->allocator;

    pomelo_array_options_t options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_webrtc_channel_t *),
        .initial_capacity = SESSIONS_INIT_CHANNELS_CAPACITY
    };

    session->channels = pomelo_array_create(&options);
    if (!session->channels) return -1;
    return 0;
}


void pomelo_webrtc_session_on_free(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    session->context = NULL;

    if (session->channels) {
        pomelo_array_destroy(session->channels);
        session->channels = NULL;
    }
}


int pomelo_webrtc_session_init(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_session_info_t * info
) {
    assert(session != NULL);
    assert(info != NULL);

    pomelo_webrtc_socket_t * socket = info->socket;
    rtc_websocket_client_t * ws_client = info->ws_client;

    // Initialize reference
    pomelo_reference_init(
        &session->ref,
        (pomelo_ref_finalize_cb) pomelo_webrtc_session_on_finalize
    );

    // Update properties and initialize websocket
    session->socket = socket;
    pomelo_webrtc_session_set_active(session);

    // New session references the socket
    pomelo_webrtc_socket_ref(socket);

    // Initialize Websocket
    int ret = pomelo_webrtc_session_ws_init(session, ws_client);
    if (ret < 0) return -1;

    // Initialize PC
    ret = pomelo_webrtc_session_pc_init(session);
    if (ret < 0) return -1;

    // Initialize session plugin
    ret = pomelo_webrtc_session_plugin_init(session);
    if (ret < 0) return -1;

    // Schedule for authenticating
    pomelo_webrtc_task_t * task =
        pomelo_webrtc_session_schedule_timeout(session, POMELO_AUTH_TIMEOUT_MS);
    if (!task) return -1;

    return 0;
}


void pomelo_webrtc_session_cleanup(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    pomelo_webrtc_socket_t * socket = session->socket;
    pomelo_webrtc_context_t * context = session->context;

    // Finalize all components
    pomelo_webrtc_session_ws_cleanup(session);
    pomelo_webrtc_session_pc_cleanup(session);
    pomelo_webrtc_session_plugin_cleanup(session);

    if (session->task_timeout) {
        pomelo_webrtc_context_unschedule_task(context, session->task_timeout);
        session->task_timeout = NULL;
    }

    pomelo_array_clear(session->channels);
    session->system_channel = NULL;
    session->flags = 0;
    session->socket = NULL;
    session->list_entry = NULL;
    session->opened_channels = 0;
    session->ping_task = NULL;
    pomelo_rtt_calculator_init(&session->rtt);
    session->client_id = 0;

    // This session no longer references the socket
    pomelo_webrtc_socket_unref(socket);
}


void pomelo_webrtc_session_close(pomelo_webrtc_session_t * session) {
    // Just request close all elements
    assert(session != NULL);

    if (!pomelo_webrtc_session_is_active(session)) {
        return; // Session is deactivated
    }
    pomelo_webrtc_session_unset_active(session);

    // Stop sending ping
    pomelo_webrtc_session_stop_ping(session);

    // Close all channels
    size_t nchannels = session->channels->size;
    for (size_t i = 0; i < nchannels; i++) {
        pomelo_webrtc_channel_t * channel = NULL;
        int ret = pomelo_array_get(session->channels, i, &channel);
        if (ret == 0 && channel) {
            pomelo_webrtc_channel_close(channel);
        }
    }
    if (session->system_channel) {
        pomelo_webrtc_channel_close(session->system_channel);
    }

    // Close WS and PC
    pomelo_webrtc_session_ws_close(session);
    pomelo_webrtc_session_pc_close(session);
    pomelo_webrtc_session_plugin_close(session);

    // Remove this session from socket
    pomelo_webrtc_socket_remove_session(session->socket, session);

    // Finally, unref itself
    pomelo_webrtc_session_unref(session);
}


void pomelo_webrtc_session_remove_channel(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_channel_t * channel
) {
    assert(session != NULL);
    assert(channel != NULL);

    void * channel_null = NULL;
    pomelo_array_set(session->channels, channel->index, channel_null);
}


void pomelo_webrtc_session_on_channel_opened(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_channel_t * channel
) {
    assert(session != NULL);
    (void) channel;

    // Include system channel
    if ((++session->opened_channels) == (session->channels->size + 1)) {
        pomelo_webrtc_session_on_all_channels_opened(session);
    }
}


void pomelo_webrtc_session_process_system_message(
    pomelo_webrtc_session_t * session,
    rtc_buffer_t * message,
    uint64_t recv_time
) {
    assert(session != NULL);
    assert(message != NULL);

    size_t length = rtc_buffer_size(message);
    if (length == 0) {
        return;
    }

    const uint8_t * data = rtc_buffer_data(message);
    uint8_t header_byte = data[0];

    uint8_t opcode = (header_byte >> 6);
    switch (opcode) {
        case SYS_OPCODE_PING:
            pomelo_webrtc_session_process_ping(
                session, data, length, recv_time
            );
            break;
        
        case SYS_OPCODE_PONG:
            pomelo_webrtc_session_process_pong(
                session, data, length, recv_time
            );
            break;
        
        default:
            break;
    }
}


void pomelo_webrtc_session_ref(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    pomelo_reference_ref(&session->ref);
}


void pomelo_webrtc_session_unref(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    pomelo_reference_unref(&session->ref);
}


/* -------------------------------------------------------------------------- */
/*                               Module APIs                                  */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_session_on_auth_result(
    pomelo_webrtc_session_t * session,
    pomelo_plugin_token_info_t * info
) {
    assert(session != NULL);

    // Clear auth timeout
    pomelo_webrtc_session_unschedule_timeout(session);

    if (!info) {
        pomelo_webrtc_session_close(session);
        return; // Authenciating failed
    }

    // Initialize channels
    if (pomelo_webrtc_session_create_channels(session) < 0) {
        pomelo_webrtc_session_close(session);
        return; // Failed to init channels
    }

    // Schedule negotiating
    int32_t timeout = *info->timeout;
    if (timeout > 0) {
        pomelo_webrtc_task_t * task =
            pomelo_webrtc_session_schedule_timeout(session, 1000ULL * timeout);
        if (!task) {
            pomelo_webrtc_session_close(session);
            return; // Failed to create timeout
        }
    }

    // Start negotiating
    pomelo_webrtc_session_pc_negotiate(session);
}


void pomelo_webrtc_session_send_local_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
) {
    assert(session != NULL);
    pomelo_webrtc_session_ws_send_candidate(session, cand, mid);
}


void pomelo_webrtc_session_send_local_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
) {
    assert(session != NULL);
    pomelo_webrtc_session_ws_send_description(session, sdp, type);
}


void pomelo_webrtc_session_recv_remote_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
) {
    assert(session != NULL);
    pomelo_webrtc_session_pc_set_remote_description(session, sdp, type);
}


void pomelo_webrtc_session_recv_remote_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
) {
    assert(session != NULL);
    pomelo_webrtc_session_pc_add_remote_candidate(session, cand, mid);
}


void pomelo_webrtc_session_recv_ready(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    if (session->flags & POMELO_WEBRTC_SESSION_FLAG_READY_SIGNAL_RECEIVED) {
        return; // Already received ready signal
    }

    session->flags |= POMELO_WEBRTC_SESSION_FLAG_READY_SIGNAL_RECEIVED;
    if (pomelo_webrtc_session_is_connected(session)) {
        pomelo_webrtc_session_on_ready(session);
    }
}

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_session_on_finalize(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    // Release the session
    pomelo_webrtc_context_release_session(session->context, session);
}


static void pomelo_webrtc_session_ping_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 1);
    pomelo_webrtc_session_send_ping(args[0].ptr);
}


void pomelo_webrtc_session_start_ping(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    pomelo_webrtc_variant_t args[] = {
        { .ptr = session }
    };

    // Schedule task
    session->ping_task = pomelo_webrtc_context_schedule_task(
        session->context,
        pomelo_webrtc_session_ping_callback,
        /* argc = */ 1,
        args,
        PING_INTERVAL_MS
    );
}


void pomelo_webrtc_session_stop_ping(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    if (session->ping_task) {
        pomelo_webrtc_context_unschedule_task(
            session->context,
            session->ping_task
        );
        session->ping_task = NULL;
    }
}


void pomelo_webrtc_session_on_all_channels_opened(
    pomelo_webrtc_session_t * session
) {
    assert(session != NULL);

    // Send ready and start ping
    pomelo_webrtc_session_ws_send_ready(session);
    pomelo_webrtc_session_start_ping(session);

    session->flags |= POMELO_WEBRTC_SESSION_FLAG_ALL_CHANNELS_OPENED;
    if (pomelo_webrtc_session_is_connected(session)) {
        pomelo_webrtc_session_on_ready(session);
    }
}


void pomelo_webrtc_session_send_ping(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    uint64_t now = uv_hrtime();
    pomelo_rtt_entry_t * entry =
        pomelo_rtt_calculator_next_entry(&session->rtt, now);

    uint64_t ping_sequence = entry->sequence;
    size_t bytes = pomelo_payload_calc_packed_uint64_bytes(ping_sequence);

    uint8_t data[SYS_PING_DATA_CAPACITY];
    pomelo_payload_t payload;

    payload.position = 0;
    payload.capacity = SYS_PING_DATA_CAPACITY;
    payload.data = data;

    // Build the header byte
    uint8_t header = (
        (SYS_OPCODE_PING << 6) |
        (((bytes - 1) & 0x07) << 3)
    );

    // Header byte
    pomelo_payload_write_uint8_unsafe(&payload, header);

    // Sequence number of ping
    pomelo_payload_write_packed_uint64_unsafe(&payload, bytes, ping_sequence);

    // Send data
    pomelo_webrtc_channel_send(session->system_channel, data, bytes + 1);
}


void pomelo_webrtc_session_send_pong(
    pomelo_webrtc_session_t * session,
    uint64_t pong_sequence,
    uint64_t recv_time,
    uint64_t socket_time
) {
    assert(session != NULL);
    (void) recv_time;

    size_t sequence_bytes =
        pomelo_payload_calc_packed_uint64_bytes(pong_sequence);
    size_t socket_time_bytes =
        pomelo_payload_calc_packed_uint64_bytes(socket_time);

    uint8_t data[SYS_PONG_DATA_CAPACITY];
    pomelo_payload_t payload;

    payload.position = 0;
    payload.capacity = SYS_PONG_DATA_CAPACITY;
    payload.data = data;

    // Build the header byte
    uint8_t header_byte = (
        (SYS_OPCODE_PONG << 6) |
        (((sequence_bytes - 1) & 0x07) << 3) |
        ((socket_time_bytes - 1) & 0x07)
    );

    // Header byte
    pomelo_payload_write_uint8_unsafe(&payload, header_byte);

    // Sequence number of ping
    pomelo_payload_write_packed_uint64_unsafe(
        &payload, sequence_bytes, pong_sequence
    );

    // Socket time
    pomelo_payload_write_packed_uint64_unsafe(
        &payload, socket_time_bytes, socket_time
    );

    // Send data
    pomelo_webrtc_channel_send(
        session->system_channel,
        data,
        1 + sequence_bytes + socket_time_bytes
    );
}


void pomelo_webrtc_session_recv_pong(
    pomelo_webrtc_session_t * session,
    uint64_t ping_sequence,
    uint64_t recv_time
) {
    assert(session != NULL);

    pomelo_rtt_entry_t * entry =
        pomelo_rtt_calculator_entry(&session->rtt, ping_sequence);
    if (!entry) {
        return; // Entry does not exist
    }

    pomelo_rtt_calculator_submit_entry(&session->rtt, entry, recv_time, 0);
}


void pomelo_webrtc_session_process_ping(
    pomelo_webrtc_session_t * session,
    const uint8_t * message,
    size_t length,
    uint64_t recv_time
) {
    assert(session != NULL);
    assert(message != NULL);

    if (length < SYS_PING_MIN_LENGTH || length > SYS_PING_MAX_LENGTH) {
        return; // Invalid length, discard
    }

    uint8_t header_byte = message[0];
    size_t sequence_bytes = ((header_byte >> 3) & 0x07) + 1;

    pomelo_payload_t payload;
    payload.capacity = length - 1; // Skip header
    payload.position = 0;
    payload.data = (uint8_t *) (message + 1); // Skip header

    uint64_t ping_sequence = 0;
    int ret = pomelo_payload_read_packed_uint64(
        &payload,
        sequence_bytes,
        &ping_sequence
    );
    if (ret < 0) return; // Failed to decode sequence

    // Get the current socket time
    pomelo_plugin_t * plugin = session->context->plugin;
    uint64_t socket_time =
        plugin->socket_time(plugin, session->socket->native_socket);

    pomelo_webrtc_session_send_pong(
        session,
        ping_sequence,
        recv_time,
        socket_time
    );
}


void pomelo_webrtc_session_process_pong(
    pomelo_webrtc_session_t * session,
    const uint8_t * message,
    size_t length,
    uint64_t recv_time
) {
    assert(session != NULL);
    assert(message != NULL);

    if (length < SYS_PONG_MIN_LENGTH || length > SYS_PONG_MAX_LENGTH) {
        return; // Invalid length, discard
    }

    uint8_t header_byte = message[0];
    size_t sequence_bytes = ((header_byte >> 3) & 0x07) + 1;

    pomelo_payload_t payload;
    payload.capacity = length - 1;
    payload.position = 0;
    payload.data = (uint8_t *) (message + 1); // Skip header

    uint64_t ping_sequence = 0;
    int ret = pomelo_payload_read_packed_uint64(
        &payload,
        sequence_bytes,
        &ping_sequence
    );
    if (ret < 0) return; // Failed to decode sequence

    // There may be client time at the end of payload, but it is ignored.
    pomelo_webrtc_session_recv_pong(session, ping_sequence, recv_time);
}


int pomelo_webrtc_session_create_channels(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    // Create channels
    pomelo_webrtc_socket_t * socket = session->socket;
    pomelo_webrtc_context_t * context = session->context;
    pomelo_array_t * channels = session->channels;
    pomelo_array_t * modes = socket->channel_modes;
    size_t nchannels = modes->size;

    // Resize the channels array
    int ret = pomelo_array_resize(channels, nchannels);
    if (ret < 0) return -1; // Failed to resize the channels array

    // Fill the channels array with NULL
    pomelo_array_fill_zero(channels);

    pomelo_webrtc_channel_t * channel;
    pomelo_channel_mode mode;
    pomelo_webrtc_channel_info_t info;
    info.session = session;

    // Create channels
    for (size_t i = 0; i < nchannels; i++) {
        pomelo_array_get(modes, i, &mode);
        info.channel_index = i;
        info.channel_mode = mode;
        channel = pomelo_webrtc_context_acquire_channel(context, &info);
        if (!channel) return -1;

        pomelo_array_set(channels, i, channel);
    }

    // Create system channel
    info.channel_index = POMELO_WEBRTC_CHANNEL_SYSTEM_INDEX;
    info.channel_mode = POMELO_CHANNEL_MODE_UNRELIABLE;
    session->system_channel =
        pomelo_webrtc_context_acquire_channel(context, &info);
    if (!session->system_channel) return -1; // Failed to create system channel

    // Wait for opened channels
    // => pomelo_webrtc_session_on_all_channels_opened

    return 0;
}


void pomelo_webrtc_session_on_ready(pomelo_webrtc_session_t * session) {
    // Clear connect timeout scheduler
    pomelo_webrtc_session_unschedule_timeout(session);

    // Update the address of this session
    pomelo_webrtc_session_pc_address(session, &session->address);

    // Open native session
    pomelo_webrtc_session_plugin_open(session);
    // => pomelo_webrtc_session_on_connected
}


void pomelo_webrtc_session_on_connected(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    pomelo_array_t * channels = session->channels;
    size_t nchannels = channels->size;
    pomelo_webrtc_channel_t * channel;
    for (size_t i = 0; i < nchannels; i++) {
        // Accept incoming messages
        pomelo_array_get(channels, i, &channel);
        pomelo_webrtc_channel_enable_receiving(channel);
    }
    pomelo_webrtc_channel_enable_receiving(session->system_channel);

    // Send ready message
    pomelo_webrtc_session_ws_send_connected(session);
    // Well done, we have the connection
}


void pomelo_webrtc_session_on_timeout(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 1);
    assert(args != NULL);

    pomelo_webrtc_session_t * session = args[0].ptr;
    assert(session != NULL);
    pomelo_webrtc_session_unschedule_timeout(session);

    pomelo_webrtc_session_close(session);
}


pomelo_webrtc_task_t * pomelo_webrtc_session_schedule_timeout(
    pomelo_webrtc_session_t * session,
    uint64_t timeout_ms
) {
    assert(session != NULL);
    pomelo_webrtc_variant_t args[] = {{ .ptr = session }};
    session->task_timeout = pomelo_webrtc_context_schedule_task(
        session->context,
        pomelo_webrtc_session_on_timeout,
        POMELO_ARRAY_LENGTH(args),
        args,
        timeout_ms
    );

    return session->task_timeout;
}


void pomelo_webrtc_session_unschedule_timeout(
    pomelo_webrtc_session_t * session
) {
    assert(session != NULL);
    if (!session->task_timeout) {
        return;
    }

    pomelo_webrtc_context_unschedule_task(
        session->context,
        session->task_timeout
    );

    session->task_timeout = NULL;
}
