#ifndef POMELO_WEBRTC_COMMON_MACRO_H
#define POMELO_WEBRTC_COMMON_MACRO_H

/// Check if the flag is set
#define POMELO_CHECK_FLAG(value, flag) ((value) & (flag))

/// Set the flag
#define POMELO_SET_FLAG(value, flag) ((value) |= (flag))

/// Unset a flag
#define POMELO_UNSET_FLAG(value, flag) ((value) &= ~(flag))

/// Get the array length
#define POMELO_ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#endif // POMELO_WEBRTC_COMMON_MACRO_H
