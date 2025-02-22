#ifndef POMELO_UTILS_STRING_BUFFER_SRC_H
#define POMELO_UTILS_STRING_BUFFER_SRC_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pomelo/allocator.h"
#ifdef __cplusplus
extern "C" {
#endif

// Well it is not just string buffer here. Update this name

struct pomelo_string_buffer_s;

/// @brief The string buffer
typedef struct pomelo_string_buffer_s pomelo_string_buffer_t;


struct pomelo_string_buffer_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief The current capacity of buffer
    size_t capacity;

    /// @brief The current writing position
    size_t position;

    /// @brief Buffer data
    char * data;
};


/// @brief Initialize string buffer
int pomelo_string_buffer_on_alloc(
    pomelo_string_buffer_t * string_buffer,
    pomelo_allocator_t * allocator
);


/// @brief Finalize string buffer
void pomelo_string_buffer_on_free(pomelo_string_buffer_t * string_buffer);


/// @brief Ensure the capacity of buffer
bool pomelo_string_buffer_ensure_capacity(
    pomelo_string_buffer_t * string_buffer,
    size_t capacity
);


/// @brief Reset string buffer
int pomelo_string_buffer_init(
    pomelo_string_buffer_t * string_buffer,
    void * unused
);


/// @brief Append a string to the end of string buffer
int pomelo_string_buffer_to_string(
    pomelo_string_buffer_t * string_buffer,
    const char ** output,
    size_t * length
);


/// @brief Wrap up the string and return the binary array of buffer
int pomelo_string_buffer_to_binary(
    pomelo_string_buffer_t * string_buffer,
    const char ** output,
    size_t * size
);


/// @brief Append a string
int pomelo_string_buffer_append_str(
    pomelo_string_buffer_t * string_buffer,
    const char * value
);


/// @brief Append uint64 value
int pomelo_string_buffer_append_u64(
    pomelo_string_buffer_t * string_buffer,
    uint64_t value
);


/// @brief Append int64 value
int pomelo_string_buffer_append_i64(
    pomelo_string_buffer_t * string_buffer,
    int64_t value
);


/// @brief Append a character
int pomelo_string_buffer_append_chr(
    pomelo_string_buffer_t * string_buffer,
    char value
);


/// @brief Append a binary array of character
int pomelo_string_buffer_append_bin(
    pomelo_string_buffer_t * string_buffer,
    const char * array,
    size_t length
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_STRING_BUFFER_SRC_H
