#ifndef POMELO_WEBRTC_RTC_API_CONTEXT_HPP
#define POMELO_WEBRTC_RTC_API_CONTEXT_HPP
#include <atomic>
#include <exception>
#include "rtc-api.hpp"
#ifdef __cplusplus
namespace rtc_api {


class RTCContext {
public:
    RTCContext(rtc_options_t * rtc_options);
    ~RTCContext();

    /// @brief Handle exception
    void handle_exception(std::exception & ex);
    
    /// @brief Set associated data
    void set_data(void * data);

    /// @brief Get associated data
    void * get_data();

public:
    /// @brief RTC options
    rtc_options_t options;

    RTCObjectPool<RTCWSServer> * pool_wsserver;
    RTCObjectPool<RTCWSClient> * pool_wsclient;
    RTCObjectPool<RTCPeerConnection> * pool_pc;
    RTCObjectPool<RTCDataChannel> * pool_dc;
    RTCBufferPool * pool_buffer;

private:
    rtc_log_callback log_callback;
    std::atomic<void *> data;
};


} // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_CONTEXT_HPP
