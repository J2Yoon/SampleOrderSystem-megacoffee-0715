#include <gtest/gtest.h>

#include "../src/Json/JsonValue.h"

using Json::Value;
using Json::ValueType;

namespace
{
    // ---- 타입별 생성 및 접근자 ----

    TEST(JsonValueMake, MakeNull로_생성하면_GetType이_Null이다)
    {
        Value value = Value::MakeNull();

        EXPECT_EQ(value.GetType(), ValueType::Null);
    }

    TEST(JsonValueMake, MakeNull로_생성하면_IsNull이_true이다)
    {
        Value value = Value::MakeNull();

        EXPECT_TRUE(value.IsNull());
    }

    TEST(JsonValueMake, 기본_생성자로_생성한_Value도_Null이다)
    {
        Value value;

        EXPECT_TRUE(value.IsNull());
    }

    TEST(JsonValueMake, MakeBool_true로_생성하면_AsBool이_true를_반환한다)
    {
        Value value = Value::MakeBool(true);

        EXPECT_TRUE(value.AsBool());
    }

    TEST(JsonValueMake, MakeBool_false로_생성하면_AsBool이_false를_반환한다)
    {
        Value value = Value::MakeBool(false);

        EXPECT_FALSE(value.AsBool());
    }

    TEST(JsonValueMake, MakeNumber로_생성하면_AsNumber가_그대로_반환한다)
    {
        Value value = Value::MakeNumber(3.14);

        EXPECT_DOUBLE_EQ(value.AsNumber(), 3.14);
    }

    TEST(JsonValueMake, MakeNumber로_생성한_정수값을_AsInt가_그대로_반환한다)
    {
        Value value = Value::MakeNumber(42.0);

        EXPECT_EQ(value.AsInt(), 42);
    }

    TEST(JsonValueMake, MakeNumber로_생성한_큰_정수값을_AsInt64가_그대로_반환한다)
    {
        Value value = Value::MakeNumber(1'700'000'000'123.0);

        EXPECT_EQ(value.AsInt64(), 1'700'000'000'123LL);
    }

    TEST(JsonValueMake, MakeString으로_생성하면_AsString이_그대로_반환한다)
    {
        Value value = Value::MakeString("hello");

        EXPECT_EQ(value.AsString(), "hello");
    }

    TEST(JsonValueMake, MakeArray로_생성하면_IsArray가_true이다)
    {
        Value value = Value::MakeArray();

        EXPECT_TRUE(value.IsArray());
    }

    TEST(JsonValueMake, MakeObject로_생성하면_IsObject가_true이다)
    {
        Value value = Value::MakeObject();

        EXPECT_TRUE(value.IsObject());
    }

    // ---- 타입 불일치 시 기본값 반환 ----

    TEST(JsonValueDefault, Null값에_AsBool을_호출하면_지정한_기본값을_반환한다)
    {
        Value value = Value::MakeNull();

        EXPECT_TRUE(value.AsBool(true));
    }

    TEST(JsonValueDefault, Null값에_AsString을_호출하면_지정한_기본값을_반환한다)
    {
        Value value = Value::MakeNull();

        EXPECT_EQ(value.AsString("default"), "default");
    }

    // ---- Array 연산 ----

    TEST(JsonValueArray, Add로_추가한_항목_개수만큼_Items가_반환된다)
    {
        Value array = Value::MakeArray();
        array.Add(Value::MakeNumber(1));
        array.Add(Value::MakeNumber(2));

        EXPECT_EQ(array.Items().size(), 2u);
    }

    TEST(JsonValueArray, Add로_추가한_순서대로_Items에_보존된다)
    {
        Value array = Value::MakeArray();
        array.Add(Value::MakeString("first"));
        array.Add(Value::MakeString("second"));

        EXPECT_EQ(array.Items()[0].AsString(), "first");
        EXPECT_EQ(array.Items()[1].AsString(), "second");
    }

    // ---- Object 연산 ----

    TEST(JsonValueObject, Set한_키를_Find로_찾을_수_있다)
    {
        Value object = Value::MakeObject();
        object.Set("name", Value::MakeString("Sample-A"));

        const Value* found = object.Find("name");

        ASSERT_NE(found, nullptr);
        EXPECT_EQ(found->AsString(), "Sample-A");
    }

    TEST(JsonValueObject, 존재하지_않는_키를_Find하면_nullptr을_반환한다)
    {
        Value object = Value::MakeObject();

        EXPECT_EQ(object.Find("missing"), nullptr);
    }

    TEST(JsonValueObject, 같은_키로_Set을_다시_호출하면_값이_덮어써진다)
    {
        Value object = Value::MakeObject();
        object.Set("stock", Value::MakeNumber(10));
        object.Set("stock", Value::MakeNumber(20));

        EXPECT_EQ(object.GetInt("stock"), 20);
    }

    TEST(JsonValueObject, GetString은_존재하지_않는_키에_대해_기본값을_반환한다)
    {
        Value object = Value::MakeObject();

        EXPECT_EQ(object.GetString("missing", "fallback"), "fallback");
    }

    TEST(JsonValueObject, GetNumber는_존재하지_않는_키에_대해_기본값을_반환한다)
    {
        Value object = Value::MakeObject();

        EXPECT_DOUBLE_EQ(object.GetNumber("missing", 1.5), 1.5);
    }

    TEST(JsonValueObject, GetBool은_존재하지_않는_키에_대해_기본값을_반환한다)
    {
        Value object = Value::MakeObject();

        EXPECT_TRUE(object.GetBool("missing", true));
    }

