#ifndef POMELO_WEBRTC_RTC_API_WS_CLIENT_HPP
#define POMELO_WEBRTC_RTC_API_WS_CLIENT_HPP
#include "rtc-object.hpp"
#ifdef __cplusplus
namespace rtc_api {


class RTCWSClient : public RTCObject {
public:
    RTCWSClient(RTCContext * context);
    ~RTCWSClient();

    void init(std::shared_ptr<rtc::WebSocket> ws_client);
    virtual void finalize() override;

    /// @brief Close websocket client
    void close();

    /// @brief Send string message
    bool send(const char * message);

    /// @brief Send binary message
    bool send(const uint8_t * message, size_t length);

    /// @brief Get remote address
    void remote_address(char * buffer, int size);

private:
    void on_open();
    void on_closed();
    void on_error(std::string error);
    void on_message_string(std::string message);
    void on_message_binary(rtc::binary message);

private:
    /// @brief Websocket client instance
    std::shared_ptr<rtc::WebSocket> ws_client = nullptr;

    /* Callbacks */
    rtc_websocket_client_open_callback open_callback = nullptr;
    rtc_websocket_client_closed_callback closed_callback = nullptr;
    rtc_websocket_client_error_callback error_callback = nullptr;
    rtc_websocket_client_message_callback message_callback = nullptr;
};


} // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_WS_CLIENT_HPP
