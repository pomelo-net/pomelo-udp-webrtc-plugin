#include <assert.h>
#include <string.h>
#include "pomelo/constants.h"
#include "pomelo/base64.h"
#include "utils/common-macro.h"
#include "utils/string-buffer.h"
#include "context.h"
#include "socket/socket.h"
#include "session-ws.h"


#define POMELO_CONNECT_TOKEN_BASE64_LENGTH                                     \
(pomelo_base64_calc_encoded_length(POMELO_CONNECT_TOKEN_BYTES) - 1)

#define POMELO_CONNECT_TOKEN_BASE64_NO_PADDING_LENGTH                          \
(pomelo_base64_calc_encoded_no_padding_length(POMELO_CONNECT_TOKEN_BYTES) - 1)

/// Compare head of message with opcode
#define opcode_cmp(message, opcode)                                            \
    (memcmp((message), (opcode), sizeof(opcode) - 1) == 0)


#define pomelo_webrtc_session_ws_is_active(session)                            \
POMELO_CHECK_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_WS_ACTIVE)


#define pomelo_webrtc_session_ws_is_authenticated(session)                     \
POMELO_CHECK_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_WS_AUTHENTICATED)


#define pomelo_webrtc_session_ws_set_active(session)                           \
POMELO_SET_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_WS_ACTIVE)


#define pomelo_webrtc_session_ws_unset_active(session)                         \
POMELO_UNSET_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_WS_ACTIVE)


#define pomelo_webrtc_session_ws_set_authenticated(session)                    \
POMELO_SET_FLAG((session)->flags, POMELO_WEBRTC_SESSION_FLAG_WS_AUTHENTICATED)


/* -------------------------------------------------------------------------- */
/*                                WS Callbacks                                */
/* -------------------------------------------------------------------------- */


void pomelo_webrtc_ws_on_open(rtc_websocket_client_t * ws_client) {
    (void) ws_client;
    // Just wait for AUTH MESSAGE from client
}


static void pomelo_webrtc_ws_on_closed_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 1);
    assert(args != NULL);

    rtc_websocket_client_t * ws_client = args[0].ptr;
    pomelo_webrtc_session_t * session =
        rtc_websocket_client_get_data(ws_client);
    if (!session) return; // Not a mapped session

    pomelo_webrtc_session_ws_on_closed(session);
}


void pomelo_webrtc_ws_on_closed(rtc_websocket_client_t * ws_client) {
    assert(ws_client != NULL);
    rtc_context_t * rtc_context = rtc_websocket_client_get_context(ws_client);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }

    pomelo_webrtc_variant_t args[] = {{ .ptr = ws_client }};
    pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_ws_on_closed_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
}


void pomelo_webrtc_ws_on_error(
    rtc_websocket_client_t * ws_client,
    const char * error
) {
    (void) ws_client;
    pomelo_webrtc_log_debug(error);
    // We will receive close event after this event
}


static void pomelo_webrtc_ws_on_message_callback(
    size_t argc,
    pomelo_webrtc_variant_t * args
) {
    assert(argc == 2);
    assert(args != NULL);

    rtc_websocket_client_t * ws_client = args[0].ptr;
    rtc_buffer_t * message = args[1].ptr;
    pomelo_webrtc_session_t * session =
        rtc_websocket_client_get_data(ws_client);

    if (session && pomelo_webrtc_session_ws_is_active(session)) {
        // Only accept message when WS is active

        size_t size = rtc_buffer_size(message);
        const char * data = (const char *) rtc_buffer_data(message);
        pomelo_webrtc_session_ws_process_message(session, data, size);
    }

    rtc_buffer_unref(message); // Unref buffer after calling callback
}


void pomelo_webrtc_ws_on_message(
    rtc_websocket_client_t * ws_client,
    rtc_buffer_t * message
) {
    assert(ws_client != NULL);
    rtc_context_t * rtc_context = rtc_websocket_client_get_context(ws_client);
    pomelo_webrtc_context_t * context = rtc_context_get_data(rtc_context);
    if (!context) {
        return;
    }

    pomelo_webrtc_variant_t args[] = {
        { .ptr = ws_client },
        { .ptr = message }
    };

    // Increase ref count of message
    rtc_buffer_ref(message);
    pomelo_webrtc_task_t * task = pomelo_webrtc_context_submit_task(
        context,
        pomelo_webrtc_ws_on_message_callback,
        POMELO_ARRAY_LENGTH(args),
        args
    );
    if (!task) {
        // Failed to submit task
        rtc_buffer_unref(message);
    }
}

