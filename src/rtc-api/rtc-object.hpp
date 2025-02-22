#ifndef POMELO_WEBRTC_RTC_API_OBJECT_HPP
#define POMELO_WEBRTC_RTC_API_OBJECT_HPP
#include "rtc-api.hpp"
#ifdef __cplusplus
namespace rtc_api {


class RTCObject {
public:
    RTCObject(RTCContext * context);
    virtual ~RTCObject() = default;

    void set_data(void * data);
    void * get_data();

    virtual void finalize();

    /// @brief Context
    RTCContext * context;

private:
    /// @brief Private data
    std::atomic<void *> data;
};


} // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_OBJECT_HPP
