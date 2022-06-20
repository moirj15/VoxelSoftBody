#include "utils.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace utils
{

FILE *OpenFile(const char *file, FilePermissions permissions)
{
    const char *cPermissions[] = {"r", "w", "w+", "rb", "wb", "wb+"};
    FILE *ret = nullptr;
    ret = fopen(file, cPermissions[(uint32_t)permissions]);
    if (!ret) {
        // TODO: better error handling
        printf("FAILED TO OPEN FILE: %s\n", file);
        exit(EXIT_FAILURE);
    }
    return ret;
}

std::string ReadEntireFileAsString(const char *file)
{
    auto *fp = OpenFile(file, FilePermissions::Read);
    fseek(fp, 0, SEEK_END);
    uint64_t length = ftell(fp);
    rewind(fp);
    if (length == 0) {
        printf("Failed to read file size\n");
    }
    std::string data(length, 0);
    fread(data.data(), sizeof(uint8_t), length, fp);
    fclose(fp);
    return data;
}

std::vector<uint8_t> ReadEntireFileAsVector(const char *file)
{
    auto *fp = OpenFile(file, FilePermissions::BinaryRead);
    fseek(fp, 0, SEEK_END);
    uint64_t length = ftell(fp);
    rewind(fp);
    if (length == 0) {
        printf("Failed to read file size\n");
    }
    std::vector<uint8_t> data(length, 0);
    fread(data.data(), sizeof(uint8_t), data.size(), fp);
    fclose(fp);
    return data;
}
} // namespace utils
