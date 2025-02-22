#ifndef POMELO_WEBRTC_RTC_API_HPP
#define POMELO_WEBRTC_RTC_API_HPP
#include "rtc-api.h"
#include "rtc/rtc.hpp"
#ifdef __cplusplus

namespace rtc_api {

class RTCContext;
class RTCObject;
class RTCBuffer;
class RTCBufferPool;
class RTCWSClient;
class RTCWSServer;
class RTCPeerConnection;
class RTCDataChannel;

template <typename T>
class RTCObjectPool;

}; // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_HPP
