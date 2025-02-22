#include <cassert>
#include <cstring>
#include <atomic>
#include "rtc/rtc.hpp"
#include "utils/list.h"
#include "rtc-api.h"
#include "rtc-buffer.hpp"
#include "rtc-object-pool.hpp"
#include "rtc-object.hpp"
#include "rtc-peer-connection.hpp"
#include "rtc-ws-client.hpp"
#include "rtc-ws-server.hpp"
#include "rtc-data-channel.hpp"
#include "rtc-buffer-pool.hpp"
#include "rtc-context.hpp"


using namespace std::chrono_literals;
using namespace rtc_api;



/* -------------------------------------------------------------------------- */
/*                               Common APIs                                  */
/* -------------------------------------------------------------------------- */

rtc_context_t * rtc_context_create(rtc_options_t * options) {
    assert(options != nullptr);
    return reinterpret_cast<rtc_context_t *>(
        new (std::nothrow) RTCContext(options)
    );
}


void rtc_context_destroy(rtc_context_t * context) {
    assert(context != nullptr);
    delete reinterpret_cast<RTCContext *>(context);
}


void rtc_context_set_data(rtc_context_t * context, void * data) {
    assert(context != nullptr);
    reinterpret_cast<RTCContext *>(context)->set_data(data);
}


void * rtc_context_get_data(rtc_context_t * context) {
    assert(context != nullptr);
    return reinterpret_cast<RTCContext *>(context)->get_data();
}


/* -------------------------------------------------------------------------- */
/*                          Websocket Server APIs                             */
/* -------------------------------------------------------------------------- */

void rtc_websocket_server_options_init(
    rtc_websocket_server_options_t * options
) {
    assert(options != nullptr);
    memset(options, 0, sizeof(rtc_websocket_server_options_t));
}


rtc_websocket_server_t * rtc_websocket_server_create(
    rtc_websocket_server_options_t * options
) {
    assert(options != nullptr);
    if (!options->context) {
        return nullptr;
    }

    auto context = reinterpret_cast<RTCContext *>(options->context);
    auto wss = context->pool_wsserver->acquire();
    if (!wss) {
        return nullptr;
    }

    try {
        wss->init(options);
    } catch (std::exception ex) {
        context->handle_exception(ex);
        context->pool_wsserver->release(wss);
        return nullptr;
    }

    return reinterpret_cast<rtc_websocket_server_t *>(wss);
}


void rtc_websocket_server_close(rtc_websocket_server_t * wss) {
    assert(wss != nullptr);
    auto wss_object = reinterpret_cast<RTCWSServer *>(wss);
    wss_object->close();
}


void rtc_websocket_server_destroy(rtc_websocket_server_t * wss) {
    assert(wss != nullptr);
    auto wss_object = reinterpret_cast<RTCWSServer *>(wss);
    auto context = wss_object->context;

    wss_object->finalize();
    context->pool_wsserver->release(wss_object);
}


void rtc_websocket_server_set_data(rtc_websocket_server_t * wss, void * data) {
    assert(wss != nullptr);
    reinterpret_cast<RTCWSServer *>(wss)->set_data(data);
}


void * rtc_websocket_server_get_data(rtc_websocket_server_t * wss) {
    assert(wss != nullptr);
    return reinterpret_cast<RTCWSServer *>(wss)->get_data();
}


rtc_context_t * rtc_websocket_server_get_context(rtc_websocket_server_t * wss) {
    assert(wss != nullptr);
    return reinterpret_cast<rtc_context_t *>(
        reinterpret_cast<RTCWSServer *>(wss)->context
    );
}


/* -------------------------------------------------------------------------- */
/*                          Websocket Client APIs                             */
/* -------------------------------------------------------------------------- */


void rtc_websocket_client_set_data(rtc_websocket_client_t * wsc, void * data) {
    assert(wsc != nullptr);
    reinterpret_cast<RTCWSClient *>(wsc)->set_data(data);
}


void * rtc_websocket_client_get_data(rtc_websocket_client_t * wsc) {
    assert(wsc != nullptr);
    return reinterpret_cast<RTCWSClient *>(wsc)->get_data();
}


void rtc_websocket_client_close(rtc_websocket_client_t * wsc) {
    assert(wsc != nullptr);
    reinterpret_cast<RTCWSClient *>(wsc)->close();
}


void rtc_websocket_client_destroy(rtc_websocket_client_t * wsc) {
    assert(wsc != nullptr);
    auto wsc_object = reinterpret_cast<RTCWSClient *>(wsc);
    auto context = wsc_object->context;

    wsc_object->finalize();
    context->pool_wsclient->release(wsc_object);
}


void rtc_websocket_client_remote_address(
    rtc_websocket_client_t * wsc,
    char * buffer,
    int size
) {
    assert(wsc != nullptr);
    reinterpret_cast<RTCWSClient *>(wsc)->remote_address(buffer, size);
}