    // ---- Dump -> Parse 왕복 ----

    TEST(JsonValueRoundTrip, Object를_Dump한_뒤_Parse하면_문자열_필드가_보존된다)
    {
        Value original = Value::MakeObject();
        original.Set("id", Value::MakeString("S001"));
        original.Set("stock", Value::MakeNumber(100));
        original.Set("yield", Value::MakeNumber(0.85));
        original.Set("active", Value::MakeBool(true));

        const std::string dumped = original.Dump();

        Value parsed;
        ASSERT_TRUE(Value::Parse(dumped, parsed));
        EXPECT_EQ(parsed.GetString("id"), "S001");
    }

    TEST(JsonValueRoundTrip, Object를_Dump한_뒤_Parse하면_숫자_필드가_보존된다)
    {
        Value original = Value::MakeObject();
        original.Set("stock", Value::MakeNumber(100));

        Value parsed;
        ASSERT_TRUE(Value::Parse(original.Dump(), parsed));

        EXPECT_EQ(parsed.GetInt("stock"), 100);
    }

    TEST(JsonValueRoundTrip, Object를_Dump한_뒤_Parse하면_실수_필드가_정밀도_손실없이_보존된다)
    {
        Value original = Value::MakeObject();
        original.Set("yield", Value::MakeNumber(0.85));

        Value parsed;
        ASSERT_TRUE(Value::Parse(original.Dump(), parsed));

        EXPECT_DOUBLE_EQ(parsed.GetNumber("yield"), 0.85);
    }

    TEST(JsonValueRoundTrip, Object를_Dump한_뒤_Parse하면_불리언_필드가_보존된다)
    {
        Value original = Value::MakeObject();
        original.Set("active", Value::MakeBool(true));

        Value parsed;
        ASSERT_TRUE(Value::Parse(original.Dump(), parsed));

        EXPECT_TRUE(parsed.GetBool("active"));
    }

    TEST(JsonValueRoundTrip, 빈_배열을_Dump한_뒤_Parse하면_빈_배열로_복원된다)
    {
        Value original = Value::MakeArray();

        Value parsed;
        ASSERT_TRUE(Value::Parse(original.Dump(), parsed));

        EXPECT_TRUE(parsed.IsArray());
        EXPECT_EQ(parsed.Items().size(), 0u);
    }

    TEST(JsonValueRoundTrip, 중첩된_배열을_Dump한_뒤_Parse하면_모든_원소가_보존된다)
    {
        Value array = Value::MakeArray();
        Value first = Value::MakeObject();
        first.Set("orderId", Value::MakeString("O001"));
        Value second = Value::MakeObject();
        second.Set("orderId", Value::MakeString("O002"));
        array.Add(first);
        array.Add(second);

        Value parsed;
        ASSERT_TRUE(Value::Parse(array.Dump(), parsed));

        ASSERT_EQ(parsed.Items().size(), 2u);
        EXPECT_EQ(parsed.Items()[0].GetString("orderId"), "O001");
        EXPECT_EQ(parsed.Items()[1].GetString("orderId"), "O002");
    }

    TEST(JsonValueRoundTrip, 특수문자가_포함된_문자열을_Dump한_뒤_Parse하면_그대로_보존된다)
    {
        Value original = Value::MakeObject();
        original.Set("name", Value::MakeString("Line1\nLine2\t\"quoted\"\\backslash"));

        Value parsed;
        ASSERT_TRUE(Value::Parse(original.Dump(), parsed));

        EXPECT_EQ(parsed.GetString("name"), "Line1\nLine2\t\"quoted\"\\backslash");
    }

    TEST(JsonValueRoundTrip, 음수_실수값을_Dump한_뒤_Parse하면_그대로_보존된다)
    {
        Value original = Value::MakeObject();
        original.Set("value", Value::MakeNumber(-12.5));

        Value parsed;
        ASSERT_TRUE(Value::Parse(original.Dump(), parsed));

        EXPECT_DOUBLE_EQ(parsed.GetNumber("value"), -12.5);
    }

    // ---- 잘못된 JSON Parse ----

    TEST(JsonValueParse, 닫는_중괄호가_없는_객체를_Parse하면_false를_반환한다)
    {
        Value parsed;

        EXPECT_FALSE(Value::Parse("{\"id\": \"S001\"", parsed));
    }

    TEST(JsonValueParse, 콜론이_누락된_객체를_Parse하면_false를_반환한다)
    {
        Value parsed;

        EXPECT_FALSE(Value::Parse("{\"id\" \"S001\"}", parsed));
    }

    TEST(JsonValueParse, 완전히_빈_문자열을_Parse하면_false를_반환한다)
    {
        Value parsed;

        EXPECT_FALSE(Value::Parse("", parsed));
    }

    TEST(JsonValueParse, 닫는_따옴표가_없는_문자열을_Parse하면_false를_반환한다)
    {
        Value parsed;

        EXPECT_FALSE(Value::Parse("{\"id\": \"S001}", parsed));
    }

    TEST(JsonValueParse, 유효한_JSON_뒤에_불필요한_문자가_붙으면_false를_반환한다)
    {
        Value parsed;

        EXPECT_FALSE(Value::Parse("{}garbage", parsed));
    }

    TEST(JsonValueParse, 잘못된_JSON을_Parse해도_예외를_던지지_않는다)
    {
        Value parsed;

        EXPECT_NO_THROW(Value::Parse("{invalid", parsed));
    }
}
