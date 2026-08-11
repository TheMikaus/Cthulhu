#include <CTRPluginFramework.hpp>
#include "cthulhu_framework.hpp"
#include "cthulhu_runtime.hpp"

#include <cstdio>
#include <cstring>

namespace CthulhuRuntime
{
    using namespace CTRPluginFramework;
    using namespace CthulhuFramework;

    static const char *BUILD_VERSION = "0.7.1-diagnostic";
    static const u64 USA_HOME_MENU_TITLE_ID = 0x0004003000008F02ULL;
    static const u32 NAND_CALL_SITE = 0x00131F34;
    static const u32 SD_CALL_SITE = 0x002253D0;
    static const u32 NAND_PROCESSOR = 0x0013C680;
    static const u32 SD_PROCESSOR = 0x0021BAD4;
    static const u32 SLOT_COUNT = 360;
    static const u32 NAND_SIZE = 0x2490;
    static const u32 SD_SIZE = 0x2DA0;
    static const u32 LOG_LIMIT = 256 * 1024;

    struct LayoutObject
    {
        u8 *raw;
        u8 *processed;
    };

    struct LayoutDescription
    {
        u32 size;
        u32 positionOffset;
        u32 folderOffset;
        u8 mediaType;
    };

    static Hook g_nandHook;
    static Hook g_sdHook;
    static LayoutSnapshot g_snapshot;
    static u32 g_sequence = 0;
    static u32 g_nandEvents = 0;
    static u32 g_sdEvents = 0;
    static char g_status[256] = "Runtime has not initialized.";
    static const char *LOG_PATH = "sdmc:/3ds/Cthulhu/home-menu-framework.log";

    static void ResetLog(void)
    {
        File file;
        if (File::Open(file, LOG_PATH, File::WRITE | File::CREATE | File::TRUNCATE) == 0)
        {
            file.WriteLine("Cthulhu HOME Menu extension framework");
            file.WriteLine(BUILD_VERSION);
            file.WriteLine("mode=read-only; mutations=disabled");
        }
    }

    static void WriteLog(const char *message)
    {
        File file;
        bool truncate = false;
        if (File::Open(file, LOG_PATH, File::READ) == 0)
        {
            truncate = file.GetSize() >= LOG_LIMIT;
            file.Close();
        }
        if (truncate)
        {
            if (File::Open(file, LOG_PATH, File::WRITE | File::CREATE | File::TRUNCATE) == 0)
                file.WriteLine("log reset after reaching 256 KiB limit");
            file.Close();
        }
        if (File::Open(file, LOG_PATH, File::WRITE | File::CREATE | File::APPEND) == 0)
            file.WriteLine(message);
    }

    static void LogSnapshot(const LayoutSnapshot &snapshot)
    {
        char line[192];
        std::snprintf(line, sizeof(line),
                      "layout seq=%lu media=%s entries=%u",
                      snapshot.sequence,
                      snapshot.mediaType == MEDIA_SD ? "SD" : "NAND",
                      snapshot.entryCount);
        WriteLog(line);

        for (u32 i = 0; i < snapshot.entryCount; ++i)
        {
            const TitleEntry &entry = snapshot.entries[i];
            std::snprintf(line, sizeof(line),
                          " entry=%03lu slot=%03u tid=%08lX%08lX pos=%d folder=%d",
                          i, entry.sourceSlot,
                          static_cast<u32>(entry.titleId >> 32),
                          static_cast<u32>(entry.titleId),
                          entry.position, entry.folder);
            WriteLog(line);
        }
    }

    static void DiagnosticReady(u32 apiVersion)
    {
        char line[96];
        std::snprintf(line, sizeof(line), "framework ready api=%lu plugins=%lu",
                      apiVersion, PluginCount());
        WriteLog(line);
    }

    static bool ValidateAndPublish(LayoutObject *object,
                                   const LayoutDescription &description)
    {
        if (!object || !object->raw ||
            !Process::CheckAddress(reinterpret_cast<u32>(object->raw), MEMPERM_READ) ||
            !Process::CheckAddress(reinterpret_cast<u32>(object->raw) + description.size - 1,
                                   MEMPERM_READ))
        {
            WriteLog("layout rejected: unreadable object or raw buffer");
            return false;
        }

        std::memset(&g_snapshot, 0, sizeof(g_snapshot));
        g_snapshot.apiVersion = API_VERSION;
        g_snapshot.sequence = ++g_sequence;
        g_snapshot.mediaType = description.mediaType;

        for (u32 slot = 0; slot < SLOT_COUNT; ++slot)
        {
            u64 titleId = 0;
            s16 position = -1;
            std::memcpy(&titleId, object->raw + 8 + slot * 8, sizeof(titleId));
            if (titleId == 0 || titleId == UINT64_MAX)
                continue;
            if ((titleId >> 48) != 4 || g_snapshot.entryCount >= MAX_VISIBLE_TITLES)
            {
                WriteLog("layout rejected: invalid title ID or excessive entry count");
                return false;
            }

            std::memcpy(&position,
                        object->raw + description.positionOffset + slot * 2,
                        sizeof(position));
            const s8 folder = *reinterpret_cast<s8 *>(
                object->raw + description.folderOffset + slot);
            if (position < -1 || position >= static_cast<s16>(SLOT_COUNT) ||
                folder < -1 || folder > 59)
            {
                WriteLog("layout rejected: invalid position or folder");
                return false;
            }

            TitleEntry &entry = g_snapshot.entries[g_snapshot.entryCount++];
            entry.titleId = titleId;
            entry.position = position;
            entry.folder = folder;
            entry.mediaType = description.mediaType;
            entry.sourceSlot = static_cast<u16>(slot);
        }

        if (g_snapshot.entryCount < 3)
        {
            WriteLog("layout rejected: fewer than three populated entries");
            return false;
        }

        PublishLayout(g_snapshot);
        return true;
    }

