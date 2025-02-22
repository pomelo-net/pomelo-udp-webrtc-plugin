#ifndef POMELO_WEBRTC_SESSION_POOL_H
#define POMELO_WEBRTC_SESSION_POOL_H
#include "session.h"
#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*                                Pool APIs                                   */
/* -------------------------------------------------------------------------- */

/// @brief Pool-compatible allocating callback
int pomelo_webrtc_session_alloc_callback(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_context_t * context
);

/// @brief Pool-compatible deallocating callback
int pomelo_webrtc_session_dealloc_callback(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_context_t * context
);

/// @brief Pool-compatible aquiring callback
int pomelo_webrtc_session_acquire_callback(
    pomelo_webrtc_session_t * session,
    pomelo_webrtc_context_t * context
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_SESSION_POOL_H
