#ifndef POMELO_WEBRTC_RTC_API_PEER_CONNECTION
#define POMELO_WEBRTC_RTC_API_PEER_CONNECTION
#include "rtc-object.hpp"
#ifdef __cplusplus
namespace rtc_api {


/// @brief Wrapper for peer connection
class RTCPeerConnection : public RTCObject {
public:
    RTCPeerConnection(RTCContext * context);
    ~RTCPeerConnection();

    void init(rtc_peer_connection_options_t * options);
    virtual void finalize() override;

    /// @brief Close connection
    void close();

    /// @brief Get remote address
    void remote_address(char * buffer, int size);

    /// @brief Create data channel
    RTCDataChannel * create_data_channel(rtc_data_channel_options_t * options);

    /// @brief Set local description
    void set_local_description(const char * type);

    /// @brief Get local description SDP
    const char * get_local_description_sdp();

    /// @brief Get local description type
    const char * get_local_description_type();

    /// @brief Set remote description
    void set_remote_description(const char * sdp, const char * type);

    /// @brief Add remote candidate
    void add_remote_candidate(const char * cand, const char * mid);

private:
    /// @brief Handle local candidate
    void on_local_candidate(rtc::Candidate candidate);

    /// @brief Handle state changed
    void on_state_change(rtc::PeerConnection::State state);

    /// @brief Handle new data channel
    void on_data_channel(std::shared_ptr<rtc::DataChannel> data_channel);

    /// @brief Get the local description
    void fetch_local_description();

    /// @brief Clear the local description
    void clear_local_description();

private:
    /// @brief Peer connection
    std::shared_ptr<rtc::PeerConnection> pc = nullptr;

    /* Callbacks */
    rtc_peer_connection_local_candidate_callback local_candidate_callback
        = nullptr;
    rtc_peer_connection_state_change_callback state_change_callback = nullptr;
    rtc_peer_connection_data_channel_callback data_channel_callback = nullptr;

    /// @brief Local description
    std::optional<rtc::Description> local_description;

    std::string local_description_sdp;
    std::string local_description_type;
};


} // namespace rtc_api
#endif // __cplusplus
#endif
