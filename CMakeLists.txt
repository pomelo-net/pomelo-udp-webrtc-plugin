cmake_minimum_required(VERSION 3.10)
project(pomelo-udp-webrtc-plugin VERSION 1.0.0)

# This project uses some parts of pomelo-udp-native: base, crypto and utils

set(POMELO_WEBRTC_NAME pomelo-udp-webrtc-plugin)
set(POMELO_WEBRTC_SRC
    # Headers
    deps/pomelo-udp-native/include/pomelo/address.h
    deps/pomelo-udp-native/include/pomelo/allocator.h
    deps/pomelo-udp-native/include/pomelo/base64.h
    deps/pomelo-udp-native/include/pomelo/common.h
    deps/pomelo-udp-native/include/pomelo/constants.h
    deps/pomelo-udp-native/include/pomelo/plugin.h
    deps/pomelo-udp-native/include/pomelo/statistic/statistic-allocator.h

    # Base
    deps/pomelo-udp-native/src/base/address.c
    deps/pomelo-udp-native/src/base/allocator.c
    deps/pomelo-udp-native/src/base/allocator.h
    deps/pomelo-udp-native/src/base/payload.c
    deps/pomelo-udp-native/src/base/payload.h
    deps/pomelo-udp-native/src/base/ref.c
    deps/pomelo-udp-native/src/base/ref.h

    # Base64
    deps/pomelo-udp-native/src/crypto/base64.c

    # Utils
    deps/pomelo-udp-native/src/utils/array.c
    deps/pomelo-udp-native/src/utils/array.h
    deps/pomelo-udp-native/src/utils/atomic.c
    deps/pomelo-udp-native/src/utils/atomic.h
    deps/pomelo-udp-native/src/utils/list.c
    deps/pomelo-udp-native/src/utils/macro.h
    deps/pomelo-udp-native/src/utils/map.c
    deps/pomelo-udp-native/src/utils/map.h
    deps/pomelo-udp-native/src/utils/mutex.c
    deps/pomelo-udp-native/src/utils/mutex.h
    deps/pomelo-udp-native/src/utils/pool.c
    deps/pomelo-udp-native/src/utils/pool.h
    deps/pomelo-udp-native/src/utils/rtt.c
    deps/pomelo-udp-native/src/utils/rtt.c
    deps/pomelo-udp-native/src/utils/sampling.c
    deps/pomelo-udp-native/src/utils/sampling.h

    src/channel/channel-dc.c
    src/channel/channel-dc.h
    src/channel/channel.c
    src/channel/channel.h
    src/channel/channel-int.h
    src/channel/channel-plugin.c
    src/channel/channel-plugin.h
    src/channel/channel.c
    src/channel/channel.h

    src/rtc-api/rtc-api.cpp
    src/rtc-api/rtc-api.h
    src/rtc-api/rtc-api.hpp
    src/rtc-api/rtc-buffer-pool.cpp
    src/rtc-api/rtc-buffer-pool.hpp
    src/rtc-api/rtc-buffer.cpp
    src/rtc-api/rtc-buffer.hpp
    src/rtc-api/rtc-context.cpp
    src/rtc-api/rtc-context.hpp
    src/rtc-api/rtc-data-channel.cpp
    src/rtc-api/rtc-data-channel.hpp
    src/rtc-api/rtc-object.cpp
    src/rtc-api/rtc-object.hpp
    src/rtc-api/rtc-peer-connection.cpp
    src/rtc-api/rtc-peer-connection.hpp
    src/rtc-api/rtc-utils.cpp
    src/rtc-api/rtc-utils.hpp
    src/rtc-api/rtc-ws-client.cpp
    src/rtc-api/rtc-ws-client.hpp
    src/rtc-api/rtc-ws-server.cpp
    src/rtc-api/rtc-ws-server.hpp

    src/session/session-int.h
    src/session/session-pc.c
    src/session/session-pc.h
    src/session/session-plugin.c
    src/session/session-plugin.h
    src/session/session-ws.c
    src/session/session-ws.h
    src/session/session.c
    src/session/session.h

    src/socket/socket-int.h
    src/socket/socket-plugin.c
    src/socket/socket-plugin.h
    src/socket/socket-wss.c
    src/socket/socket-wss.h
    src/socket/socket.c
    src/socket/socket.h

    src/utils/common-macro.h
    src/utils/string-buffer.c
    src/utils/string-buffer.h

    src/context.c
    src/context.h
    src/plugin.c
    src/plugin.h
)

# Settings for libdatachannel
option(BUILD_SHARED_LIBS "" OFF)
option(BUILD_SHARED_DEPS_LIBS "" OFF)
option(NO_TESTS "" ON)
option(NO_EXAMPLES "" ON)
option(NO_MEDIA "" ON)
option(LIBUV_BUILD_SHARED OFF)

add_subdirectory(deps/libdatachannel)
add_subdirectory(deps/libuv)

set(POMELO_INCLUDE
    deps/pomelo-udp-native/include
    deps/pomelo-udp-native/src
)

# Sodium
set(SODIUM_INCLUDE
    deps/libsodium/src/libsodium
    deps/libsodium/src/libsodium/include
    deps/libsodium/src/libsodium/include/sodium
)
set(SODIUM_SRC
    deps/libsodium/src/libsodium/sodium/codecs.c
    src/utils/sodium.c
)
set(SODIUM_DEFINES DEV_MODE=0 CONFIGURED=1)
add_library(sodium STATIC ${SODIUM_SRC})
target_include_directories(sodium PUBLIC ${SODIUM_INCLUDE})
target_compile_definitions(sodium PRIVATE ${SODIUM_DEFINES})


add_library(${POMELO_WEBRTC_NAME} SHARED ${POMELO_WEBRTC_SRC})
target_include_directories(${POMELO_WEBRTC_NAME} PUBLIC src ${SODIUM_INCLUDE} ${POMELO_INCLUDE})
target_link_libraries(${POMELO_WEBRTC_NAME} PUBLIC datachannel-static uv_a sodium)
set_target_properties(${POMELO_WEBRTC_NAME} PROPERTIES CXX_STANDARD 17)


if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    set(POMELO_COMPILE_FLAGS /WX)
else()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message("Pomelo UDP WebRTC Debug")
        set(POMELO_COMPILE_FLAGS -Wall -Wpedantic -Wextra -Werror -g)
    else()
        message("Pomelo UDP WebRTC Non-Debug")
        set(POMELO_COMPILE_FLAGS -Wall -Wpedantic -Wextra -Werror)
    endif()
endif()

target_compile_options(${POMELO_WEBRTC_NAME} PRIVATE ${POMELO_COMPILE_FLAGS})
target_compile_definitions(${POMELO_WEBRTC_NAME} PRIVATE
    POMELO_PLUGIN_WEBRTC_ENABLED_LOG=1
    ${SODIUM_DEFINES}
)
