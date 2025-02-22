#include <assert.h>
#include <stdlib.h>

// Base64
#define SODIUM_STATIC 1
#define DEV_MODE 0
#define CONFIGURED 1
#include "sodium/codecs.c"

#define VARIANT sodium_base64_VARIANT_URLSAFE

void sodium_misuse(void) {
    assert(0);
    abort();
}

char * pomelo_plugin_base64_encode(
    char * b64,
    size_t b64_maxlen,
    uint8_t * bin,
    size_t bin_len
) {
    assert(b64 != NULL);
    assert(bin != NULL);

    return sodium_bin2base64(b64, b64_maxlen, bin, bin_len, VARIANT);
}


int pomelo_plugin_base64_decode(
    const char * b64,
    size_t b64_len,
    uint8_t * bin,
    size_t bin_maxlen
) {
    assert(b64 != NULL);
    assert(bin != NULL);

    return sodium_base642bin(
        bin, bin_maxlen, b64, b64_len, NULL, NULL, NULL, VARIANT
    );
}
