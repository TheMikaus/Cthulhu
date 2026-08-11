#pragma once

#include <stddef.h>
#include <stdint.h>

namespace CthulhuHome {

static const uint32_t REQUEST_MAGIC = 0x53544843; // "CTHS", little-endian
static const uint16_t REQUEST_VERSION = 3;
static const char SORT_BUILD_VERSION[] = "0.6.1";
static const size_t MAX_TITLE_UNITS = 64;

enum SortAlgorithm : uint16_t {
    SORT_ALPHABETICAL = 1,
    SORT_REVERSE_ALPHABETICAL = 2,
    SORT_LAST_PLAYED = 3,
    SORT_PLAY_TIME = 4,
    SORT_PLAY_COUNT = 5,
    SORT_TITLE_ID = 6,
    SORT_CUSTOM = 7
};

enum MediaType : uint8_t {
    MEDIA_NAND = 0,
    MEDIA_SD = 1
};

enum EntryFlags : uint8_t {
    ENTRY_HAS_TITLE = 1 << 0
};

#pragma pack(push, 1)
struct RequestHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t algorithm;
    uint32_t entryCount;
    uint32_t payloadSize;
    uint32_t payloadCrc32;
};

// Entries are stored in the exact order the HOME Menu runtime should apply.
// The title is retained so the same artifact can back the future search UI.
struct RequestEntry {
    uint64_t titleId;
    uint8_t mediaType;
    uint8_t flags;
    uint16_t reserved;
    uint16_t title[MAX_TITLE_UNITS];
};
#pragma pack(pop)

static_assert(sizeof(RequestHeader) == 20, "Unexpected request header layout");
static_assert(sizeof(RequestEntry) == 140, "Unexpected request entry layout");

uint32_t Crc32(const void *data, size_t size);
bool ValidateRequest(const RequestHeader &header, const RequestEntry *entries,
                     size_t fileSize);
int CompareTitles(const RequestEntry &left, const RequestEntry &right);

} // namespace CthulhuHome
