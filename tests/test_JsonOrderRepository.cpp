#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

#include "../src/Persistence/JsonOrderRepository.h"

using Model::Order;
using Model::OrderStatus;
using Persistence::JsonOrderRepository;

namespace
{
    class JsonOrderRepositoryTest : public ::testing::Test
    {
    protected:
        std::filesystem::path testRootDirectory_;
        std::string filePath_;

        void SetUp() override
        {
            testRootDirectory_ = std::filesystem::temp_directory_path() / "SampleOrderSystemTests_OrderRepo";
            std::error_code errorCode;
            std::filesystem::remove_all(testRootDirectory_, errorCode);
            std::filesystem::create_directories(testRootDirectory_, errorCode);
            filePath_ = (testRootDirectory_ / "orders.json").string();
        }

        void TearDown() override
        {
            std::error_code errorCode;
            std::filesystem::remove_all(testRootDirectory_, errorCode);
        }

        void WriteRawFile(const std::string& content) const
        {
            std::ofstream file(filePath_, std::ios::binary | std::ios::trunc);
            file << content;
        }
    };

    Order::TimePoint SampleCreatedAt()
    {
        // 밀리초 단위까지 의미 있는 시각(초 단위 배수가 아님)을 사용해 정밀도 손실을 검증한다.
        return Order::TimePoint(std::chrono::milliseconds(1'700'000'000'123LL));
    }

    Order MakeOrder(const std::string& orderId = "O001", int quantity = 10)
    {
        return Order(orderId, "S001", "Customer-A", quantity, SampleCreatedAt());
    }

    // ---- 파일 없음 / 파싱 실패 폴백 ----

    TEST_F(JsonOrderRepositoryTest, 파일이_없으면_예외없이_빈_목록으로_시작한다)
    {
        JsonOrderRepository repository(filePath_);

        EXPECT_TRUE(repository.GetAll().empty());
    }

    TEST_F(JsonOrderRepositoryTest, 깨진_JSON_파일이면_예외없이_빈_목록으로_폴백한다)
    {
        WriteRawFile("[{\"orderId\": \"O001\", ");

        JsonOrderRepository repository(filePath_);

        EXPECT_TRUE(repository.GetAll().empty());
    }

    TEST_F(JsonOrderRepositoryTest, 깨진_JSON_파일이어도_생성자가_예외를_던지지_않는다)
    {
        WriteRawFile("not even json");

        EXPECT_NO_THROW({ JsonOrderRepository repository(filePath_); });
    }

    // ---- Create ----