int rtc_websocket_client_send_string(
    rtc_websocket_client_t * wsc,
    const char * message
) {
    assert(wsc != nullptr);
    assert(message != nullptr);
    return reinterpret_cast<RTCWSClient *>(wsc)->send(message) ? 0 : -1;
}


/// @brief Send binary message over websocket
int rtc_websocket_client_send_binary(
    rtc_websocket_client_t * wsc,
    const uint8_t * message,
    size_t length
) {
    assert(wsc != nullptr);
    assert(message != nullptr);
    return reinterpret_cast<RTCWSClient *>(wsc)->send(message, length) ? 0 : -1;
}


rtc_context_t * rtc_websocket_client_get_context(rtc_websocket_client_t * wsc) {
    assert(wsc != nullptr);
    return reinterpret_cast<rtc_context_t *>(
        reinterpret_cast<RTCWSClient *>(wsc)->context
    );
}


/* -------------------------------------------------------------------------- */
/*                           Peer Connection APIs                             */
/* -------------------------------------------------------------------------- */

void rtc_peer_connection_options_init(rtc_peer_connection_options_t * options) {
    assert(options != nullptr);
    memset(options, 0, sizeof(rtc_peer_connection_options_t));
}


/// @brief Create new peer connection
rtc_peer_connection_t * rtc_peer_connection_create(
    rtc_peer_connection_options_t * options
) {
    assert(options != nullptr);
    if (!options->context) {
        return nullptr;
    }
    auto context = reinterpret_cast<RTCContext *>(options->context);
    auto pc = context->pool_pc->acquire();
    if (!pc) {
        return nullptr;
    }

    try {
        pc->init(options);
    } catch (std::exception ex) {
        context->handle_exception(ex);
        context->pool_pc->release(pc);
        return nullptr;
    }

    return reinterpret_cast<rtc_peer_connection_t *>(pc);
}


void rtc_peer_connection_destroy(rtc_peer_connection_t * pc) {
    assert(pc != nullptr);
    auto pc_object = reinterpret_cast<RTCPeerConnection *>(pc);
    auto context = pc_object->context;

    pc_object->finalize();
    context->pool_pc->release(pc_object);
}


void rtc_peer_connection_close(rtc_peer_connection_t * pc) {
    assert(pc != nullptr);
    reinterpret_cast<RTCPeerConnection *>(pc)->close();
}


void rtc_peer_connection_set_data(rtc_peer_connection_t * pc, void * data) {
    assert(pc != nullptr);
    reinterpret_cast<RTCPeerConnection *>(pc)->set_data(data);
}


void * rtc_peer_connection_get_data(rtc_peer_connection_t * pc) {
    assert(pc != nullptr);
    return reinterpret_cast<RTCPeerConnection *>(pc)->get_data();
}


void rtc_peer_connection_remote_address(
    rtc_peer_connection_t * pc,
    char * buffer,
    int size
) {
    assert(pc != nullptr);
    reinterpret_cast<RTCPeerConnection *>(pc)->remote_address(buffer, size);
}


rtc_data_channel_t * rtc_peer_connection_create_data_channel(
    rtc_peer_connection_t * pc,
    rtc_data_channel_options_t * options
) {
    assert(pc != nullptr);
    assert(options != nullptr);

    return reinterpret_cast<rtc_data_channel_t *>(
        reinterpret_cast<RTCPeerConnection *>(pc)->create_data_channel(options)
    );
}


void rtc_peer_connection_set_local_description(
    rtc_peer_connection_t * pc,
    const char * type
) {
    assert(pc != nullptr);
    reinterpret_cast<RTCPeerConnection *>(pc)->set_local_description(type);
}


const char * rtc_peer_connection_get_local_description_type(
    rtc_peer_connection_t * pc
) {
    assert(pc != nullptr);
    auto rtc_pc = reinterpret_cast<RTCPeerConnection *>(pc);
    return rtc_pc->get_local_description_type();
}


/// @brief Get SDP of local description
const char * rtc_peer_connection_get_local_description_sdp(
    rtc_peer_connection_t * pc
) {
    assert(pc != nullptr);
    auto rtc_pc = reinterpret_cast<RTCPeerConnection *>(pc);
    return rtc_pc->get_local_description_sdp();
}


void rtc_peer_connection_set_remote_description(
    rtc_peer_connection_t * pc,
    const char * sdp,
    const char * type
) {
    assert(pc != nullptr);
    assert(sdp != nullptr);
    assert(type != nullptr);

    reinterpret_cast<RTCPeerConnection *>(pc)->set_remote_description(sdp, type);
}


