#include <assert.h>
#include "session-pool.h"
#include "context.h"


#define SESSIONS_INIT_CHANNELS_CAPACITY 256

/* -------------------------------------------------------------------------- */
/*                                Pool APIs                                   */
/* -------------------------------------------------------------------------- */

int pomelo_webrtc_session_alloc_callback(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_context_t * context
) {
    assert(session != NULL);
    assert(context != NULL);
    memset(session, 0, sizeof(pomelo_webrtc_session_t));
    pomelo_allocator_t * allocator = context->allocator;

    pomelo_array_options_t options;
    pomelo_array_options_init(&options);
    options.allocator = allocator;
    options.element_size = sizeof(pomelo_webrtc_channel_t *);
    options.initial_capacity = SESSIONS_INIT_CHANNELS_CAPACITY;

    session->channels = pomelo_array_create(&options);
    if (!session->channels) {
        return -1;
    }

    return 0;
}


int pomelo_webrtc_session_dealloc_callback(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_context_t * context
) {
    assert(session != NULL);
    (void) context;
    
    if (session->channels) {
        pomelo_array_destroy(session->channels);
        session->channels = NULL;
    }

    return 0;
}


int pomelo_webrtc_session_acquire_callback(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_context_t * context
) {
    assert(session != NULL);
    assert(context != NULL);

    session->context = context;
    session->flags = 0;
    session->socket = NULL;
    session->system_channel = NULL;
    session->list_node = NULL;
    session->opened_channels = 0;
    session->ping_task = NULL;
    pomelo_rtt_calculator_init(&session->rtt);
    session->native_session = NULL;
    session->client_id = 0;
    session->pc = NULL;
    session->ws_client = NULL;
    session->task_timeout = NULL;

    return pomelo_array_clear(session->channels);
}
