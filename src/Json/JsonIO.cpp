#include "JsonIO.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace Json
{
    std::optional<std::string> FileIO::ReadAllText(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            return std::nullopt;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool FileIO::WriteAllText(const std::string& filePath, const std::string& content)
    {
        const std::filesystem::path path(filePath);
        if (path.has_parent_path())
        {
            std::error_code errorCode;
            std::filesystem::create_directories(path.parent_path(), errorCode);
        }

        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            return false;
        }

        file << content;
        return true;
    }
}
