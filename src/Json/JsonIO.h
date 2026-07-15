#pragma once

#include <optional>
#include <string>

namespace Json
{
    // JSON(텍스트) 파일 읽기/쓰기를 전담하는 유틸리티.
    // 상위 디렉터리가 없으면 쓰기 시점에 자동으로 생성한다.
    class FileIO
    {
    public:
        static std::optional<std::string> ReadAllText(const std::string& filePath);
        static bool WriteAllText(const std::string& filePath, const std::string& content);
    };
}
