#include <CTRPluginFramework.hpp>
#include "cthulhu_runtime.hpp"

namespace CTRPluginFramework
{
    static const u32 CTHULHU_BOOT_STARTED = 0x43544831;
    static const u32 CTHULHU_BOOT_READY = 0x43544832;
    static const u32 CTHULHU_BOOT_FAILED = 0x4354483F;

    void PatchProcess(FwkSettings &settings)
    {
        if (FwkSettings::Header)
        {
            FwkSettings::Header->config[31] = CTHULHU_BOOT_STARTED;
            svcFlushProcessDataCache(CUR_PROCESS_HANDLE,
                reinterpret_cast<u32>(&FwkSettings::Header->config[31]), sizeof(u32));
        }
        settings.AllowActionReplay = false;
        settings.ThreadPriority = 0x39;
        settings.WaitTimeToBoot = Time::Zero;
        const bool installed = CthulhuRuntime::InstallHooks();
        if (FwkSettings::Header)
        {
            FwkSettings::Header->config[31] = installed ? CTHULHU_BOOT_READY : CTHULHU_BOOT_FAILED;
            svcFlushProcessDataCache(CUR_PROCESS_HANDLE,
                reinterpret_cast<u32>(&FwkSettings::Header->config[31]), sizeof(u32));
        }
    }

    void OnProcessExit(void)
    {
    }

    int main(void)
    {
        PluginMenu menu("Cthulhu HOME Extension", 0, 7, 0,
                        CthulhuRuntime::Status());
        menu.SynchronizeWithFrame(true);
        menu += new MenuEntry("Runtime status", nullptr, [](MenuEntry *)
        {
            MessageBox("Cthulhu runtime", CthulhuRuntime::Status())();
        });
        menu.Run();
        return 0;
    }
}
