#include "home_menu_layout.h"

#include <algorithm>
#include <string.h>
#include <vector>

namespace CthulhuHome {

struct LayoutOffsets {
    size_t titleIds;
    size_t positions;
    size_t folders;
};

static bool GetOffsets(const LayoutBuffer &layout, LayoutOffsets *offsets) {
    if (!layout.data || !offsets)
        return false;

    if (layout.mediaType == MEDIA_NAND && layout.size == LAUNCHER_SIZE) {
        offsets->titleIds = 0x008;
        offsets->positions = 0xD9A;
        offsets->folders = 0x106A;
        return true;
    }
    if (layout.mediaType == MEDIA_SD && layout.size == SD_SAVE_DATA_SIZE) {
        offsets->titleIds = 0x008;
        offsets->positions = 0xCB0;
        offsets->folders = 0xF80;
        return true;
    }
    return false;
}

template <typename T>
static T ReadValue(const uint8_t *data, size_t offset) {
    T value;
    memcpy(&value, data + offset, sizeof(value));
    return value;
}

template <typename T>
static void WriteValue(uint8_t *data, size_t offset, T value) {
    memcpy(data + offset, &value, sizeof(value));
}

static LayoutResult FindTitle(const LayoutBuffer &layout, uint64_t titleId,
                              uint16_t *foundSlot, int16_t *position,
                              int8_t *folder) {
    LayoutOffsets offsets;
    if (!GetOffsets(layout, &offsets))
        return LAYOUT_INVALID_SIZE;

    bool found = false;
    for (size_t slot = 0; slot < LAYOUT_SLOT_COUNT; ++slot) {
        const uint64_t candidate = ReadValue<uint64_t>(
            layout.data, offsets.titleIds + slot * sizeof(uint64_t));
        if (candidate != titleId)
            continue;
        if (found)
            return LAYOUT_DUPLICATE_LOADED_TITLE;
        found = true;
        *foundSlot = static_cast<uint16_t>(slot);
        *position = ReadValue<int16_t>(
            layout.data, offsets.positions + slot * sizeof(int16_t));
        *folder = ReadValue<int8_t>(layout.data, offsets.folders + slot);
    }

    return found ? LAYOUT_OK : LAYOUT_INVALID_ARGUMENT;
}

LayoutResult BuildLayoutMutations(const RequestEntry *request, size_t requestCount,
                                  const LayoutBuffer &launcher,
                                  const LayoutBuffer &sdSaveData,
                                  LayoutMutation *mutations,
                                  size_t mutationCapacity,
                                  size_t *mutationCount) {
    if (!mutationCount || (requestCount && (!request || !mutations)))
        return LAYOUT_INVALID_ARGUMENT;
    *mutationCount = 0;

    LayoutOffsets unused;
    if (!GetOffsets(launcher, &unused) || !GetOffsets(sdSaveData, &unused))
        return LAYOUT_INVALID_SIZE;

    for (size_t i = 0; i < requestCount; ++i) {
        for (size_t j = 0; j < i; ++j) {
            if (request[i].titleId == request[j].titleId &&
                request[i].mediaType == request[j].mediaType)
                return LAYOUT_DUPLICATE_REQUEST_TITLE;
        }
    }

    std::vector<LayoutMutation> pending;
    pending.reserve(requestCount);
    for (size_t i = 0; i < requestCount; ++i) {
        const LayoutBuffer *layout = 0;
        if (request[i].mediaType == MEDIA_NAND)
            layout = &launcher;
        else if (request[i].mediaType == MEDIA_SD)
            layout = &sdSaveData;
        else
            return LAYOUT_INVALID_ARGUMENT;

        uint16_t slot = 0;
        int16_t position = 0;
        int8_t folder = 0;
        const LayoutResult result = FindTitle(*layout, request[i].titleId,
                                              &slot, &position, &folder);
        if (result == LAYOUT_INVALID_ARGUMENT)
            continue; // Installed titles need not be visible on HOME Menu.
        if (result != LAYOUT_OK)
            return result;
        const int16_t positionLimit = folder == -1 ?
            static_cast<int16_t>(LAYOUT_SLOT_COUNT) : 60;
        if (folder < -1 || folder >= 60 || position < 0 || position >= positionLimit)
            return LAYOUT_INVALID_POSITION;

        LayoutMutation mutation = {};
        mutation.titleId = request[i].titleId;
        mutation.mediaType = request[i].mediaType;
        mutation.slot = slot;
        mutation.oldPosition = position;
        mutation.folder = folder;
        pending.push_back(mutation);
    }

    if (mutationCapacity < pending.size())
        return LAYOUT_OUTPUT_TOO_SMALL;
    for (int folder = -1; folder < 60; ++folder) {
        std::vector<int16_t> positions;
        std::vector<size_t> indexes;
        for (size_t i = 0; i < pending.size(); ++i) {
            if (pending[i].folder == folder) {
                positions.push_back(pending[i].oldPosition);
                indexes.push_back(i);
            }
        }
        std::sort(positions.begin(), positions.end());
        if (std::adjacent_find(positions.begin(), positions.end()) != positions.end())
            return LAYOUT_DUPLICATE_POSITION;
        for (size_t i = 0; i < indexes.size(); ++i)
            pending[indexes[i]].newPosition = positions[i];
    }
    for (size_t i = 0; i < pending.size(); ++i)
        mutations[i] = pending[i];
    *mutationCount = pending.size();
    return LAYOUT_OK;
}

LayoutResult ApplyLayoutMutations(const LayoutMutation *mutations,
                                  size_t mutationCount,
                                  const LayoutBuffer &launcher,
                                  const LayoutBuffer &sdSaveData) {
    if (mutationCount && !mutations)
        return LAYOUT_INVALID_ARGUMENT;

    for (size_t i = 0; i < mutationCount; ++i) {
        const int16_t positionLimit = mutations[i].folder == -1 ?
            static_cast<int16_t>(LAYOUT_SLOT_COUNT) : 60;
        if (mutations[i].folder < -1 || mutations[i].folder >= 60 ||
            mutations[i].newPosition < 0 || mutations[i].newPosition >= positionLimit)
            return LAYOUT_INVALID_POSITION;
        for (size_t j = 0; j < i; ++j) {
            if (mutations[i].folder == mutations[j].folder &&
                mutations[i].newPosition == mutations[j].newPosition)
                return LAYOUT_DUPLICATE_POSITION;
        }
        const LayoutBuffer *layout = mutations[i].mediaType == MEDIA_NAND
            ? &launcher : mutations[i].mediaType == MEDIA_SD ? &sdSaveData : 0;
        LayoutOffsets offsets;
        if (!layout || !GetOffsets(*layout, &offsets) ||
            mutations[i].slot >= LAYOUT_SLOT_COUNT)
            return LAYOUT_INVALID_ARGUMENT;
        const size_t slot = mutations[i].slot;
        if (ReadValue<uint64_t>(layout->data,
                offsets.titleIds + slot * sizeof(uint64_t)) != mutations[i].titleId ||
            ReadValue<int16_t>(layout->data,
                offsets.positions + slot * sizeof(int16_t)) != mutations[i].oldPosition ||
            ReadValue<int8_t>(layout->data, offsets.folders + slot) != mutations[i].folder)
            return LAYOUT_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < mutationCount; ++i) {
        const LayoutBuffer &layout = mutations[i].mediaType == MEDIA_NAND
            ? launcher : sdSaveData;
        LayoutOffsets offsets = {};
        if (!GetOffsets(layout, &offsets))
            return LAYOUT_INVALID_SIZE;
        WriteValue<int16_t>(layout.data,
            offsets.positions + mutations[i].slot * sizeof(int16_t),
            mutations[i].newPosition);
    }
    return LAYOUT_OK;
}

} // namespace CthulhuHome
