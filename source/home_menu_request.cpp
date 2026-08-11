#include "home_menu_request.h"

namespace CthulhuHome {

uint32_t Crc32(const void *data, size_t size) {
    const uint8_t *bytes = static_cast<const uint8_t *>(data);
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }

    return ~crc;
}

bool ValidateRequest(const RequestHeader &header, const RequestEntry *entries,
                     size_t fileSize) {
    if (header.magic != REQUEST_MAGIC || header.version != REQUEST_VERSION)
        return false;
    if (header.algorithm < SORT_ALPHABETICAL || header.algorithm > SORT_CUSTOM)
        return false;
    if (header.entryCount > 720)
        return false;
    if (header.payloadSize != header.entryCount * sizeof(RequestEntry))
        return false;
    if (fileSize != sizeof(RequestHeader) + header.payloadSize)
        return false;
    if (header.entryCount != 0 && entries == 0)
        return false;
    return Crc32(entries, header.payloadSize) == header.payloadCrc32;
}

static uint16_t FoldAscii(uint16_t value) {
    if (value >= 'A' && value <= 'Z')
        return value + ('a' - 'A');
    return value;
}

int CompareTitles(const RequestEntry &left, const RequestEntry &right) {
    const bool leftNamed = (left.flags & ENTRY_HAS_TITLE) != 0;
    const bool rightNamed = (right.flags & ENTRY_HAS_TITLE) != 0;
    if (leftNamed != rightNamed)
        return leftNamed ? -1 : 1;

    for (size_t i = 0; i < MAX_TITLE_UNITS; ++i) {
        const uint16_t a = FoldAscii(left.title[i]);
        const uint16_t b = FoldAscii(right.title[i]);
        if (a != b)
            return a < b ? -1 : 1;
        if (a == 0)
            break;
    }

    if (left.titleId != right.titleId)
        return left.titleId < right.titleId ? -1 : 1;
    if (left.mediaType != right.mediaType)
        return left.mediaType < right.mediaType ? -1 : 1;
    return 0;
}

} // namespace CthulhuHome
