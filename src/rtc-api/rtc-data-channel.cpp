#include <cassert>
#include "rtc-data-channel.hpp"
#include "rtc-buffer-pool.hpp"
#include "rtc-buffer.hpp"
#include "rtc-context.hpp"


using namespace rtc_api;


RTCDataChannel::RTCDataChannel(RTCContext * context): RTCObject(context) {}


void RTCDataChannel::init(std::shared_ptr<rtc::DataChannel> dc) {
    this->dc = dc;

    // Update callbacks
    open_callback = context->options.dc_open_callback;
    closed_callback = context->options.dc_closed_callback;
    error_callback = context->options.dc_error_callback;
    message_callback = context->options.dc_message_callback;

    if (open_callback) {
        dc->onOpen(std::bind(&RTCDataChannel::on_open, this));
    }

    if (closed_callback) {
        dc->onClosed(std::bind(&RTCDataChannel::on_closed, this));
    }

    if (error_callback) {
        dc->onError(
            std::bind(&RTCDataChannel::on_error, this, std::placeholders::_1)
        );
    }

    if (message_callback) {
        dc->onMessage(
            std::bind(
                &RTCDataChannel::on_message_binary, this, std::placeholders::_1
            ),
            std::bind(
                &RTCDataChannel::on_message_string, this, std::placeholders::_1
            )
        );
    }
}


void RTCDataChannel::finalize() {
    RTCObject::finalize();
    if (!dc) {
        return;
    }

    try {
        dc->close();
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }

    dc = nullptr;
    open_callback = nullptr;
    closed_callback = nullptr;
    error_callback = nullptr;
    message_callback = nullptr;
}


RTCDataChannel::~RTCDataChannel() {
    finalize();
}


void RTCDataChannel::close() {
    try {
        dc->close();
    } catch (std::exception ex) {
        context->handle_exception(ex);
    }
}


bool RTCDataChannel::send(const uint8_t * message, size_t length) {
    assert(message != nullptr);
    auto data = reinterpret_cast<const std::byte *>(message);
    try {
        return dc->send(data, length);
    } catch (std::exception ex) {
        context->handle_exception(ex);
        return false;
    }
}


bool RTCDataChannel::send(RTCBuffer * buffer) {
    return send(buffer->data(), buffer->size());
}


bool RTCDataChannel::is_open() {
    return dc->isOpen();
}


const char * RTCDataChannel::get_label() {
    label = dc->label(); // Fetch label
    return label.c_str();
}


void RTCDataChannel::on_open() {
    open_callback(reinterpret_cast<rtc_data_channel_t *>(this));
}


void RTCDataChannel::on_closed() {
    closed_callback(reinterpret_cast<rtc_data_channel_t *>(this));
}


void RTCDataChannel::on_error(std::string error) {
    error_callback(
        reinterpret_cast<rtc_data_channel_t *>(this),
        error.c_str()
    );
}


void RTCDataChannel::on_message_string(std::string message) {
    RTCBuffer * buffer = context->pool_buffer->acquire();
    if (!buffer) {
        return; // Failed to acquire new buffer
    }
    buffer->set(message);

    message_callback(
        reinterpret_cast<rtc_data_channel_t *>(this),
        reinterpret_cast<rtc_buffer_t *>(buffer)
    );
    buffer->unref();
}


void RTCDataChannel::on_message_binary(rtc::binary message) {
    RTCBuffer * buffer = context->pool_buffer->acquire();
    if (!buffer) {
        return; // Failed to acquire new buffer
    }
    buffer->set(message);

    message_callback(
        reinterpret_cast<rtc_data_channel_t *>(this),
        reinterpret_cast<rtc_buffer_t *>(buffer)
    );
    buffer->unref();
}

