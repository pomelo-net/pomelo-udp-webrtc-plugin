#include <cassert>
#include "rtc-ws-client.hpp"
#include "rtc-buffer-pool.hpp"
#include "rtc-buffer.hpp"
#include "rtc-context.hpp"
#include "rtc-utils.hpp"


using namespace rtc_api;


RTCWSClient::RTCWSClient(RTCContext * context): RTCObject(context) {}


void RTCWSClient::init(std::shared_ptr<rtc::WebSocket> ws_client) {
    this->ws_client = ws_client;

    open_callback = context->options.ws_open_callback;
    closed_callback = context->options.ws_closed_callback;
    error_callback = context->options.ws_error_callback;
    message_callback = context->options.ws_message_callback;

    if (open_callback) {
        ws_client->onOpen(std::bind(&RTCWSClient::on_open, this));
    }

    if (closed_callback) {
        ws_client->onClosed(std::bind(&RTCWSClient::on_closed, this));
    }

    if (error_callback) {
        ws_client->onError(
            std::bind(&RTCWSClient::on_error, this, std::placeholders::_1)
        );
    }

    if (message_callback) {
        ws_client->onMessage(
            std::bind(
                &RTCWSClient::on_message_binary, this, std::placeholders::_1
            ),
            std::bind(
                &RTCWSClient::on_message_string, this, std::placeholders::_1
            )
        );
    }
}


void RTCWSClient::finalize() {
    RTCObject::finalize();
    if (!ws_client) {
        return;
    }
    
    try {
        ws_client->forceClose();
        ws_client->resetCallbacks();
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }

    ws_client = nullptr;
    open_callback = nullptr;
    closed_callback = nullptr;
    error_callback = nullptr;
    message_callback = nullptr;
}


RTCWSClient::~RTCWSClient() {
    finalize();
}


void RTCWSClient::close() {
    try {
        ws_client->close();
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }
}


bool RTCWSClient::send(const char * message) {
    assert(message != nullptr);
    try {
        return ws_client->send(std::string(message));
    } catch (std::exception ex) {
        context->handle_exception(ex);
        return false;
    }
}


bool RTCWSClient::send(const uint8_t * message, size_t length) {
    assert(message != nullptr);
    auto data = reinterpret_cast<const std::byte *>(message);

    try {
        return ws_client->send(rtc::binary(data, data + length));
    } catch (std::exception ex) {
        context->handle_exception(ex);
        return false;
    }
}


void RTCWSClient::remote_address(char * buffer, int size) {
    assert(buffer != nullptr);
    if (size <= 0) {
        return;
    }
    try {
        copy_address(ws_client->remoteAddress(), buffer, size);
    } catch (std::exception ex) {
        context->handle_exception(ex);
        buffer[0] = '\0';
    }
}


void RTCWSClient::on_open() {
    open_callback(reinterpret_cast<rtc_websocket_client_t *>(this));
}


void RTCWSClient::on_closed() {
    closed_callback(reinterpret_cast<rtc_websocket_client_t *>(this));
}


void RTCWSClient::on_error(std::string error) {
    error_callback(
        reinterpret_cast<rtc_websocket_client_t *>(this),
        error.c_str()
    );
}


void RTCWSClient::on_message_string(std::string message) {
    RTCBuffer * buffer = context->pool_buffer->acquire();
    if (!buffer) {
        return; // Failed to acquire new buffer
    }
    buffer->set(message);

    message_callback(
        reinterpret_cast<rtc_websocket_client_t *>(this),
        reinterpret_cast<rtc_buffer_t *>(buffer)
    );
    buffer->unref();
}


void RTCWSClient::on_message_binary(rtc::binary message) {
    RTCBuffer * buffer = context->pool_buffer->acquire();
    if (!buffer) {
        return; // Failed to acquire new buffer
    }
    buffer->set(message);

    message_callback(
        reinterpret_cast<rtc_websocket_client_t *>(this),
        reinterpret_cast<rtc_buffer_t *>(buffer)
    );
    buffer->unref();
}
