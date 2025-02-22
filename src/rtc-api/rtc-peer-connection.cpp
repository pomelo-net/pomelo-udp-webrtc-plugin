#include <cassert>
#include "rtc-api.hpp"
#include "rtc-peer-connection.hpp"
#include "rtc-data-channel.hpp"
#include "rtc-buffer-pool.hpp"
#include "rtc-buffer.hpp"
#include "rtc-context.hpp"
#include "rtc-utils.hpp"


using namespace rtc_api;
using namespace std::chrono;


RTCPeerConnection::RTCPeerConnection(RTCContext * context):
    RTCObject(context) {}


void RTCPeerConnection::init(rtc_peer_connection_options_t * options) {
    set_data(options->data);

    // Update callbacks
    local_candidate_callback = context->options.pc_local_candidate_callback;
    state_change_callback = context->options.pc_state_change_callback;
    data_channel_callback = context->options.pc_data_channel_callback;

    rtc::Configuration conf;
    for (int i = 0; i < options->ice_servers_count; ++i) {
        conf.iceServers.emplace_back(std::string(options->ice_servers[i]));
    }

    if (options->proxy_server) {
        conf.proxyServer.emplace(options->proxy_server);
    }

    if (options->bind_address) {
        conf.bindAddress = std::string(options->bind_address);
    }

    if (options->port_range_begin > 0 || options->port_range_end > 0) {
        conf.portRangeBegin = options->port_range_begin;
        conf.portRangeEnd = options->port_range_end;
    }

    conf.certificateType =
        static_cast<rtc::CertificateType>(options->certificate_type);
    conf.iceTransportPolicy =
        static_cast<rtc::TransportPolicy>(options->ice_transport_policy);
    conf.enableIceTcp = options->enable_ice_tcp;
    conf.enableIceUdpMux = options->enable_ice_udp_mux;
    conf.disableAutoNegotiation = true;
    conf.forceMediaTransport = options->force_media_transport;

    if (options->mtu > 0) {
        conf.mtu = size_t(options->mtu);
    }

    if (options->max_message_size) {
        conf.maxMessageSize = size_t(options->max_message_size);
    }

    pc = std::make_shared<rtc::PeerConnection>(std::move(conf));

    if (local_candidate_callback) {
        pc->onLocalCandidate(std::bind(
            &RTCPeerConnection::on_local_candidate, this, std::placeholders::_1
        ));
    }

    if (state_change_callback) {
        pc->onStateChange(std::bind(
            &RTCPeerConnection::on_state_change, this, std::placeholders::_1
        ));
    }

    if (data_channel_callback) {
        pc->onDataChannel(std::bind(
            &RTCPeerConnection::on_data_channel, this, std::placeholders::_1
        ));
    }
}


void RTCPeerConnection::finalize() {
    RTCObject::finalize();
    if (!pc) {
        return;
    }

    try {
        pc->close();
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }

    pc = nullptr;
    local_candidate_callback = nullptr;
    state_change_callback = nullptr;
    data_channel_callback = nullptr;

    clear_local_description();
}


RTCPeerConnection::~RTCPeerConnection() {
    finalize();
}


void RTCPeerConnection::close() {
    try {
        pc->close();
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }
}


void RTCPeerConnection::remote_address(char * buffer, int size) {
    assert(buffer != nullptr);
    if (size <= 0) {
        return;
    }

    try {
        copy_address(pc->remoteAddress(), buffer, size);
    } catch (std::exception ex) {
        context->handle_exception(ex);
        buffer[0] = '\n'; // Empty address
    }
}


