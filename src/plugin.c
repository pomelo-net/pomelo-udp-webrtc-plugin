#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "context.h"


/* -------------------------------------------------------------------------- */
/*                               Plugin APIs                                  */
/* -------------------------------------------------------------------------- */


/// @brief Entry of plugin
void pomelo_webrtc_entry(pomelo_plugin_t * plugin) {
    // Use default allocator to create new context
    pomelo_allocator_t * allocator = pomelo_allocator_default();
    pomelo_webrtc_context_t * context =
        pomelo_webrtc_context_create(allocator, plugin);
    plugin->set_data(plugin, context);

}
POMELO_PLUGIN_ENTRY_REGISTER(pomelo_webrtc_entry)


void POMELO_PLUGIN_CALL pomelo_webrtc_on_unload(pomelo_plugin_t * plugin) {
    pomelo_webrtc_context_t * context = plugin->get_data(plugin);
    if (context) {
        // Clear plugin data and destroy the context
        plugin->set_data(plugin, NULL);
        pomelo_webrtc_context_destroy(context);
    }
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

void pomelo_webrtc_log(const char * format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}


void pomelo_webrtc_log_debug(const char * format, ...) {
#ifndef NDEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#else
    (void) format;
#endif // NDEBUG
}
