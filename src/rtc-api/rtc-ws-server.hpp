#ifndef POMELO_WEBRTC_RTC_API_WS_SERVER_HPP
#define POMELO_WEBRTC_RTC_API_WS_SERVER_HPP
#include "rtc-object.hpp"
#ifdef __cplusplus
namespace rtc_api {


class RTCWSServer : public RTCObject {
public:
    RTCWSServer(RTCContext * context);
    ~RTCWSServer();

    void init(rtc_websocket_server_options_t * options);
    void close();
    virtual void finalize() override;

private:
    void on_client(std::shared_ptr<rtc::WebSocket> ws_client);

private:
    /// @brief Websocket server instance
    std::shared_ptr<rtc::WebSocketServer> ws_server = nullptr;

    /// @brief Callback for websocket server
    rtc_websocket_server_callback callback = nullptr;
};


} // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_WS_SERVER_HPP
