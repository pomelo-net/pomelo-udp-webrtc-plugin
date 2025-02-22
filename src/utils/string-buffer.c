#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include "string-buffer.h"


#define POMELO_PLUGIN_STRING_BUFFER_INITIAL_CAPACITY    1024
#define POMELO_PLUGIN_STRING_BUFFER_MAX_CAPACITY        256 * 1024
#define POMELO_PLUGIN_STRING_BUFFER_POOL_MAX_AVAILABLE  10000



bool pomelo_string_buffer_ensure_capacity(
    pomelo_string_buffer_t * string_buffer,
    size_t capacity
) {
    assert(string_buffer != NULL);
    if (capacity < string_buffer->capacity) {
        // Nothing to do
        return true;
    }

    if (capacity >= POMELO_PLUGIN_STRING_BUFFER_MAX_CAPACITY) {
        // Exceed the max length
        return false;
    }

    size_t new_capacity = 2 * string_buffer->capacity;
    if (new_capacity > POMELO_PLUGIN_STRING_BUFFER_MAX_CAPACITY) {
        new_capacity = POMELO_PLUGIN_STRING_BUFFER_MAX_CAPACITY;
    }

    pomelo_allocator_t * allocator = string_buffer->allocator;
    char * buffer = pomelo_allocator_malloc(allocator, new_capacity);
    if (!buffer) {
        // Failed to allocate new memory block
        return false;
    }

    memcpy(buffer, string_buffer->data, string_buffer->position);
    pomelo_allocator_free(allocator, string_buffer->data);
    
    string_buffer->data = buffer;
    string_buffer->capacity = new_capacity;

    return true;
}


int pomelo_string_buffer_on_alloc(
    pomelo_string_buffer_t * string_buffer,
    pomelo_allocator_t * allocator
) {
    assert(string_buffer != NULL);
    assert(allocator != NULL);
    memset(string_buffer, 0, sizeof(pomelo_string_buffer_t));
    string_buffer->allocator = allocator;
    string_buffer->data = pomelo_allocator_malloc(
        allocator,
        POMELO_PLUGIN_STRING_BUFFER_INITIAL_CAPACITY
    );
    if (!string_buffer->data) {
        return -1;
    }

    string_buffer->capacity = POMELO_PLUGIN_STRING_BUFFER_INITIAL_CAPACITY;
    return 0;
}


void pomelo_string_buffer_on_free(pomelo_string_buffer_t * string_buffer) {
    assert(string_buffer != NULL);
    if (string_buffer->data) {
        pomelo_allocator_free(string_buffer->allocator, string_buffer->data);
        string_buffer->data = NULL;
    }
}


int pomelo_string_buffer_init(
    pomelo_string_buffer_t * string_buffer,
    void * unused
) {
    assert(string_buffer != NULL);
    (void) unused;
    string_buffer->position = 0;
    return 0;
}


int pomelo_string_buffer_to_string(
    pomelo_string_buffer_t * string_buffer,
    const char ** output,
    size_t * length
) {
    assert(string_buffer != NULL);

    // Append the terminated character
    int ret = pomelo_string_buffer_append_chr(string_buffer, '\0');
    if (ret < 0) {
        return ret;
    }

    if (output) {
        *output = string_buffer->data;
    }

    if (length) {
        *length = string_buffer->position - 1;
    }

    return 0;
}


int pomelo_string_buffer_to_binary(
    pomelo_string_buffer_t * string_buffer,
    const char ** output,
    size_t * size
) {
    assert(string_buffer != NULL);

    if (output) {
        *output = string_buffer->data;
    }

    if (size) {
        *size += string_buffer->position;
    }

    return 0;
}


int pomelo_string_buffer_append_str(
    pomelo_string_buffer_t * string_buffer,
    const char * value
) {
    assert(string_buffer != NULL);
    assert(value != NULL);

    size_t length = strlen(value);
    if (length == 0) {
        return 0;
    }

    bool ret = pomelo_string_buffer_ensure_capacity(
        string_buffer,
        string_buffer->position + length
    );
    if (!ret) {
        return -1;
    }

    memcpy(string_buffer->data + string_buffer->position, value, length);
    string_buffer->position += length;

    return 0;
}


int pomelo_string_buffer_append_u64(
    pomelo_string_buffer_t * string_buffer,
    uint64_t value
) {
    assert(string_buffer != NULL);
    char value_buffer[21]; // Max uint64 value can fit in 20 characters
    sprintf(value_buffer, "%" PRIu64, value);
    return pomelo_string_buffer_append_str(string_buffer, value_buffer);
}


int pomelo_string_buffer_append_i64(
    pomelo_string_buffer_t * string_buffer,
    int64_t value
) {
    char value_buffer[21]; // Max uint64 value can fit in 20 characters
    sprintf(value_buffer, "%" PRIi64, value);
    return pomelo_string_buffer_append_str(string_buffer, value_buffer);
}


int pomelo_string_buffer_append_chr(
    pomelo_string_buffer_t * string_buffer,
    char value
) {
    assert(string_buffer != NULL);
    bool ret = pomelo_string_buffer_ensure_capacity(
        string_buffer,
        string_buffer->position + 1
    );
    if (!ret) {
        return -1;
    }

    string_buffer->data[string_buffer->position++] = value;
    return 0;
}


int pomelo_string_buffer_append_bin(
    pomelo_string_buffer_t * string_buffer,
    const char * array,
    size_t length
) {
    assert(string_buffer != NULL);
    if (length == 0) {
        return 0;
    }

    bool ret = pomelo_string_buffer_ensure_capacity(
        string_buffer,
        string_buffer->position + length
    );

    if (!ret) {
        return -1;
    }

    memcpy(string_buffer->data + string_buffer->position, array, length);
    string_buffer->position += length;

    return 0;
}
