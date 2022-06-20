#pragma once
#include <string>
#include <vector>

enum class FilePermissions {
    Read,
    Write,
    ReadWrite,
    BinaryRead,
    BinaryWrite,
    BinaryReadWrite,
};

namespace utils
{

FILE *OpenFile(const char *file, FilePermissions permissions);
std::string ReadEntireFileAsString(const char *file);
std::vector<uint8_t> ReadEntireFileAsVector(const char *file);
}