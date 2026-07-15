#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "../src/Controller/MainMenuController.h"
#include "../src/Controller/ProductionLineController.h"
#include "../src/Controller/SampleController.h"
#include "../src/Model/Order.h"
#include "../src/Model/OrderStatus.h"
#include "../src/Model/ProductionQueueItem.h"
#include "../src/Model/Sample.h"
#include "../src/Persistence/IOrderRepository.h"
#include "../src/Persistence/IProductionQueueRepository.h"
#include "../src/Persistence/ISampleRepository.h"

using Controller::MainMenuController;
using Controller::ProductionLineController;
using Controller::SampleController;
using Model::Order;
using Model::OrderStatus;
using Model::ProductionQueueItem;
using Model::Sample;

namespace
{
    // ISampleRepository의 인메모리 페이크.
    class InMemorySampleRepository : public Persistence::ISampleRepository
    {
    public:
        bool Create(const Sample& sample) override
        {
            if (FindById(sample.GetSampleId()).has_value())
            {
                return false;
            }
            samples_.push_back(sample);
            return true;
        }

        std::vector<Sample> GetAll() const override
        {
            return samples_;
        }

        std::optional<Sample> FindById(const std::string& sampleId) const override
        {
            const auto iterator = std::find_if(samples_.begin(), samples_.end(),
                [&sampleId](const Sample& sample) { return sample.GetSampleId() == sampleId; });
            if (iterator == samples_.end())
            {
                return std::nullopt;
            }
            return *iterator;
        }

        bool Update(const Sample& sample) override
        {
            for (auto& existing : samples_)
            {
                if (existing.GetSampleId() == sample.GetSampleId())
                {
                    existing = sample;
                    return true;
                }
            }
            return false;
        }

        bool Remove(const std::string& sampleId) override
        {
            const auto iterator = std::find_if(samples_.begin(), samples_.end(),
                [&sampleId](const Sample& sample) { return sample.GetSampleId() == sampleId; });
            if (iterator == samples_.end())
            {
                return false;
            }
            samples_.erase(iterator);
            return true;
        }

    private:
        std::vector<Sample> samples_;
    };

    // IOrderRepository의 인메모리 페이크.
    class InMemoryOrderRepository : public Persistence::IOrderRepository
    {
    public:
        bool Create(const Order& order) override
        {
            if (FindById(order.GetOrderId()).has_value())
            {
                return false;
            }
            orders_.push_back(order);
            return true;
        }

        std::vector<Order> GetAll() const override
        {
            return orders_;
        }

        std::optional<Order> FindById(const std::string& orderId) const override
        {
            const auto iterator = std::find_if(orders_.begin(), orders_.end(),
                [&orderId](const Order& order) { return order.GetOrderId() == orderId; });
            if (iterator == orders_.end())
            {
                return std::nullopt;
            }
            return *iterator;
        }

        bool Update(const Order& order) override
        {
            for (auto& existing : orders_)
            {
                if (existing.GetOrderId() == order.GetOrderId())
                {
                    existing = order;
                    return true;
                }
            }
            return false;
        }

        bool Remove(const std::string& orderId) override
        {
            const auto iterator = std::find_if(orders_.begin(), orders_.end(),
                [&orderId](const Order& order) { return order.GetOrderId() == orderId; });
            if (iterator == orders_.end())
            {
                return false;
            }
            orders_.erase(iterator);
            return true;
        }

    private:
        std::vector<Order> orders_;
    };

    // IProductionQueueRepository의 인메모리 페이크.
    class InMemoryProductionQueueRepository : public Persistence::IProductionQueueRepository
    {
    public:
        bool Create(const ProductionQueueItem& item) override
        {
            if (FindById(item.GetOrderId()).has_value())
            {
                return false;
            }
            items_.push_back(item);
            return true;
        }

        std::vector<ProductionQueueItem> GetAll() const override
        {
            return items_;
        }