RTCDataChannel * RTCPeerConnection::create_data_channel(
    rtc_data_channel_options_t * options
) {
    assert(options != nullptr);

    rtc::DataChannelInit dci = {};
    auto * reliability = &options->reliability;
    dci.reliability.unordered = reliability->unordered;
    if (reliability->unreliable) {
        if (reliability->maxPacketLifeTime > 0) {
            dci.reliability.maxPacketLifeTime.emplace(
                milliseconds(reliability->maxPacketLifeTime)
            );
        } else {
            dci.reliability.maxRetransmits.emplace(reliability->maxRetransmits);
        }
    }

    dci.negotiated = options->negotiated;
    dci.id = options->manualStream
        ? std::make_optional(options->stream)
        : std::nullopt;
    dci.protocol = options->protocol ? options->protocol : "";

    const char * label = options->label;
    std::shared_ptr<rtc::DataChannel> dc = nullptr;
    try {
        dc = pc->createDataChannel(
            std::string(label ? label : ""),
            std::move(dci)
        );
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }

    if (!dc) {
        return nullptr;
    }

    auto channel = context->pool_dc->acquire();
    if (!channel) {
        return nullptr;
    }

    try {
        channel->init(dc);
    } catch (std::exception ex) {
        context->handle_exception(ex);
        context->pool_dc->release(channel);
        return nullptr;
    }

    channel->set_data(options->data);
    return channel;
}


void RTCPeerConnection::set_local_description(const char * type) {
    if (type == nullptr) {
        pc->setLocalDescription(); // Create offer
    } else {
        pc->setLocalDescription(
            rtc::Description::stringToType(std::string(type))
        );
    }
}


const char * RTCPeerConnection::get_local_description_sdp() {
    fetch_local_description();
    return local_description_sdp.c_str();
}


const char * RTCPeerConnection::get_local_description_type() {
    fetch_local_description();
    return local_description_type.c_str();
}


void RTCPeerConnection::set_remote_description(
    const char * sdp,
    const char * type
) {
    assert(sdp != nullptr);
    assert(type != nullptr);

    try {
        pc->setRemoteDescription({
            std::string(sdp),
            std::string(type ? type : "")
        });
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }
}


void RTCPeerConnection::add_remote_candidate(
    const char * cand,
    const char * mid
) {
    assert(cand != nullptr);
    assert(mid != nullptr);

    try {
        pc->addRemoteCandidate({
            std::string(cand),
            std::string(mid ? mid : "")
        });
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }
}


void RTCPeerConnection::on_local_candidate(rtc::Candidate candidate) {
    RTCBuffer * cand_buff = context->pool_buffer->acquire();
    if (!cand_buff) {
        return;
    }
    RTCBuffer * mid_buff = context->pool_buffer->acquire();
    if (!mid_buff) {
        context->pool_buffer->release(cand_buff);
        return;
    }

    cand_buff->set(candidate.candidate());
    mid_buff->set(candidate.mid());

    local_candidate_callback(
        reinterpret_cast<rtc_peer_connection_t *>(this),
        reinterpret_cast<rtc_buffer_t *>(cand_buff),
        reinterpret_cast<rtc_buffer_t *>(mid_buff)
    );

    cand_buff->unref();
    mid_buff->unref();
}


void RTCPeerConnection::on_state_change(rtc::PeerConnection::State state) {
    state_change_callback(
        reinterpret_cast<rtc_peer_connection_t *>(this),
        static_cast<rtc_peer_connection_state>(state)
    );
}


void RTCPeerConnection::on_data_channel(
    std::shared_ptr<rtc::DataChannel> data_channel
) {
    RTCDataChannel * dc = context->pool_dc->acquire();
    if (!dc) {
        return;
    }

    try {
        dc->init(data_channel);
    } catch (std::exception ex) {
        context->handle_exception(ex);
        context->pool_dc->release(dc);
        return; // Failed to initialize data channel
    }

    // Call the callback
    data_channel_callback(
        reinterpret_cast<rtc_peer_connection_t *>(this),
        reinterpret_cast<rtc_data_channel_t *>(dc)
    );
}


void RTCPeerConnection::fetch_local_description() {
    if (local_description.has_value()) {
        return;
    }

    local_description = pc->localDescription();
    if (!local_description.has_value()) {
        return;
    }

    local_description_sdp = std::string(*local_description);
    local_description_type = local_description->typeString();
}


void RTCPeerConnection::clear_local_description() {
    local_description.reset();
}
