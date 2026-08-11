#include "../source/home_menu_request.h"
#include "../source/home_menu_layout.h"

#include <assert.h>
#include <string.h>
#include <vector>

using namespace CthulhuHome;

static RequestEntry Entry(uint64_t id, const char *title) {
    RequestEntry entry = {};
    entry.titleId = id;
    entry.flags = ENTRY_HAS_TITLE;
    for (size_t i = 0; title[i] && i < MAX_TITLE_UNITS - 1; ++i)
        entry.title[i] = static_cast<uint8_t>(title[i]);
    return entry;
}

template <typename T>
static void Put(std::vector<uint8_t> &buffer, size_t offset, T value) {
    memcpy(buffer.data() + offset, &value, sizeof(value));
}

static void PutLayoutEntry(std::vector<uint8_t> &buffer, bool nand, size_t slot,
                           uint64_t titleId, int16_t position, int8_t folder) {
    Put<uint64_t>(buffer, 0x008 + slot * sizeof(uint64_t), titleId);
    Put<int16_t>(buffer, (nand ? 0xD9A : 0xCB0) + slot * sizeof(int16_t), position);
    Put<int8_t>(buffer, (nand ? 0x106A : 0xF80) + slot, folder);
}

int main() {
    const char *crcFixture = "123456789";
    assert(Crc32(crcFixture, strlen(crcFixture)) == 0xCBF43926u);

    RequestEntry alpha = Entry(2, "Animal Crossing");
    RequestEntry zelda = Entry(3, "zelda");
    assert(CompareTitles(alpha, zelda) < 0);

    RequestEntry sameCase = Entry(1, "ANIMAL CROSSING");
    assert(CompareTitles(sameCase, alpha) < 0); // Title ID is the stable tie-breaker.

    RequestEntry unnamed = {};
    unnamed.titleId = 1;
    assert(CompareTitles(alpha, unnamed) < 0);

    RequestEntry entries[] = {alpha, zelda};
    RequestHeader header = {};
    header.magic = REQUEST_MAGIC;
    header.version = REQUEST_VERSION;
    header.algorithm = SORT_ALPHABETICAL;
    header.entryCount = 2;
    header.payloadSize = sizeof(entries);
    header.payloadCrc32 = Crc32(entries, sizeof(entries));

    assert(ValidateRequest(header, entries, sizeof(header) + sizeof(entries)));
    header.payloadCrc32 ^= 1;
    assert(!ValidateRequest(header, entries, sizeof(header) + sizeof(entries)));

    std::vector<uint8_t> nand(LAUNCHER_SIZE, 0xFF);
    std::vector<uint8_t> sd(SD_SAVE_DATA_SIZE, 0xFF);
    PutLayoutEntry(nand, true, 4, 0x100, 10, -1);
    PutLayoutEntry(sd, false, 7, 0x200, 20, -1);
    PutLayoutEntry(nand, true, 8, 0x300, 30, 2);
    PutLayoutEntry(sd, false, 9, 0x400, 4, 2);

    RequestEntry order[] = {Entry(0x200, "Beta"), Entry(0x100, "Alpha"),
                            Entry(0x300, "Folder B"), Entry(0x400, "Folder A"),
                            Entry(0x999, "Hidden")};
    order[0].mediaType = MEDIA_SD;
    order[1].mediaType = MEDIA_NAND;
    order[2].mediaType = MEDIA_NAND;
    order[3].mediaType = MEDIA_SD;
    order[4].mediaType = MEDIA_SD;

    LayoutBuffer nandLayout = {nand.data(), nand.size(), MEDIA_NAND};
    LayoutBuffer sdLayout = {sd.data(), sd.size(), MEDIA_SD};
    LayoutMutation mutations[5] = {};
    size_t mutationCount = 0;
    assert(BuildLayoutMutations(order, 5, nandLayout, sdLayout,
                                mutations, 5, &mutationCount) == LAYOUT_OK);
    assert(mutationCount == 4);
    assert(mutations[0].titleId == 0x200 && mutations[0].newPosition == 10);
    assert(mutations[1].titleId == 0x100 && mutations[1].newPosition == 20);
    assert(mutations[2].titleId == 0x300 && mutations[2].folder == 2 &&
           mutations[2].newPosition == 4);
    assert(mutations[3].titleId == 0x400 && mutations[3].folder == 2 &&
           mutations[3].newPosition == 30);
    assert(ApplyLayoutMutations(mutations, mutationCount, nandLayout, sdLayout) == LAYOUT_OK);

    int16_t actual = 0;
    memcpy(&actual, sd.data() + 0xCB0 + 7 * sizeof(int16_t), sizeof(actual));
    assert(actual == 10);
    memcpy(&actual, nand.data() + 0xD9A + 4 * sizeof(int16_t), sizeof(actual));
    assert(actual == 20);
    memcpy(&actual, sd.data() + 0xCB0 + 9 * sizeof(int16_t), sizeof(actual));
    assert(actual == 30);
    memcpy(&actual, nand.data() + 0xD9A + 8 * sizeof(int16_t), sizeof(actual));
    assert(actual == 4);

    // Applying the same transaction twice is rejected as stale.
    assert(ApplyLayoutMutations(mutations, mutationCount, nandLayout, sdLayout) ==
           LAYOUT_INVALID_ARGUMENT);
    return 0;
}
