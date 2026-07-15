#include "JsonValue.h"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace Json
{
    Value::Value() = default;

    Value Value::MakeNull()
    {
        return Value();
    }

    Value Value::MakeBool(bool value)
    {
        Value result;
        result.type_ = ValueType::Boolean;
        result.boolValue_ = value;
        return result;
    }

    Value Value::MakeNumber(double value)
    {
        Value result;
        result.type_ = ValueType::Number;
        result.numberValue_ = value;
        return result;
    }

    Value Value::MakeString(std::string value)
    {
        Value result;
        result.type_ = ValueType::String;
        result.stringValue_ = std::move(value);
        return result;
    }

    Value Value::MakeArray()
    {
        Value result;
        result.type_ = ValueType::Array;
        return result;
    }

    Value Value::MakeObject()
    {
        Value result;
        result.type_ = ValueType::Object;
        return result;
    }

    ValueType Value::GetType() const { return type_; }
    bool Value::IsNull() const { return type_ == ValueType::Null; }
    bool Value::IsArray() const { return type_ == ValueType::Array; }
    bool Value::IsObject() const { return type_ == ValueType::Object; }

    bool Value::AsBool(bool defaultValue) const
    {
        return type_ == ValueType::Boolean ? boolValue_ : defaultValue;
    }

    double Value::AsNumber(double defaultValue) const
    {
        return type_ == ValueType::Number ? numberValue_ : defaultValue;
    }

    int Value::AsInt(int defaultValue) const
    {
        return type_ == ValueType::Number ? static_cast<int>(numberValue_) : defaultValue;
    }

    long long Value::AsInt64(long long defaultValue) const
    {
        return type_ == ValueType::Number ? static_cast<long long>(numberValue_) : defaultValue;
    }

    std::string Value::AsString(const std::string& defaultValue) const
    {
        return type_ == ValueType::String ? stringValue_ : defaultValue;
    }

    void Value::Add(Value value)
    {
        type_ = ValueType::Array;
        arrayValue_.push_back(std::move(value));
    }

    const std::vector<Value>& Value::Items() const
    {
        return arrayValue_;
    }

    void Value::Set(const std::string& key, Value value)
    {
        type_ = ValueType::Object;
        for (auto& member : objectValue_)
        {
            if (member.first == key)
            {
                member.second = std::move(value);
                return;
            }
        }
        objectValue_.emplace_back(key, std::move(value));
    }

    const Value* Value::Find(const std::string& key) const
    {
        for (const auto& member : objectValue_)
        {
            if (member.first == key)
            {
                return &member.second;
            }
        }
        return nullptr;
    }

    std::string Value::GetString(const std::string& key, const std::string& defaultValue) const
    {
        const Value* found = Find(key);
        return found != nullptr ? found->AsString(defaultValue) : defaultValue;
    }

    double Value::GetNumber(const std::string& key, double defaultValue) const
    {
        const Value* found = Find(key);
        return found != nullptr ? found->AsNumber(defaultValue) : defaultValue;
    }

    int Value::GetInt(const std::string& key, int defaultValue) const
    {
        const Value* found = Find(key);
        return found != nullptr ? found->AsInt(defaultValue) : defaultValue;
    }

    long long Value::GetInt64(const std::string& key, long long defaultValue) const
    {
        const Value* found = Find(key);
        return found != nullptr ? found->AsInt64(defaultValue) : defaultValue;
    }

    bool Value::GetBool(const std::string& key, bool defaultValue) const
    {
        const Value* found = Find(key);
        return found != nullptr ? found->AsBool(defaultValue) : defaultValue;
    }

    namespace
    {
        void AppendEscapedString(std::string& output, const std::string& text)
        {
            output += '"';
            for (char character : text)
            {
                switch (character)
                {
                case '"':  output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default:   output += character; break;
                }
            }
            output += '"';
        }

        // 정수 값은 소수점 없이("10"), 그 외 실수 값은 불필요한 자릿수 없이 출력한다.
        void AppendNumber(std::string& output, double value)
        {
            if (value == static_cast<long long>(value))
            {
                output += std::to_string(static_cast<long long>(value));
                return;
            }

            std::ostringstream stream;
            stream << std::setprecision(15) << value;
            output += stream.str();
        }
    }

    void Value::DumpTo(std::string& output, int indentSize, int currentDepth) const
    {
        const std::string childIndent(static_cast<size_t>(indentSize) * static_cast<size_t>(currentDepth + 1), ' ');
        const std::string closingIndent(static_cast<size_t>(indentSize) * static_cast<size_t>(currentDepth), ' ');

        switch (type_)
        {
        case ValueType::Null:
            output += "null";
            break;
        case ValueType::Boolean:
            output += boolValue_ ? "true" : "false";
            break;
        case ValueType::Number:
            AppendNumber(output, numberValue_);
            break;
        case ValueType::String:
            AppendEscapedString(output, stringValue_);
            break;
        case ValueType::Array:
            if (arrayValue_.empty())
            {
                output += "[]";
                break;
            }
            output += "[\n";
            for (size_t index = 0; index < arrayValue_.size(); ++index)
            {
                output += childIndent;
                arrayValue_[index].DumpTo(output, indentSize, currentDepth + 1);
                if (index + 1 < arrayValue_.size())
                {
                    output += ",";
                }
                output += "\n";
            }
            output += closingIndent + "]";
            break;
        case ValueType::Object:
            if (objectValue_.empty())
            {
                output += "{}";
                break;
            }
            output += "{\n";
            for (size_t index = 0; index < objectValue_.size(); ++index)
            {
                output += childIndent;
                AppendEscapedString(output, objectValue_[index].first);
                output += ": ";
                objectValue_[index].second.DumpTo(output, indentSize, currentDepth + 1);
                if (index + 1 < objectValue_.size())
                {
                    output += ",";
                }
                output += "\n";
            }
            output += closingIndent + "}";
            break;
        }
    }

    std::string Value::Dump(int indentSize) const
    {
        std::string result;
        DumpTo(result, indentSize, 0);
        return result;
    }

    namespace
    {
        // 외부 라이브러리 없이 자체 구현하는 최소 기능의 재귀 하강(recursive-descent) JSON 파서.
        // 형식이 올바르지 않으면 예외를 던지지 않고 실패(false)만 반환한다.
        class JsonParser
        {
        public:
            explicit JsonParser(const std::string& text) : text_(text) {}

            bool ParseDocument(Value& outValue)
            {
                if (!ParseValue(outValue))
                {
                    return false;
                }
                SkipWhitespace();
                return position_ == text_.size();
            }

        private:
            const std::string& text_;
            size_t position_ = 0;

            void SkipWhitespace()
            {
                while (position_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[position_])))
                {
                    ++position_;
                }
            }

            bool Consume(char expected)
            {
                SkipWhitespace();
                if (position_ < text_.size() && text_[position_] == expected)
                {
                    ++position_;
                    return true;
                }
                return false;
            }

            bool ParseValue(Value& outValue)
            {
                SkipWhitespace();
                if (position_ >= text_.size())
                {
                    return false;
                }

                const char nextCharacter = text_[position_];
                if (nextCharacter == '{') return ParseObject(outValue);
                if (nextCharacter == '[') return ParseArray(outValue);
                if (nextCharacter == '"') return ParseString(outValue);
                if (nextCharacter == 't' || nextCharacter == 'f') return ParseBool(outValue);
                if (nextCharacter == 'n') return ParseNull(outValue);
                return ParseNumber(outValue);
            }

            bool ParseObject(Value& outValue)
            {
                if (!Consume('{'))
                {
                    return false;
                }

                outValue = Value::MakeObject();
                if (Consume('}'))
                {
                    return true;
                }

                while (true)
                {
                    Value keyValue;
                    if (!ParseString(keyValue))
                    {
                        return false;
                    }
                    if (!Consume(':'))
                    {
                        return false;
                    }

                    Value memberValue;
                    if (!ParseValue(memberValue))
                    {
                        return false;
                    }
                    outValue.Set(keyValue.AsString(), std::move(memberValue));

                    if (Consume(','))
                    {
                        continue;
                    }
                    return Consume('}');
                }
            }

            bool ParseArray(Value& outValue)
            {
                if (!Consume('['))
                {
                    return false;
                }

                outValue = Value::MakeArray();
                if (Consume(']'))
                {
                    return true;
                }

                while (true)
                {
                    Value itemValue;
                    if (!ParseValue(itemValue))
                    {
                        return false;
                    }
                    outValue.Add(std::move(itemValue));

                    if (Consume(','))
                    {
                        continue;
                    }
                    return Consume(']');
                }
            }

            // \uXXXX 이스케이프를 UTF-8 바이트로 인코딩해 result에 덧붙인다(서로게이트 쌍 포함).
            bool AppendUnicodeEscape(std::string& result)
            {
                if (position_ + 4 > text_.size())
                {
                    return false;
                }

                unsigned int codePoint = 0;
                for (int digitIndex = 0; digitIndex < 4; ++digitIndex)
                {
                    const char hexDigit = text_[position_++];
                    codePoint <<= 4;
                    if (hexDigit >= '0' && hexDigit <= '9') codePoint |= static_cast<unsigned int>(hexDigit - '0');
                    else if (hexDigit >= 'a' && hexDigit <= 'f') codePoint |= static_cast<unsigned int>(hexDigit - 'a' + 10);
                    else if (hexDigit >= 'A' && hexDigit <= 'F') codePoint |= static_cast<unsigned int>(hexDigit - 'A' + 10);
                    else return false;
                }

                if (codePoint >= 0xD800 && codePoint <= 0xDBFF &&
                    position_ + 6 <= text_.size() && text_[position_] == '\\' && text_[position_ + 1] == 'u')
                {
                    position_ += 2;
                    unsigned int lowSurrogate = 0;
                    for (int digitIndex = 0; digitIndex < 4; ++digitIndex)
                    {
                        const char hexDigit = text_[position_++];
                        lowSurrogate <<= 4;
                        if (hexDigit >= '0' && hexDigit <= '9') lowSurrogate |= static_cast<unsigned int>(hexDigit - '0');
                        else if (hexDigit >= 'a' && hexDigit <= 'f') lowSurrogate |= static_cast<unsigned int>(hexDigit - 'a' + 10);
                        else if (hexDigit >= 'A' && hexDigit <= 'F') lowSurrogate |= static_cast<unsigned int>(hexDigit - 'A' + 10);
                        else return false;
                    }
                    codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (lowSurrogate - 0xDC00);
                }

                if (codePoint <= 0x7F)
                {
                    result += static_cast<char>(codePoint);
                }
                else if (codePoint <= 0x7FF)
                {
                    result += static_cast<char>(0xC0 | (codePoint >> 6));
                    result += static_cast<char>(0x80 | (codePoint & 0x3F));
                }
                else if (codePoint <= 0xFFFF)
                {
                    result += static_cast<char>(0xE0 | (codePoint >> 12));
                    result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (codePoint & 0x3F));
                }
                else
                {
                    result += static_cast<char>(0xF0 | (codePoint >> 18));
                    result += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
                    result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (codePoint & 0x3F));
                }
                return true;
            }

            bool ParseString(Value& outValue)
            {
                if (!Consume('"'))
                {
                    return false;
                }

                std::string result;
                while (position_ < text_.size() && text_[position_] != '"')
                {
                    const char character = text_[position_++];
                    if (character != '\\')
                    {
                        result += character;
                        continue;
                    }

                    if (position_ >= text_.size())
                    {
                        return false;
                    }
                    const char escapeCharacter = text_[position_++];
                    switch (escapeCharacter)
                    {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'u':
                        if (!AppendUnicodeEscape(result))
                        {
                            return false;
                        }
                        break;
                    default:
                        return false;
                    }
                }

                if (position_ >= text_.size())
                {
                    return false; // 닫는 따옴표를 찾지 못함
                }
                ++position_; // 닫는 '"' 소비

                outValue = Value::MakeString(std::move(result));
                return true;
            }

            bool ParseBool(Value& outValue)
            {
                if (text_.compare(position_, 4, "true") == 0)
                {
                    position_ += 4;
                    outValue = Value::MakeBool(true);
                    return true;
                }
                if (text_.compare(position_, 5, "false") == 0)
                {
                    position_ += 5;
                    outValue = Value::MakeBool(false);
                    return true;
                }
                return false;
            }

            bool ParseNull(Value& outValue)
            {
                if (text_.compare(position_, 4, "null") == 0)
                {
                    position_ += 4;
                    outValue = Value::MakeNull();
                    return true;
                }
                return false;
            }

            bool ParseNumber(Value& outValue)
            {
                const size_t start = position_;
                if (position_ < text_.size() && (text_[position_] == '-' || text_[position_] == '+'))
                {
                    ++position_;
                }
                while (position_ < text_.size() &&
                    (std::isdigit(static_cast<unsigned char>(text_[position_])) || text_[position_] == '.'
                        || text_[position_] == 'e' || text_[position_] == 'E'
                        || text_[position_] == '+' || text_[position_] == '-'))
                {
                    ++position_;
                }

                if (position_ == start)
                {
                    return false;
                }

                try
                {
                    outValue = Value::MakeNumber(std::stod(text_.substr(start, position_ - start)));
                }
                catch (...)
                {
                    return false;
                }
                return true;
            }
        };
    }

    bool Value::Parse(const std::string& text, Value& outValue)
    {
        JsonParser parser(text);
        return parser.ParseDocument(outValue);
    }
}
