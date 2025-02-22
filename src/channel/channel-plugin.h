#ifndef POMELO_WEBRTC_CHANNEL_PLUGIN_H
#define POMELO_WEBRTC_CHANNEL_PLUGIN_H
#include "channel-int.h"
#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                           Plugin implementation                            */
/* -------------------------------------------------------------------------- */

void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_receive(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    size_t channel_index,
    void * callback_data,
    pomelo_message_t * message,
    pomelo_plugin_error_t error
);


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_send(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    size_t channel_index,
    pomelo_message_t * message
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_CHANNEL_PLUGIN_H
