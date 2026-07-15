#pragma once

#include <string>
#include <utility>
#include <vector>

namespace Json
{
    // JSON 값이 가질 수 있는 종류.
    enum class ValueType
    {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    // 외부 라이브러리 없이 자체 구현하는 JSON 값 표현/파서/직렬화기.
    // Persistence 계층이 시료/주문/생산 큐 데이터를 파일로 저장·복원하는 데 사용한다.
    class Value
    {
    public:
        Value();

        static Value MakeNull();
        static Value MakeBool(bool value);
        static Value MakeNumber(double value);
        static Value MakeString(std::string value);
        static Value MakeArray();
        static Value MakeObject();

        ValueType GetType() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsObject() const;

        bool AsBool(bool defaultValue = false) const;
        double AsNumber(double defaultValue = 0.0) const;
        int AsInt(int defaultValue = 0) const;
        long long AsInt64(long long defaultValue = 0) const;
        std::string AsString(const std::string& defaultValue = "") const;

        // Array 전용 연산.
        void Add(Value value);
        const std::vector<Value>& Items() const;

        // Object 전용 연산.
        void Set(const std::string& key, Value value);
        const Value* Find(const std::string& key) const;
        std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
        double GetNumber(const std::string& key, double defaultValue = 0.0) const;
        int GetInt(const std::string& key, int defaultValue = 0) const;
        long long GetInt64(const std::string& key, long long defaultValue = 0) const;
        bool GetBool(const std::string& key, bool defaultValue = false) const;

        // 들여쓰기를 포함한 JSON 문자열로 직렬화한다.
        std::string Dump(int indentSize = 2) const;

        // JSON 문자열을 파싱한다. 형식이 올바르지 않으면 예외를 던지지 않고 false를 반환한다.
        static bool Parse(const std::string& text, Value& outValue);

    private:
        void DumpTo(std::string& output, int indentSize, int currentDepth) const;

        ValueType type_ = ValueType::Null;
        bool boolValue_ = false;
        double numberValue_ = 0.0;
        std::string stringValue_;
        std::vector<Value> arrayValue_;
        std::vector<std::pair<std::string, Value>> objectValue_;
    };
}
