#pragma once

#include "home_menu_request.h"

#include <stddef.h>
#include <stdint.h>

namespace CthulhuHome {

static const size_t LAYOUT_SLOT_COUNT = 360;
static const size_t LAUNCHER_SIZE = 0x2490;
static const size_t SD_SAVE_DATA_SIZE = 0x2DA0;

enum LayoutResult {
    LAYOUT_OK = 0,
    LAYOUT_INVALID_ARGUMENT,
    LAYOUT_INVALID_SIZE,
    LAYOUT_DUPLICATE_REQUEST_TITLE,
    LAYOUT_DUPLICATE_LOADED_TITLE,
    LAYOUT_INVALID_POSITION,
    LAYOUT_DUPLICATE_POSITION,
    LAYOUT_OUTPUT_TOO_SMALL
};

struct LayoutBuffer {
    uint8_t *data;
    size_t size;
    uint8_t mediaType;
};

struct LayoutMutation {
    uint64_t titleId;
    uint8_t mediaType;
    uint16_t slot;
    int16_t oldPosition;
    int16_t newPosition;
    int8_t folder;
};

// Builds a transaction without changing either layout buffer. Icons are sorted
// independently within the main screen (folder -1) and each existing folder.
// The occupied positions in every group are retained; titles never cross folders.
LayoutResult BuildLayoutMutations(const RequestEntry *request, size_t requestCount,
                                  const LayoutBuffer &launcher,
                                  const LayoutBuffer &sdSaveData,
                                  LayoutMutation *mutations,
                                  size_t mutationCapacity,
                                  size_t *mutationCount);

// Applies a transaction produced by BuildLayoutMutations. The buffers and old
// positions are checked again, making stale or mismatched transactions fail closed.
LayoutResult ApplyLayoutMutations(const LayoutMutation *mutations,
                                  size_t mutationCount,
                                  const LayoutBuffer &launcher,
                                  const LayoutBuffer &sdSaveData);

} // namespace CthulhuHome