        std::optional<ProductionQueueItem> FindById(const std::string& orderId) const override
        {
            const auto iterator = std::find_if(items_.begin(), items_.end(),
                [&orderId](const ProductionQueueItem& item) { return item.GetOrderId() == orderId; });
            if (iterator == items_.end())
            {
                return std::nullopt;
            }
            return *iterator;
        }

        bool Update(const ProductionQueueItem& item) override
        {
            for (auto& existing : items_)
            {
                if (existing.GetOrderId() == item.GetOrderId())
                {
                    existing = item;
                    return true;
                }
            }
            return false;
        }

        bool Remove(const std::string& orderId) override
        {
            const auto iterator = std::find_if(items_.begin(), items_.end(),
                [&orderId](const ProductionQueueItem& item) { return item.GetOrderId() == orderId; });
            if (iterator == items_.end())
            {
                return false;
            }
            items_.erase(iterator);
            return true;
        }

    private:
        std::vector<ProductionQueueItem> items_;
    };

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

    class MainMenuControllerTest : public ::testing::Test
    {
    protected:
        InMemorySampleRepository sampleRepository_;
        InMemoryOrderRepository orderRepository_;
        InMemoryProductionQueueRepository productionQueueRepository_;

        void SeedSample(const std::string& sampleId, int stock)
        {
            sampleRepository_.Create(Sample(sampleId, "Sample-" + sampleId, 10.0, 0.9, stock));
        }

        void SeedOrder(const std::string& orderId, const std::string& sampleId, int quantity, OrderStatus status)
        {
            orderRepository_.Create(Order(orderId, sampleId, "Customer-A", quantity, status,
                MakeLocalDateTimePoint(2026, 7, 15)));
        }

        void SeedProductionQueueItem(const std::string& orderId)
        {
            productionQueueRepository_.Create(ProductionQueueItem(
                orderId, 5, 6, ProductionQueueItem::MinutesDuration(60.0), std::chrono::system_clock::now()));
        }
    };

    TEST_F(MainMenuControllerTest, 등록된_시료_수와_총_재고가_정확히_집계된다)
    {
        SeedSample("S001", 20);
        SeedSample("S002", 30);
        SampleController sampleController(sampleRepository_);
        ProductionLineController productionLineController(
            productionQueueRepository_, orderRepository_, sampleController);
        MainMenuController mainMenuController(sampleController, orderRepository_, productionLineController);

        const auto summary = mainMenuController.GetSummary();

        EXPECT_EQ(summary.registeredSampleCount, 2);
        EXPECT_EQ(summary.totalStock, 50);
    }

    TEST_F(MainMenuControllerTest, 전체_주문_수는_상태와_무관하게_모두_집계된다)
    {
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Reserved);
        SeedOrder("ORD-2", "S001", 10, OrderStatus::Rejected);
        SeedOrder("ORD-3", "S001", 10, OrderStatus::Released);
        SampleController sampleController(sampleRepository_);
        ProductionLineController productionLineController(
            productionQueueRepository_, orderRepository_, sampleController);
        MainMenuController mainMenuController(sampleController, orderRepository_, productionLineController);

        const auto summary = mainMenuController.GetSummary();

        EXPECT_EQ(summary.totalOrderCount, 3);
    }

    TEST_F(MainMenuControllerTest, 생산라인_대기_건수가_생산큐_항목_수와_일치한다)
    {
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 10, OrderStatus::Producing);
        SeedProductionQueueItem("ORD-1");
        SeedProductionQueueItem("ORD-2");
        SampleController sampleController(sampleRepository_);
        ProductionLineController productionLineController(
            productionQueueRepository_, orderRepository_, sampleController);
        MainMenuController mainMenuController(sampleController, orderRepository_, productionLineController);

        const auto summary = mainMenuController.GetSummary();

        EXPECT_EQ(summary.productionQueuePendingItemCount, 2);
    }
}
