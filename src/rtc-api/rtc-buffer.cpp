#include "rtc-buffer.hpp"
#include "rtc-buffer-pool.hpp"

using namespace rtc_api;


RTCBuffer::RTCBuffer(RTCContext * context): RTCObject(context) {}


const uint8_t * RTCBuffer::data() {
    return is_binary
        ? reinterpret_cast<const uint8_t *>(binary_data.data())
        : reinterpret_cast<const uint8_t *>(string_data.c_str());
}


size_t RTCBuffer::size() {
    return is_binary ? binary_data.size() : (string_data.size() + 1);
}


void RTCBuffer::set(std::string & string_data) {
    is_binary = false;
    this->string_data = std::move(string_data);
}


void RTCBuffer::set(std::string && string_data) {
    is_binary = false;
    this->string_data = string_data;
}


void RTCBuffer::set(rtc::binary & binary_data) {
    is_binary = true;
    this->binary_data = std::move(binary_data);
}


void RTCBuffer::prepare(size_t capacity, uint8_t ** data) {
    is_binary = true;
    binary_data.resize(capacity);
    *data = reinterpret_cast<uint8_t *>(binary_data.data());
}


void RTCBuffer::reset_ref() {
    ref_counter.store(1, std::memory_order_relaxed);
}


void RTCBuffer::ref() {
    int prev = 0;
    do {
        prev = ref_counter.load(std::memory_order_relaxed);
        if (prev == 0) {
            return;
        }
    } while (!ref_counter.compare_exchange_weak(
        prev,
        prev + 1,
        std::memory_order_relaxed
    ));
}


void RTCBuffer::unref() {
    int prev = ref_counter.fetch_add(1, std::memory_order_relaxed);
    if (prev == 1) { // Need to release
        source->release(this);
    }
}
