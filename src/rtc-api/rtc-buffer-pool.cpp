#include "rtc-buffer-pool.hpp"
#include "rtc-buffer.hpp"


using namespace rtc_api;

RTCBufferPool::RTCBufferPool(RTCContext * context):
    RTCObjectPool<RTCBuffer>(context) {}


RTCBuffer * RTCBufferPool::acquire() {
    RTCBuffer * buffer = RTCObjectPool<RTCBuffer>::acquire();
    if (!buffer) return nullptr;

    buffer->source = this;
    buffer->reset_ref();
    return buffer;
}
