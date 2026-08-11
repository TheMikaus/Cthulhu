#include "../source/home_menu_request.h"

#include <algorithm>
#include <cstdio>
#include <vector>

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    FILE *input = std::fopen(argv[1], "rb");
    if (!input)
        return 3;
    std::fseek(input, 0, SEEK_END);
    long fileSize = std::ftell(input);
    std::rewind(input);
    if (fileSize < static_cast<long>(sizeof(CthulhuHome::RequestHeader)))
        return 4;
    std::vector<unsigned char> bytes(static_cast<size_t>(fileSize));
    if (std::fread(bytes.data(), bytes.size(), 1, input) != 1)
        return 5;
    std::fclose(input);

    auto *header = reinterpret_cast<CthulhuHome::RequestHeader *>(bytes.data());
    auto *entries = reinterpret_cast<CthulhuHome::RequestEntry *>(
        bytes.data() + sizeof(*header));
    if (!CthulhuHome::ValidateRequest(*header, entries, bytes.size()))
        return 6;

    std::stable_sort(entries, entries + header->entryCount,
        [](const CthulhuHome::RequestEntry &left,
           const CthulhuHome::RequestEntry &right) {
            const bool leftNamed = (left.flags & CthulhuHome::ENTRY_HAS_TITLE) != 0;
            const bool rightNamed = (right.flags & CthulhuHome::ENTRY_HAS_TITLE) != 0;
            if (leftNamed != rightNamed)
                return leftNamed;
            return CthulhuHome::CompareTitles(left, right) > 0;
        });
    header->algorithm = CthulhuHome::SORT_REVERSE_ALPHABETICAL;
    header->payloadCrc32 = CthulhuHome::Crc32(entries, header->payloadSize);

    FILE *output = std::fopen(argv[2], "wb");
    if (!output)
        return 7;
    bool ok = std::fwrite(bytes.data(), bytes.size(), 1, output) == 1;
    ok = std::fclose(output) == 0 && ok;
    return ok ? 0 : 8;
}
