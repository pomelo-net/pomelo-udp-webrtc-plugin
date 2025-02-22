#include "rtc-object.hpp"
#include "rtc-buffer-pool.hpp"

using namespace rtc_api;


RTCObject::RTCObject(RTCContext * context): context(context) {}


void RTCObject::finalize() {
    set_data(nullptr);
}


void RTCObject::set_data(void * data) {
    this->data.store(data, std::memory_order_relaxed);
}


void * RTCObject::get_data() {
    return data.load(std::memory_order_relaxed);
}
