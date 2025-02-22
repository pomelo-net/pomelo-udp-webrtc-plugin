#ifndef POMELO_WEBRTC_CHANNEL_POOL_H
#define POMELO_WEBRTC_CHANNEL_POOL_H
#include "channel.h"
#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                                Pool APIs                                   */
/* -------------------------------------------------------------------------- */

/// @brief Pool-compatible aquiring callback
int pomelo_webrtc_channel_acquire_callback(
    pomelo_webrtc_channel_t * channel,
    pomelo_webrtc_context_t * context
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_CHANNEL_POOL_H