/* -------------------------------------------------------------------------- */
/*                                 Public APIs                                */
/* -------------------------------------------------------------------------- */

int pomelo_webrtc_session_ws_init(
    pomelo_webrtc_session_t * session,
    rtc_websocket_client_t * ws_client
) {
    assert(session != NULL);
    assert(ws_client != NULL);

    session->ws_client = ws_client;
    pomelo_webrtc_session_ws_set_active(session);
    
    // Init websocket client
    rtc_websocket_client_set_data(ws_client, session);

    // WSC references the session until WSC is closed
    pomelo_webrtc_session_ref(session);
    return 0;
}


void pomelo_webrtc_session_ws_cleanup(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    // Delete the websocket & peer connection
    if (session->ws_client) {
        rtc_websocket_client_destroy(session->ws_client);
        session->ws_client = NULL;
    }
}


void pomelo_webrtc_session_ws_close(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    if (!pomelo_webrtc_session_ws_is_active(session)) {
        return;
    }
    pomelo_webrtc_session_ws_unset_active(session);

    rtc_websocket_client_close(session->ws_client);
    // => pomelo_webrtc_session_ws_on_closed
}


void pomelo_webrtc_session_ws_send_description(
    pomelo_webrtc_session_t * session,
    const char * sdp,
    const char * type
) {
    assert(session != NULL);
    if (!pomelo_webrtc_session_ws_is_active(session)) {
        return; // WS is deactivated
    }

    pomelo_webrtc_context_t * context = session->context;
    pomelo_string_buffer_t * buffer =
        pomelo_webrtc_context_acquire_string_buffer(context);
    if (!buffer) return; // Cannot allocate buffer

    // Format: <opcode>|<type>|<sdp>
    pomelo_string_buffer_append_str(buffer, OPCODE_DESCRIPTION);
    pomelo_string_buffer_append_chr(buffer, MESSAGE_SEPARATOR);
    pomelo_string_buffer_append_str(buffer, type);
    pomelo_string_buffer_append_chr(buffer, MESSAGE_SEPARATOR);
    pomelo_string_buffer_append_str(buffer, sdp);

    const char * response = NULL;
    size_t response_length = 0;
    pomelo_string_buffer_to_binary(buffer, &response, &response_length);
    rtc_websocket_client_send_binary(
        session->ws_client,
        (const uint8_t *) response,
        response_length
    );

    // Finally release the buffer
    pomelo_webrtc_context_release_string_buffer(context, buffer);
}


void pomelo_webrtc_session_ws_send_candidate(
    pomelo_webrtc_session_t * session,
    const char * cand,
    const char * mid
) {
    assert(session != NULL);
    if (!pomelo_webrtc_session_ws_is_active(session)) {
        return; // WS is deactivated
    }

    pomelo_webrtc_context_t * context = session->context;
    pomelo_string_buffer_t * buffer =
        pomelo_webrtc_context_acquire_string_buffer(context);
    if (!buffer) return; // Cannot acquire new string buffer

    // Format: <opcode>|<mid>|<cand>
    pomelo_string_buffer_append_str(buffer, OPCODE_CANDIDATE);
    pomelo_string_buffer_append_chr(buffer, MESSAGE_SEPARATOR);
    pomelo_string_buffer_append_str(buffer, mid);
    pomelo_string_buffer_append_chr(buffer, MESSAGE_SEPARATOR);
    pomelo_string_buffer_append_str(buffer, cand);

    const char * response = NULL;
    size_t response_length = 0;
    pomelo_string_buffer_to_binary(buffer, &response, &response_length);
    rtc_websocket_client_send_binary(
        session->ws_client,
        (const uint8_t *) response,
        response_length
    );

    // Finally release the buffer
    pomelo_webrtc_context_release_string_buffer(context, buffer);
}


void pomelo_webrtc_session_ws_send_ready(pomelo_webrtc_session_t * session) {
    assert(session != NULL);
    if (!pomelo_webrtc_session_ws_is_active(session)) {
        return; // WS is deactivated
    }

    rtc_websocket_client_send_binary(
        session->ws_client,
        (const uint8_t *) OPCODE_READY,
        sizeof(OPCODE_READY) - 1
    );
}


