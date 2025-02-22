#include <assert.h>
#include "channel-pool.h"


int pomelo_webrtc_channel_acquire_callback(
    pomelo_webrtc_channel_t * channel,
    pomelo_webrtc_context_t * context
) {
    assert(channel != NULL);
    assert(context != NULL);

    channel->context = context;
    return 0;
}
