// I like this better than stdbool -- enum somehow feels slightly less invasive than a #define... though
// the define will work better for #ifdefs and stuff
#pragma once

#if ESP_PLATFORM
// Looks like we get collisions, likely from others already including stdbool
#include <stdbool.h>
#else
typedef enum
{
    false,
    true
} bool;

#endif
