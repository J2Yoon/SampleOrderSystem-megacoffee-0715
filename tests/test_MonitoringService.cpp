#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <vector>

#include "../src/Controller/MonitoringService.h"
#include "../src/Model/Order.h"
#include "../src/Model/OrderStatus.h"
#include "../src/Model/Sample.h"

using Model::Order;
using Model::OrderStatus;
using Model::Sample;

namespace
{
    Order::TimePoint MakeLocalDateTimePoint(int year, int month, int day)
    {
        std::tm timeStruct{};
        timeStruct.tm_year = year - 1900;
        timeStruct.tm_mon = month - 1;
        timeStruct.tm_mday = day;
        timeStruct.tm_hour = 12;
        timeStruct.tm_min = 0;
        timeStruct.tm_sec = 0;
        timeStruct.tm_isdst = -1;
        const std::time_t time = std::mktime(&timeStruct);
        return std::chrono::system_clock::from_time_t(time);
    }

    Order MakeOrder(const std::string& orderId, const std::string& sampleId, int quantity, OrderStatus status)
    {
        return Order(orderId, sampleId, "Customer-A", quantity, status, MakeLocalDateTimePoint(2026, 7, 15));
    }

    // ---- CountOrdersByStatus ----

    TEST(MonitoringServiceTest, REJECTED_주문은_상태별_집계에서_제외된다)
    {
        const std::vector<Order> orders{
            MakeOrder("ORD-1", "S001", 10, OrderStatus::Reserved),
            MakeOrder("ORD-2", "S001", 10, OrderStatus::Rejected),
        };

        const auto counts = Controller::MonitoringService::CountOrdersByStatus(orders);

        EXPECT_EQ(counts.count(OrderStatus::Rejected), 0u);
        EXPECT_EQ(counts.at(OrderStatus::Reserved), 1);
    }

    TEST(MonitoringServiceTest, 상태별_주문_건수가_정확히_집계된다)
    {
        const std::vector<Order> orders{
            MakeOrder("ORD-1", "S001", 10, OrderStatus::Reserved),
            MakeOrder("ORD-2", "S001", 10, OrderStatus::Producing),
            MakeOrder("ORD-3", "S001", 10, OrderStatus::Confirmed),
            MakeOrder("ORD-4", "S001", 10, OrderStatus::Confirmed),
            MakeOrder("ORD-5", "S001", 10, OrderStatus::Released),
        };

        const auto counts = Controller::MonitoringService::CountOrdersByStatus(orders);

        EXPECT_EQ(counts.at(OrderStatus::Reserved), 1);
        EXPECT_EQ(counts.at(OrderStatus::Producing), 1);
        EXPECT_EQ(counts.at(OrderStatus::Confirmed), 2);
        EXPECT_EQ(counts.at(OrderStatus::Released), 1);
    }

    // ---- BuildStockStatuses ----

    TEST(MonitoringServiceTest, 재고_상태가_여유_부족_고갈로_올바르게_판정된다)
    {
        const std::vector<Sample> samples{
            Sample("S001", "Sample-S001", 10.0, 0.9, 20), // 재고(20) >= 미출고 수요(10) -> 여유
            Sample("S002", "Sample-S002", 10.0, 0.9, 5),  // 재고(5) < 미출고 수요(10) -> 부족
            Sample("S003", "Sample-S003", 10.0, 0.9, 0),  // 재고(0) -> 고갈
        };
        const std::vector<Order> orders{
            MakeOrder("ORD-1", "S001", 10, OrderStatus::Confirmed),
            MakeOrder("ORD-2", "S002", 10, OrderStatus::Reserved),
        };

        const auto stockStatuses = Controller::MonitoringService::BuildStockStatuses(samples, orders);

        ASSERT_EQ(stockStatuses.size(), 3u);
        EXPECT_EQ(stockStatuses[0].level, Controller::StockLevel::Sufficient);
        EXPECT_EQ(stockStatuses[1].level, Controller::StockLevel::Low);
        EXPECT_EQ(stockStatuses[2].level, Controller::StockLevel::Depleted);
    }
}
