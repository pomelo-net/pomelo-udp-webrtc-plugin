#include "rtc-utils.hpp"


void rtc_api::copy_address(
    std::optional<std::string> addr,
    char * buffer,
    int size
) {
    if (!addr.has_value() || size < static_cast<int>(addr->size() + 1)) {
        buffer[0] = '\0';
        return;
    }

    std::copy(addr->begin(), addr->end(), buffer);
    buffer[addr->size()] = '\0';
}
