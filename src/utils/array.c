#include <string.h>
#include <assert.h>
#include "array.h"


#ifndef NDEBUG // Debug mode

/// The signature of all arrays
#define POMELO_ARRAY_SIGNATURE 0x303fb1

/// Check the signature of array in debug mode
#define pomelo_array_check_signature(array) \
    assert(array->signature == POMELO_ARRAY_SIGNATURE)

#else // Release mode

/// Check the signature of array in release mode, this is no-op
#define pomelo_array_check_signature(array) (void) array

#endif


void pomelo_array_options_init(pomelo_array_options_t * options) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_array_options_t));
}


pomelo_array_t * pomelo_array_create(pomelo_array_options_t * options) {
    assert(options != NULL);

    if (options->element_size <= 0) {
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_array_t * array =
        pomelo_allocator_malloc_t(allocator, pomelo_array_t);

    if (!array) {
        return NULL;
    }

    memset(array, 0, sizeof(pomelo_array_t));
    array->allocator = allocator;
    array->element_size = options->element_size;

#ifndef NDEBUG
    array->signature = POMELO_ARRAY_SIGNATURE;
#endif

    size_t capacity = options->initial_capacity > 0
        ? options->initial_capacity
        : POMELO_ARRAY_INIT_CAPACITY;
    void * elements =
        pomelo_allocator_malloc(allocator, capacity * array->element_size);

    if (!elements) {
        // Cannot allocate initial elements
        pomelo_allocator_free(allocator, array);
        return NULL;
    }

    // Update elements & capacity
    array->elements = elements;
    array->capacity = capacity;

    return array;
}


void pomelo_array_destroy(pomelo_array_t * array) {
    assert(array != NULL);
    pomelo_array_check_signature(array);

    if (array->elements) {
        pomelo_allocator_free(array->allocator, array->elements);
    }

    pomelo_allocator_free(array->allocator, array);
}


int pomelo_array_ensure_size(pomelo_array_t * array, size_t size) {
    assert(array != NULL);
    pomelo_array_check_signature(array);

    if (size <= array->capacity) {
        return 0; // Nothing to do
    }

    size_t double_capacity = 2 * array->capacity;
    size_t n = size / double_capacity + 1;
    size_t capacity = double_capacity * n;
    pomelo_allocator_t * allocator = array->allocator;

    void * elements =
        pomelo_allocator_malloc(allocator, capacity * array->element_size);
    if (!elements) {
        // Cannot allocate a new space for elements
        return -1;
    }

    memcpy(elements, array->elements, array->capacity * array->element_size);
    pomelo_allocator_free(allocator, array->elements);

    // Update elements & capacity
    array->elements = elements;
    array->capacity = capacity;

    return 0;
}


int pomelo_array_append_p(pomelo_array_t * array, void * p_element) {
    assert(array != NULL);
    assert(p_element != NULL);
    pomelo_array_check_signature(array);

    if (pomelo_array_ensure_size(array, array->size + 1) < 0) {
        return -1;
    }

    size_t offset = (array->size++) * array->element_size;
    memcpy(
        (uint8_t *) array->elements + offset,
        p_element,
        array->element_size
    );

    return 0;
}


int pomelo_array_resize(pomelo_array_t * array, size_t new_size) {
    assert(array != NULL);
    pomelo_array_check_signature(array);

    size_t size = array->size;
    if (pomelo_array_ensure_size(array, new_size) < 0) {
        return -1;
    }

    if (new_size > size) {
        memset(
            ((uint8_t *) array->elements) + size * array->element_size,
            0, // Clear new allocated memory
            (new_size - size) * array->element_size
        );
    }
    array->size = new_size;

    return 0;
}


void * pomelo_array_get_p(pomelo_array_t * array, size_t index) {
    assert(array != NULL);
    pomelo_array_check_signature(array);

    if (index < 0 || index >= array->size) {
        return NULL;
    }
    return ((uint8_t *) array->elements) + index * array->element_size;
}


int pomelo_array_get(pomelo_array_t * array, size_t index, void * p_value) {
    assert(array != NULL);
    assert(p_value != NULL);

    if (index < 0 || index >= array->size) {
        return -1; // Invalid index
    }

    // Copy the value
    memcpy(
        p_value,
        ((uint8_t *) array->elements) + index * array->element_size,
        array->element_size
    );

    return 0;
}


int pomelo_array_set_p(pomelo_array_t * array, size_t index, void * p_value) {
    assert(array != NULL);
    assert(p_value != NULL);

    if (index < 0 || index >= array->size) {
        return -1; // Invalid index
    }

    // Copy the value
    memcpy(
        ((uint8_t *) array->elements) + index * array->element_size,
        p_value,
        array->element_size
    );

    return 0;
}
