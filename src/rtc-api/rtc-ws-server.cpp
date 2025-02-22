#include "rtc-ws-server.hpp"
#include "rtc-ws-client.hpp"
#include "rtc-buffer-pool.hpp"
#include "rtc-context.hpp"


using namespace rtc_api;


RTCWSServer::RTCWSServer(RTCContext * context): RTCObject(context) {}


void RTCWSServer::init(rtc_websocket_server_options_t * options) {
    set_data(options->data);

    rtc::WebSocketServer::Configuration conf;
    conf.port = options->port;
    conf.enableTls = options->enable_tls;
    conf.certificatePemFile = options->certificate_pem_file
        ? std::make_optional(std::string(options->certificate_pem_file))
        : std::nullopt;
    conf.keyPemFile = options->key_pem_file
        ? std::make_optional(std::string(options->key_pem_file))
        : std::nullopt;
    conf.keyPemPass = options->key_pem_pass
        ? std::make_optional(std::string(options->key_pem_pass))
        : std::nullopt;
    conf.bindAddress = options->bind_address
        ? std::make_optional(std::string(options->bind_address))
        : std::nullopt;

    if (options->max_message_size > 0) {
        conf.maxMessageSize = size_t(options->max_message_size);
    }

    ws_server = std::make_shared<rtc::WebSocketServer>(std::move(conf));

    callback = context->options.wss_client_callback;
    if (callback) {
        ws_server->onClient(
            std::bind(&RTCWSServer::on_client, this, std::placeholders::_1)
        );
    }
}


void RTCWSServer::close() {
    if (!ws_server) {
        return;
    }

    ws_server->onClient(nullptr);
    ws_server->stop();
}


void RTCWSServer::finalize() {
    RTCObject::finalize();
    if (!ws_server) {
        return;
    }

    try {
        ws_server->onClient(nullptr);
        ws_server->stop();
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }

    ws_server = nullptr;
    callback = nullptr;
}


RTCWSServer::~RTCWSServer() {
    finalize();
}


void RTCWSServer::on_client(std::shared_ptr<rtc::WebSocket> ws_client) {
    // Wrap client and call the callback
    RTCWSClient * wsc = context->pool_wsclient->acquire();
    if (!wsc) {
        return;
    }

    try {
        wsc->init(ws_client);
    } catch (std::exception ex) {
        context->handle_exception(ex);
        context->pool_wsclient->release(wsc);
        return; // Failed to create new websocket client
    }

    callback(
        reinterpret_cast<rtc_websocket_server_t *>(this),
        reinterpret_cast<rtc_websocket_client_t *>(wsc)
    );
}