void pomelo_webrtc_session_ws_send_connected(
    pomelo_webrtc_session_t * session
) {
    assert(session != NULL);
    if (!pomelo_webrtc_session_ws_is_active(session)) {
        return; // WS is deactivated
    }

    rtc_websocket_client_send_binary(
        session->ws_client,
        (const uint8_t *) OPCODE_CONNECTED,
        sizeof(OPCODE_CONNECTED) - 1
    );
}

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_session_ws_recv_auth(
    pomelo_webrtc_session_t * session,
    const char * auth,
    size_t auth_length
) {
    assert(session != NULL);
    assert(auth != NULL);

    if (
        auth_length != POMELO_CONNECT_TOKEN_BASE64_LENGTH &&
        auth_length != POMELO_CONNECT_TOKEN_BASE64_NO_PADDING_LENGTH
    ) {
        // Invalid auth token
        pomelo_webrtc_session_ws_auth_result(session, NULL);
        return;
    }

    pomelo_webrtc_context_t * context = session->context;
    assert(context != NULL);

    uint8_t connect_token[POMELO_CONNECT_TOKEN_BYTES] = { 0 };

    // Convert message to base64
    int ret = pomelo_base64_decode(
        connect_token,
        POMELO_CONNECT_TOKEN_BYTES,
        auth,
        auth_length
    );
    if (ret < 0) {
        // Failed to decode connect token
        pomelo_webrtc_session_ws_auth_result(session, NULL);
        return;
    }

    // Decode the connect token
    pomelo_plugin_t * plugin = context->plugin;
    int64_t client_id = 0;
    int32_t timeout = 0;
    pomelo_plugin_token_info_t info = { 0 };
    info.client_id = &client_id;
    info.timeout = &timeout;

    ret = plugin->connect_token_decode(
        plugin,
        session->socket->native_socket,
        connect_token,
        &info
    );

    if (ret < 0) {
        pomelo_webrtc_session_ws_auth_result(session, NULL);
        return;
    }

    session->client_id = client_id;
    pomelo_webrtc_session_ws_auth_result(session, &info);
}


void pomelo_webrtc_session_ws_auth_result(
    pomelo_webrtc_session_t * session,
    pomelo_plugin_token_info_t * info
) {
    assert(session != NULL);

    if (info) {
        // Set as authenticated
        pomelo_webrtc_session_ws_set_authenticated(session);
        pomelo_webrtc_session_ws_send_auth_success(session);
    } else {
        rtc_websocket_client_send_binary(
            session->ws_client,
            (const uint8_t *) RESULT_AUTH_FAILED,
            sizeof(RESULT_AUTH_FAILED) - 1
        );
    }

    pomelo_webrtc_session_on_auth_result(session, info);
}


void pomelo_webrtc_session_ws_send_auth_success(
    pomelo_webrtc_session_t * session
) {
    assert(session != NULL);

    pomelo_webrtc_context_t * context = session->context;
    pomelo_string_buffer_t * buffer =
        pomelo_webrtc_context_acquire_string_buffer(context);
    if (!buffer) return; // Cannot acquire new string buffer

    // Get server time
    pomelo_plugin_t * plugin = context->plugin;
    uint64_t time = plugin->socket_time(plugin, session->socket->native_socket);

    // Append the type
    pomelo_string_buffer_append_str(buffer, RESULT_AUTH_OK);
    pomelo_string_buffer_append_chr(buffer, MESSAGE_SEPARATOR);
    pomelo_string_buffer_append_i64(buffer, session->client_id);
    pomelo_string_buffer_append_chr(buffer, MESSAGE_SEPARATOR);
    pomelo_string_buffer_append_u64(buffer, time);

    const char * message = NULL;
    size_t length = 0;
    pomelo_string_buffer_to_binary(buffer, &message, &length);

    rtc_websocket_client_send_binary(
        session->ws_client,
        (const uint8_t *) message,
        length
    );

    // Finally release the buffer
    pomelo_webrtc_context_release_string_buffer(context, buffer);
}


