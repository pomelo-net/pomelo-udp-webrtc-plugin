#ifndef POMELO_WEBRTC_RTC_API_OBJECT_POOL_HPP
#define POMELO_WEBRTC_RTC_API_OBJECT_POOL_HPP
#include "pomelo/allocator.h"
#include "utils/list.h"
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
        pomelo_list_options_t options;
        pomelo_list_options_init(&options);

        options.allocator = allocator;
        options.element_size = sizeof(RTCBuffer *);
        options.synchronized = synchronized;

        available_objects = pomelo_list_create(&options);
        if (!available_objects) {
            throw std::bad_alloc();
        }

        allocated_objects = pomelo_list_create(&options);
        if (!allocated_objects) {
            throw std::bad_alloc();
        }
    }

    virtual ~RTCObjectPool() {
        if (available_objects) {
            pomelo_list_destroy(available_objects);
            available_objects = nullptr;
        }

        if (allocated_objects) {
            T * object = nullptr;
            while (pomelo_list_pop_front(allocated_objects, &object) == 0) {
                delete object;
            }
            pomelo_list_destroy(allocated_objects);
            allocated_objects = nullptr;
        }
    }

    T * acquire() {
        T * object = nullptr;
        pomelo_list_pop_front(available_objects, &object);
        if (!object) { // No more elements in available list
            object = new (std::nothrow) T(context);
            if (!object) {
                return nullptr;
            }

            pomelo_list_push_back(allocated_objects, object);
        }

        init(object);
        return object;
    }

    void release(T * object) {
        finalize(object);
        pomelo_list_push_back(available_objects, object);
    }

protected:
    /// @brief Initialize object before acquiring
    virtual void init(T * object) {
        (void) object;
        /* noop */
    }

    /// @brief Finalize object after releasing
    virtual void finalize(T * object) {
        (void) object;
        /* noop */
    }

private:
    RTCContext * context;
    pomelo_list_t * available_objects;
    pomelo_list_t * allocated_objects;
};


} // namespace rtc_api
#endif // __cplusplus
#endif // POMELO_WEBRTC_RTC_API_OBJECT_POOL_HPP