    TEST_F(JsonOrderRepositoryTest, Create하면_GetAll에_해당_주문이_포함된다)
    {
        JsonOrderRepository repository(filePath_);

        repository.Create(MakeOrder("O001"));

        const auto all = repository.GetAll();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetOrderId(), "O001");
    }

    TEST_F(JsonOrderRepositoryTest, 중복된_주문번호로_Create하면_false를_반환한다)
    {
        JsonOrderRepository repository(filePath_);
        repository.Create(MakeOrder("O001"));

        EXPECT_FALSE(repository.Create(MakeOrder("O001")));
    }

    TEST_F(JsonOrderRepositoryTest, 주문수량이_0인_주문도_정상적으로_Create된다)
    {
        JsonOrderRepository repository(filePath_);

        EXPECT_TRUE(repository.Create(MakeOrder("O001", 0)));
    }

    // ---- FindById ----

    TEST_F(JsonOrderRepositoryTest, 등록된_주문번호로_FindById하면_값을_반환한다)
    {
        JsonOrderRepository repository(filePath_);
        repository.Create(MakeOrder("O001"));

        EXPECT_TRUE(repository.FindById("O001").has_value());
    }

    TEST_F(JsonOrderRepositoryTest, 등록되지_않은_주문번호로_FindById하면_nullopt를_반환한다)
    {
        JsonOrderRepository repository(filePath_);

        EXPECT_FALSE(repository.FindById("UNKNOWN").has_value());
    }

    // ---- Update ----

    TEST_F(JsonOrderRepositoryTest, 존재하는_주문을_Update하면_true를_반환한다)
    {
        JsonOrderRepository repository(filePath_);
        repository.Create(MakeOrder("O001"));
        Order updated = MakeOrder("O001");
        updated.TryTransitionTo(OrderStatus::Confirmed);

        EXPECT_TRUE(repository.Update(updated));
    }

    TEST_F(JsonOrderRepositoryTest, Update하면_변경된_상태가_FindById에_반영된다)
    {
        JsonOrderRepository repository(filePath_);
        repository.Create(MakeOrder("O001"));
        Order updated = MakeOrder("O001");
        updated.TryTransitionTo(OrderStatus::Confirmed);

        repository.Update(updated);

        const auto found = repository.FindById("O001");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetStatus(), OrderStatus::Confirmed);
    }

    TEST_F(JsonOrderRepositoryTest, 존재하지_않는_주문번호로_Update하면_false를_반환한다)
    {
        JsonOrderRepository repository(filePath_);

        EXPECT_FALSE(repository.Update(MakeOrder("UNKNOWN")));
    }

    // ---- Remove ----

    TEST_F(JsonOrderRepositoryTest, 존재하는_주문을_Remove하면_true를_반환한다)
    {
        JsonOrderRepository repository(filePath_);
        repository.Create(MakeOrder("O001"));

        EXPECT_TRUE(repository.Remove("O001"));
    }

    TEST_F(JsonOrderRepositoryTest, 존재하지_않는_주문번호로_Remove하면_false를_반환한다)
    {
        JsonOrderRepository repository(filePath_);

        EXPECT_FALSE(repository.Remove("UNKNOWN"));
    }

    // ---- 상태값 왕복 (모든 OrderStatus) ----

    class JsonOrderRepositoryStatusRoundTripTest
        : public JsonOrderRepositoryTest
        , public ::testing::WithParamInterface<OrderStatus>
    {
    };

    TEST_P(JsonOrderRepositoryStatusRoundTripTest, 저장후_재적재하면_상태값이_동일하게_보존된다)
    {
        {
            JsonOrderRepository repository(filePath_);
            Order order("O001", "S001", "Customer-A", 10, GetParam(), SampleCreatedAt());
            repository.Create(order);
        }

        JsonOrderRepository reloaded(filePath_);
        const auto found = reloaded.FindById("O001");

        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetStatus(), GetParam());
    }

    INSTANTIATE_TEST_SUITE_P(
        모든상태,
        JsonOrderRepositoryStatusRoundTripTest,
        ::testing::Values(
            OrderStatus::Reserved,
            OrderStatus::Rejected,
            OrderStatus::Producing,
            OrderStatus::Confirmed,
            OrderStatus::Released));

    // ---- 시각(time_point) 왕복 정밀도 ----

    TEST_F(JsonOrderRepositoryTest, 저장후_재적재하면_생성일시가_밀리초_단위까지_정확히_보존된다)
    {
        const Order::TimePoint createdAt = SampleCreatedAt();
        {
            JsonOrderRepository repository(filePath_);
            repository.Create(Order("O001", "S001", "Customer-A", 10, createdAt));
        }

        JsonOrderRepository reloaded(filePath_);
        const auto found = reloaded.FindById("O001");

        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetCreatedAt(), createdAt);
    }

    // ---- 재시작(재적재) 시나리오: 가장 중요한 DoD ----

    TEST_F(JsonOrderRepositoryTest, 저장후_새_인스턴스로_재적재하면_주문개수가_동일하다)
    {
        {
            JsonOrderRepository repository(filePath_);
            repository.Create(MakeOrder("O001"));
            repository.Create(MakeOrder("O002"));
        }

        JsonOrderRepository reloaded(filePath_);

        EXPECT_EQ(reloaded.GetAll().size(), 2u);
    }

    TEST_F(JsonOrderRepositoryTest, 저장후_새_인스턴스로_재적재하면_모든_필드가_동일하다)
    {
        {
            JsonOrderRepository repository(filePath_);
            repository.Create(MakeOrder("O001", 25));
        }

        JsonOrderRepository reloaded(filePath_);
        const auto found = reloaded.FindById("O001");

        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetOrderId(), "O001");
        EXPECT_EQ(found->GetSampleId(), "S001");
        EXPECT_EQ(found->GetCustomerName(), "Customer-A");
        EXPECT_EQ(found->GetQuantity(), 25);
        EXPECT_EQ(found->GetStatus(), OrderStatus::Reserved);
    }

    // ---- FIFO 순서 보존 ----

    TEST_F(JsonOrderRepositoryTest, 여러_주문을_생성한_순서가_저장후_재적재해도_그대로_유지된다)
    {
        {
            JsonOrderRepository repository(filePath_);
            repository.Create(MakeOrder("O001"));
            repository.Create(MakeOrder("O002"));
            repository.Create(MakeOrder("O003"));
        }

        JsonOrderRepository reloaded(filePath_);
        const auto all = reloaded.GetAll();

        ASSERT_EQ(all.size(), 3u);
        EXPECT_EQ(all[0].GetOrderId(), "O001");
        EXPECT_EQ(all[1].GetOrderId(), "O002");
        EXPECT_EQ(all[2].GetOrderId(), "O003");
    }
}
