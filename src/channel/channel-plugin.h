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
    pomelo_webrtc_recv_command_t * command
);


void POMELO_PLUGIN_CALL pomelo_webrtc_plugin_session_send(
    pomelo_plugin_t * plugin,
    pomelo_session_t * native_session,
    size_t channel_index,
    pomelo_message_t * message
);



/* -------------------------------------------------------------------------- */
/*                            Private APIs                                    */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_plugin_channel_receive(
    pomelo_plugin_t * plugin,
    pomelo_webrtc_recv_command_t * command
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_WEBRTC_CHANNEL_PLUGIN_H
