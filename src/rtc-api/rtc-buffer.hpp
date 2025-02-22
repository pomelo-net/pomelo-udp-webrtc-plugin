#ifndef POMELO_WEBRTC_RTC_API_BUFFER_HPP
#define POMELO_WEBRTC_RTC_API_BUFFER_HPP
#include "rtc/rtc.hpp"
#include "rtc-object.hpp"
#ifdef __cplusplus


namespace rtc_api {

/// @brief RTC buffer
class RTCBuffer : public RTCObject {
public:
    RTCBuffer(RTCContext * context);

    const uint8_t * data();
    size_t size();

    void set(std::string & string_data);
    void set(std::string && string_data);
    void set(rtc::binary & binary_data);
    void prepare(size_t capacity, uint8_t ** data);

    void reset_ref();
    void ref();
    void unref();

private:
    std::atomic<int> ref_counter;

    bool is_binary = true;
    rtc::binary binary_data;
    std::string string_data;
    RTCBufferPool * source;
    friend class RTCBufferPool;
};


} // namespace rtc_api


#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_BUFFER_HPP
