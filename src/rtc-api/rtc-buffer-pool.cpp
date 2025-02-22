#include "rtc-buffer-pool.hpp"
#include "rtc-buffer.hpp"


using namespace rtc_api;

RTCBufferPool::RTCBufferPool(RTCContext * context):
    RTCObjectPool<RTCBuffer>(context) {}


void RTCBufferPool::init(RTCBuffer * buffer) {
    buffer->source = this;
    buffer->reset_ref();
}
