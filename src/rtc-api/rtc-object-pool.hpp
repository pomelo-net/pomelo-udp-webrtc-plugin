#ifndef POMELO_WEBRTC_RTC_API_OBJECT_POOL_HPP
#define POMELO_WEBRTC_RTC_API_OBJECT_POOL_HPP
#include <cassert>
#include "pomelo/allocator.h"
#include "utils/list.h"
#include "utils/pool.h"
#include "rtc-api.hpp"
#ifdef __cplusplus
namespace rtc_api {


template <typename T>
class RTCObjectPool {
public:
    RTCObjectPool(RTCContext * context, bool synchronized = true):
        context(context)
    {
        pomelo_allocator_t * allocator = pomelo_allocator_default();
        pomelo_pool_root_options_t pool_options = {
            .allocator = allocator,
            .element_size = sizeof(T),
            .synchronized = synchronized,
            .on_alloc = on_alloc,
            .on_free = on_free,
            .alloc_data = this
        };
        pool = pomelo_pool_root_create(&pool_options);
        if (!pool) {
            throw std::bad_alloc();
        }
    }

    virtual ~RTCObjectPool() {
        if (pool) {
            pomelo_pool_destroy(pool);
            pool = nullptr;
        }
    }

    /// @brief Acquire an object from the pool
    virtual T * acquire() {
        return static_cast<T *>(pomelo_pool_acquire(pool, nullptr));
    }

    /// @brief Release an object to the pool
    virtual void release(T * object) {
        assert(object != nullptr);
        pomelo_pool_release(pool, static_cast<void *>(object));
    }

private:
    /// @brief Alloc callback
    static int on_alloc(void * element, void * arg) {
        assert(element != nullptr);
        assert(arg != nullptr);

        RTCObjectPool * pool = static_cast<RTCObjectPool *>(arg);

        try {
            new (element) T(pool->context);
        } catch (std::exception & e) {
            return -1;
        }
        return 0;
    }

    /// @brief Free callback
    static void on_free(void * element) {
        assert(element != nullptr);
        reinterpret_cast<T *>(element)->~T();
    }

private:
    /// @brief Context
    RTCContext * context;

    /// @brief Internal pool
    pomelo_pool_t * pool;
};


} // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_OBJECT_POOL_HPP