void rtc_peer_connection_add_remote_candidate(
    rtc_peer_connection_t * pc,
    const char * cand,
    const char * mid
) {
    assert(pc != nullptr);
    assert(cand != nullptr);
    assert(mid != nullptr);

    reinterpret_cast<RTCPeerConnection *>(pc)->add_remote_candidate(cand, mid);
}


rtc_context_t * rtc_peer_connection_get_context(rtc_peer_connection_t * pc) {
    assert(pc != nullptr);
    return reinterpret_cast<rtc_context_t *>(
        reinterpret_cast<RTCPeerConnection *>(pc)->context
    );
}


/* -------------------------------------------------------------------------- */
/*                            Data channel APIs                               */
/* -------------------------------------------------------------------------- */

void rtc_data_channel_options_init(rtc_data_channel_options_t * options) {
    assert(options != nullptr);
    memset(options, 0, sizeof(rtc_data_channel_options_t));
}


void rtc_data_channel_destroy(rtc_data_channel_t * dc) {
    assert(dc != nullptr);
    auto dc_object = reinterpret_cast<RTCDataChannel *>(dc);
    auto context = dc_object->context;

    dc_object->finalize();
    context->pool_dc->release(dc_object);
}


void rtc_data_channel_close(rtc_data_channel_t * dc) {
    assert(dc != nullptr);
    reinterpret_cast<RTCDataChannel *>(dc)->close();
}


void rtc_data_channel_set_data(rtc_data_channel_t * dc, void * data) {
    assert(dc != nullptr);
    reinterpret_cast<RTCDataChannel *>(dc)->set_data(data);
}


void * rtc_data_channel_get_data(rtc_data_channel_t * dc) {
    assert(dc != nullptr);
    return reinterpret_cast<RTCDataChannel *>(dc)->get_data();
}


int rtc_data_channel_send(
    rtc_data_channel_t * dc,
    const uint8_t * message,
    size_t length
) {
    assert(dc != nullptr);
    assert(message != nullptr);
    bool result = reinterpret_cast<RTCDataChannel *>(dc)->send(message, length);
    return result ? 0 : -1;
}


int rtc_data_channel_send_buffer(
    rtc_data_channel_t * dc,
    rtc_buffer_t * buffer
) {
    assert(dc != nullptr);
    assert(buffer != nullptr);
    return reinterpret_cast<RTCDataChannel *>(dc)->send(
        reinterpret_cast<RTCBuffer *>(buffer)
    ) ? 0 : -1;
}


rtc_context_t * rtc_data_channel_get_context(rtc_data_channel_t * dc) {
    assert(dc != nullptr);
    return reinterpret_cast<rtc_context_t *>(
        reinterpret_cast<RTCDataChannel *>(dc)->context
    );
}


bool rtc_data_channel_is_open(rtc_data_channel_t * dc) {
    assert(dc != nullptr);
    return reinterpret_cast<RTCDataChannel *>(dc)->is_open();
}


const char * rtc_data_channel_get_label(rtc_data_channel_t * dc) {
    assert(dc != nullptr);
    return reinterpret_cast<RTCDataChannel *>(dc)->get_label();
}


/* -------------------------------------------------------------------------- */
/*                          Readonly Buffer APIs                              */
/* -------------------------------------------------------------------------- */

size_t rtc_buffer_size(rtc_buffer_t * buffer) {
    assert(buffer != nullptr);
    return reinterpret_cast<RTCBuffer *>(buffer)->size();
}


const uint8_t * rtc_buffer_data(rtc_buffer_t * buffer) {
    assert(buffer != nullptr);
    return reinterpret_cast<RTCBuffer *>(buffer)->data();
}


void rtc_buffer_ref(rtc_buffer_t * buffer) {
    assert(buffer != nullptr);
    reinterpret_cast<RTCBuffer *>(buffer)->ref();
}


void rtc_buffer_unref(rtc_buffer_t * buffer) {
    assert(buffer != nullptr);
    reinterpret_cast<RTCBuffer *>(buffer)->unref();
}


rtc_buffer_t * rtc_buffer_prepare(
    rtc_context_t * context,
    size_t capacity,
    uint8_t ** data
) {
    assert(context != nullptr);
    assert(data != nullptr);
    if (capacity == 0) {
        return nullptr;
    }

    RTCBuffer * buffer =
        reinterpret_cast<RTCContext *>(context)->pool_buffer->acquire();
    if (!buffer) {
        return nullptr; // Failed to acquire new element
    }

    buffer->prepare(capacity, data);
    return reinterpret_cast<rtc_buffer_t *>(buffer);
}


rtc_context_t * rtc_buffer_get_context(rtc_context_t * buffer) {
    assert(buffer != nullptr);
    return reinterpret_cast<rtc_context_t *>(
        reinterpret_cast<RTCBuffer *>(buffer)->context
    );
}
