#ifndef POMELO_WEBRTC_RTC_API_DATA_CHANNEL_HPP
#define POMELO_WEBRTC_RTC_API_DATA_CHANNEL_HPP
#include "rtc-api.hpp"
#include "rtc-object.hpp"
#ifdef __cplusplus
namespace rtc_api {


/// @brief Wrapper for data channel
class RTCDataChannel : public RTCObject {
public:
    RTCDataChannel(RTCContext * context);
    ~RTCDataChannel();

    void init(std::shared_ptr<rtc::DataChannel> dc);
    virtual void finalize() override;

    /// @brief Close data channel
    void close();

    /// @brief Send message
    bool send(const uint8_t * message, size_t length);

    /// @brief Send a buffer
    bool send(RTCBuffer * buffer);

    /// @brief Check if data channel is 'open'
    bool is_open();

    /// @brief Get label of data channel
    const char * get_label();

private:
    void on_open();
    void on_closed();
    void on_error(std::string error);
    void on_message_string(std::string message);
    void on_message_binary(rtc::binary message);

private:
    /// @brief Data channel
    std::shared_ptr<rtc::DataChannel> dc;

    /// @brief Label of data channel
    std::string label;

    /* Callbacks */
    rtc_data_channel_open_callback open_callback = nullptr;
    rtc_data_channel_closed_callback closed_callback = nullptr;
    rtc_data_channel_error_callback error_callback = nullptr;
    rtc_data_channel_message_callback message_callback = nullptr;
};


} // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_DATA_CHANNEL_HPP