void pomelo_webrtc_session_ws_process_message(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t size
) {
    assert(session != NULL);
    assert(message != NULL);

    if (pomelo_webrtc_session_ws_is_authenticated(session)) {
        pomelo_webrtc_session_ws_on_message_authenticated(
            session, message, size - 1
        );
    } else {
        pomelo_webrtc_session_ws_on_message_unauthenticated(
            session, message, size - 1
        );
    }
}


void pomelo_webrtc_session_ws_on_message_unauthenticated(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t message_length
) {
    assert(session != NULL);
    assert(message != NULL);

    if (message_length <= sizeof(OPCODE_AUTH)) {
        return;
    }

    if (!opcode_cmp(message, OPCODE_AUTH)) {
        return;
    }

    pomelo_webrtc_session_ws_recv_auth(
        session,
        message + sizeof(OPCODE_AUTH), // Ingore separator
        message_length - sizeof(OPCODE_AUTH) // Ingore separator
    );
}


void pomelo_webrtc_session_ws_on_message_authenticated(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t message_length
) {
    assert(session != NULL);
    assert(message != NULL);

    // Check description opcode
    if (opcode_cmp(message, OPCODE_DESCRIPTION)) {
        pomelo_webrtc_session_ws_process_description_message(
            session,
            message + sizeof(OPCODE_DESCRIPTION),
            message_length - sizeof(OPCODE_DESCRIPTION)
        );
        return;
    }

    // Check candidate opcode
    if (opcode_cmp(message, OPCODE_CANDIDATE)) {
        pomelo_webrtc_session_ws_process_candidate_message(
            session,
            message + sizeof(OPCODE_CANDIDATE),
            message_length - sizeof(OPCODE_CANDIDATE)
        );
        return;
    }

    // Check opcode ready
    if (opcode_cmp(message, OPCODE_READY)) {
        pomelo_webrtc_session_recv_ready(session);
        return;
    }
}


void pomelo_webrtc_session_ws_process_description_message(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t size
) {
    assert(session != NULL);
    assert(message != NULL);
    if (size == 0) {
        // Empty message
        return;
    }

    // Format: <type>|<sdp>
    const char * separator = strchr(message, MESSAGE_SEPARATOR);
    if (!separator) {
        return;
    }

    pomelo_webrtc_context_t * context = session->context;
    pomelo_string_buffer_t * type_buffer =
        pomelo_webrtc_context_acquire_string_buffer(context);
    if (!type_buffer) return; // Cannot acquire new string buffer

    // Append the type
    pomelo_string_buffer_append_bin(type_buffer, message, separator - message);

    const char * type = NULL;
    pomelo_string_buffer_to_string(type_buffer, &type, NULL);

    // We can reuse the remain part of message as sdp by skipping separator
    const char * sdp = separator + 1;

    pomelo_webrtc_session_recv_remote_description(session, sdp, type);

    // Finally, release string buffer
    pomelo_webrtc_context_release_string_buffer(context, type_buffer);
}


void pomelo_webrtc_session_ws_process_candidate_message(
    pomelo_webrtc_session_t * session,
    const char * message,
    size_t size
) {
    assert(session != NULL);
    assert(message != NULL);
    (void) size;

    // Format: <mid>|<cand>
    const char * separator = strchr(message, MESSAGE_SEPARATOR);
    if (!separator) {
        return;
    }

    pomelo_webrtc_context_t * context = session->context;
    pomelo_string_buffer_t * mid_buffer =
        pomelo_webrtc_context_acquire_string_buffer(context);
    if (!mid_buffer) return; // Cannot acquire new string buffer

    // Append the type
    pomelo_string_buffer_append_bin(mid_buffer, message, separator - message);

    const char * mid = NULL;
    pomelo_string_buffer_to_string(mid_buffer, &mid, NULL);

    // We can reuse the remain part of message as sdp by skipping separator
    const char * cand = separator + 1;

    pomelo_webrtc_session_recv_remote_candidate(session, cand, mid);

    // Finally, release string buffer
    pomelo_webrtc_context_release_string_buffer(context, mid_buffer);
}


void pomelo_webrtc_session_ws_on_closed(pomelo_webrtc_session_t * session) {
    assert(session != NULL);

    pomelo_webrtc_session_close(session);
    pomelo_webrtc_session_unref(session); // WS no longer references session
}
