#ifndef POMELO_PLUGIN_WEBRTC_BASE64_H
#define POMELO_PLUGIN_WEBRTC_BASE64_H
#include <stdint.h>
#include <stddef.h>

#define POMELO_PLUGIN_BASE64_VARIANT 5 // sodium_base64_VARIANT_URLSAFE

#define POMELO_PLUGIN_ENCODED_LENGTH(BIN_LEN)                           \
    (((BIN_LEN) / 3U) * 4U +                                                   \
    ((((BIN_LEN) - ((BIN_LEN) / 3U) * 3U) |                                    \
    (((BIN_LEN) - ((BIN_LEN) / 3U) * 3U) >> 1)) & 1U) *                        \
    (4U - (~((((POMELO_PLUGIN_BASE64_VARIANT) & 2U) >> 1) - 1U) &       \
    (3U - ((BIN_LEN) - ((BIN_LEN) / 3U) * 3U)))) + 1U)


#ifdef __cplusplus
extern "C" {
#endif

/// @brief Encode binary to base64 (URL Safe)
char * pomelo_plugin_base64_encode(
    char * b64,
    size_t b64_maxlen,
    uint8_t * bin,
    size_t bin_len
);

/// @brief Decode binary from base64 (URL Safe)
int pomelo_plugin_base64_decode(
    const char * b64,
    size_t b64_len,
    uint8_t * bin,
    size_t bin_maxlen
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PLUGIN_WEBRTC_BASE64_H
