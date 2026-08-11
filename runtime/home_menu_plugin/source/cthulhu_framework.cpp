#include "cthulhu_framework.hpp"

namespace CthulhuFramework
{
    static Plugin g_plugins[MAX_PLUGINS];
    static u32 g_pluginCount = 0;
    static bool g_started = false;

    bool RegisterPlugin(const Plugin &plugin)
    {
        if (g_started || !plugin.name || plugin.requiredApiVersion != API_VERSION ||
            g_pluginCount >= MAX_PLUGINS)
            return false;

        g_plugins[g_pluginCount++] = plugin;
        return true;
    }

    void Start(void)
    {
        if (g_started)
            return;
        g_started = true;
        for (u32 i = 0; i < g_pluginCount; ++i)
            if (g_plugins[i].onFrameworkReady)
                g_plugins[i].onFrameworkReady(API_VERSION);
    }

    void PublishLayout(const LayoutSnapshot &snapshot)
    {
        if (!g_started || snapshot.apiVersion != API_VERSION ||
            snapshot.entryCount > MAX_VISIBLE_TITLES)
            return;

        for (u32 i = 0; i < g_pluginCount; ++i)
            if (g_plugins[i].onLayoutReady)
                g_plugins[i].onLayoutReady(snapshot);
    }

    u32 PluginCount(void)
    {
        return g_pluginCount;
    }
}
