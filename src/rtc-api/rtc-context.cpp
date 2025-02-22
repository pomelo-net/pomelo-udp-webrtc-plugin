#include <cstring>
#include "rtc-context.hpp"
#include "rtc-object-pool.hpp"
#include "rtc-buffer-pool.hpp"
#include "rtc-ws-server.hpp"
#include "rtc-ws-client.hpp"
#include "rtc-peer-connection.hpp"
#include "rtc-data-channel.hpp"


using namespace std::chrono_literals;
using namespace rtc_api;


RTCContext::RTCContext(rtc_options_t * options) {
    pool_wsserver = new RTCObjectPool<RTCWSServer>(this);
    pool_wsclient = new RTCObjectPool<RTCWSClient>(this);
    pool_pc = new RTCObjectPool<RTCPeerConnection>(this) ;
    pool_dc = new RTCObjectPool<RTCDataChannel>(this);
    pool_buffer = new RTCBufferPool(this);

    memcpy(&this->options, options, sizeof(rtc_options_t));

    // Initialize logger
    log_callback = nullptr;
    rtc_log_level level = options->log_level;
    rtc_log_callback callback = options->log_callback;
    
    if (callback) {
        if (level >= RTC_LOG_LEVEL_DEBUG) {
            log_callback = callback;
        }
        rtc::InitLogger(static_cast<rtc::LogLevel>(level), [callback](
            rtc::LogLevel level,
            std::string message
        ) {
            callback(static_cast<rtc_log_level>(level), message.c_str());
        });
    }

    // Finally, preload RTC
    rtc::Preload();
}


RTCContext::~RTCContext() {
    memset(&options, 0, sizeof(rtc_options_t));

    delete pool_wsserver;
    pool_wsserver = nullptr;

    delete pool_wsclient;
    pool_wsclient = nullptr;

    delete pool_pc;
    pool_pc = nullptr;

    delete pool_dc;
    pool_dc = nullptr;

    delete pool_buffer;
    pool_buffer = nullptr;

    rtc::Cleanup().wait_for(10s);
    log_callback = nullptr;
}



void RTCContext::handle_exception(std::exception & ex) {
    if (log_callback) {
        log_callback(RTC_LOG_LEVEL_DEBUG, ex.what());
    }
}


void RTCContext::set_data(void * data) {
    this->data.store(data, std::memory_order_relaxed);
}


void * RTCContext::get_data() {
    return data.load(std::memory_order_relaxed);
}
