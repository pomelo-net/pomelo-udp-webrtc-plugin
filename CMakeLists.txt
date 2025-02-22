cmake_minimum_required(VERSION 3.10)
project(pomelo-udp-webrtc-plugin VERSION 1.0.0)

set(POMELO_WEBRTC_NAME pomelo-udp-webrtc-plugin)
set(POMELO_WEBRTC_SRC
    src/base/address.c
    src/base/allocator.c
    src/base/payload.c
    src/base/payload.h
    src/base/ref.c
    src/base/ref.h
    src/base/rtt.c
    src/base/rtt.h
    
    src/channel/channel-dc.c
    src/channel/channel-dc.h
    src/channel/channel.c
    src/channel/channel.h
    src/channel/channel-int.h
    src/channel/channel-plugin.c
    src/channel/channel-plugin.h
    src/channel/channel-pool.c
    src/channel/channel-pool.h
    src/channel/channel.c
    src/channel/channel.h

    src/codec/packed.c
    src/codec/packed.h

    src/pomelo/address.h
    src/pomelo/allocator.h
    src/pomelo/common.h
    src/pomelo/plugin.h

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
    src/session/session-pool.c
    src/session/session-pool.h
    src/session/session-ws.c
    src/session/session-ws.h
    src/session/session.c
    src/session/session.h

    src/socket/socket-int.h
    src/socket/socket-plugin.c
    src/socket/socket-plugin.h
    src/socket/socket-pool.c
    src/socket/socket-pool.h
    src/socket/socket-wss.c
    src/socket/socket-wss.h
    src/socket/socket.c
    src/socket/socket.h

    src/utils/array.c
    src/utils/array.h
    src/utils/atomic.c
    src/utils/atomic.h
    src/utils/base64.c
    src/utils/base64.h
    src/utils/list.c
    src/utils/list.h
    src/utils/map.c
    src/utils/map.h
    src/utils/mutex.c
    src/utils/mutex.h
    src/utils/pool.c
    src/utils/pool.h
    src/utils/sampling.c
    src/utils/sampling.h
    src/utils/string-buffer.c
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

set(SODIUM_INCLUDE
    deps/libsodium/src/libsodium
    deps/libsodium/src/libsodium/include/sodium
)

add_library(${POMELO_WEBRTC_NAME} SHARED ${POMELO_WEBRTC_SRC})
target_include_directories(${POMELO_WEBRTC_NAME} PUBLIC src ${SODIUM_INCLUDE})
target_link_libraries(${POMELO_WEBRTC_NAME} PUBLIC datachannel-static uv_a)
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
    POMELO_MULTI_THREAD=1
    POMELO_PLUGIN_WEBRTC_ENABLED_LOG=1
)
