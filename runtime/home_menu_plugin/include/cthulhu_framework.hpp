#pragma once

#include <3ds.h>

namespace CthulhuFramework
{
    static const u32 API_VERSION = 1;
    static const u32 MAX_PLUGINS = 8;
    static const u32 MAX_VISIBLE_TITLES = 360;

    enum MediaType : u8
    {
        MEDIA_NAND = 0,
        MEDIA_SD = 1
    };

    struct TitleEntry
    {
        u64 titleId;
        s16 position;
        s8 folder;
        u8 mediaType;
        u16 sourceSlot;
    };

    struct LayoutSnapshot
    {
        u32 apiVersion;
        u32 sequence;
        u8 mediaType;
        u16 entryCount;
        TitleEntry entries[MAX_VISIBLE_TITLES];
    };

    typedef void (*OnFrameworkReady)(u32 apiVersion);
    typedef void (*OnLayoutReady)(const LayoutSnapshot &snapshot);

    struct Plugin
    {
        const char *name;
        u32 requiredApiVersion;
        OnFrameworkReady onFrameworkReady;
        OnLayoutReady onLayoutReady;
    };

    bool RegisterPlugin(const Plugin &plugin);
    void Start(void);
    void PublishLayout(const LayoutSnapshot &snapshot);
    u32 PluginCount(void);
}
