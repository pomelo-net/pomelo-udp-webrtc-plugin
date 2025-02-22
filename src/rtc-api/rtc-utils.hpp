#ifndef POMELO_WEBRTC_RTC_API_UTILS_HPP
#define POMELO_WEBRTC_RTC_API_UTILS_HPP
#include <string>
#include <optional>
#ifdef __cplusplus
namespace rtc_api {


void copy_address(std::optional<std::string> addr, char * buffer, int size);


}; // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_UTILS_HPP

