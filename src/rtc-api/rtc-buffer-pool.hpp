#ifndef POMELO_WEBRTC_RTC_API_BUFFER_POOL_HPP
#define POMELO_WEBRTC_RTC_API_BUFFER_POOL_HPP
#include "rtc-api.hpp"
#include "rtc-object-pool.hpp"
#include "rtc-buffer.hpp"
#ifdef __cplusplus
namespace rtc_api {


/// @brief Buffer pool
class RTCBufferPool : public RTCObjectPool<RTCBuffer> {
public:
    RTCBufferPool(RTCContext * context);

    void init(RTCBuffer * buffer) override;
};


} // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_BUFFER_POOL_HPP
