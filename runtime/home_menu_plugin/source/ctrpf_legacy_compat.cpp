#include <3ds.h>

// The pinned CTRPluginFramework archive initializes this legacy newlib table
// itself. Modern devkitARM no longer provides the storage symbol.
extern "C"
{
    u32 __syscalls[14] = {};
}