    static void RunNandProcessor(LayoutObject *object)
    {
        reinterpret_cast<void (*)(LayoutObject *)>(NAND_PROCESSOR)(object);
    }

    static void RunSdProcessor(LayoutObject *object)
    {
        reinterpret_cast<void (*)(LayoutObject *)>(SD_PROCESSOR)(object);
    }

    static void NandHookCallback(LayoutObject *object)
    {
        RunNandProcessor(object);
        const LayoutDescription description{NAND_SIZE, 0xD9A, 0x106A, MEDIA_NAND};
        if (ValidateAndPublish(object, description))
            ++g_nandEvents;
        std::snprintf(g_status, sizeof(g_status),
                      "v%s read-only; NAND events %lu, SD events %lu",
                      BUILD_VERSION, g_nandEvents, g_sdEvents);
    }

    static void SdHookCallback(LayoutObject *object)
    {
        RunSdProcessor(object);
        const LayoutDescription description{SD_SIZE, 0xCB0, 0xF80, MEDIA_SD};
        if (ValidateAndPublish(object, description))
            ++g_sdEvents;
        std::snprintf(g_status, sizeof(g_status),
                      "v%s read-only; NAND events %lu, SD events %lu",
                      BUILD_VERSION, g_nandEvents, g_sdEvents);
    }

    static bool CheckInstruction(u32 address, u32 expected)
    {
        u32 actual = 0;
        return Process::Read32(address, actual) && actual == expected;
    }

    bool InstallHooks(void)
    {
        ResetLog();
        char line[128];
        std::snprintf(line, sizeof(line), "title=%08lX%08lX",
                      static_cast<u32>(Process::GetTitleID() >> 32),
                      static_cast<u32>(Process::GetTitleID()));
        WriteLog(line);

        if (Process::GetTitleID() != USA_HOME_MENU_TITLE_ID)
        {
            std::snprintf(g_status, sizeof(g_status), "Unsupported HOME Menu title ID.");
            WriteLog("startup rejected: unsupported title ID");
            return false;
        }

        if (!CheckInstruction(NAND_CALL_SITE, 0xEB0029D1) ||
            !CheckInstruction(NAND_CALL_SITE - 4, 0xE1A00004) ||
            !CheckInstruction(SD_CALL_SITE, 0xEBFFD9BF) ||
            !CheckInstruction(SD_CALL_SITE - 4, 0xE58D9004) ||
            !CheckInstruction(NAND_PROCESSOR, 0xE92D47F0) ||
            !CheckInstruction(SD_PROCESSOR, 0xE92D4FF0))
        {
            std::snprintf(g_status, sizeof(g_status),
                          "Code signature mismatch; no hooks installed.");
            WriteLog("startup rejected: code signature mismatch");
            return false;
        }

        const Plugin diagnostics{"layout-diagnostics", API_VERSION,
                                 &DiagnosticReady, &LogSnapshot};
        if (!RegisterPlugin(diagnostics))
        {
            WriteLog("startup rejected: diagnostic plugin registration failed");
            return false;
        }
        CthulhuFramework::Start();

        g_nandHook.Initialize(NAND_CALL_SITE,
                              reinterpret_cast<u32>(&NandHookCallback));
        g_nandHook.SetFlags(USE_LR_TO_RETURN);
        g_sdHook.Initialize(SD_CALL_SITE, reinterpret_cast<u32>(&SdHookCallback));
        g_sdHook.SetFlags(USE_LR_TO_RETURN);
        if (g_nandHook.Enable() != HookResult::Success ||
            g_sdHook.Enable() != HookResult::Success)
        {
            g_nandHook.Disable();
            g_sdHook.Disable();
            std::snprintf(g_status, sizeof(g_status), "Hook installation failed safely.");
            WriteLog("startup rejected: hook installation failed");
            return false;
        }

        std::snprintf(g_status, sizeof(g_status),
                      "v%s read-only hooks armed; API %lu",
                      BUILD_VERSION, API_VERSION);
        WriteLog("startup complete: read-only hooks armed");
        return true;
    }

    const char *Status(void)
    {
        return g_status;
    }
}
