#include "MonitoringService.h"

namespace Controller
{
    namespace
    {
        bool IsPendingDemandStatus(Model::OrderStatus status)
        {
            return status == Model::OrderStatus::Reserved
                || status == Model::OrderStatus::Producing
                || status == Model::OrderStatus::Confirmed;
        }

        int CalculatePendingDemandForSample(const std::string& sampleId, const std::vector<Model::Order>& orders)
        {
            int pendingDemand = 0;
            for (const auto& order : orders)
            {
                if (order.GetSampleId() == sampleId && IsPendingDemandStatus(order.GetStatus()))
                {
                    pendingDemand += order.GetQuantity();
                }
            }
            return pendingDemand;
        }

        StockLevel DetermineStockLevel(int stock, int pendingDemand)
        {
            if (stock <= 0)
            {
                return StockLevel::Depleted;
            }
            if (stock < pendingDemand)
            {
                return StockLevel::Low;
            }
            return StockLevel::Sufficient;
        }
    }

    namespace MonitoringService
    {
        std::map<Model::OrderStatus, int> CountOrdersByStatus(const std::vector<Model::Order>& orders)
        {
            std::map<Model::OrderStatus, int> counts;
            counts[Model::OrderStatus::Reserved] = 0;
            counts[Model::OrderStatus::Producing] = 0;
            counts[Model::OrderStatus::Confirmed] = 0;
            counts[Model::OrderStatus::Released] = 0;

            for (const auto& order : orders)
            {
                if (order.GetStatus() == Model::OrderStatus::Rejected)
                {
                    continue; // REJECTED는 모니터링 집계에서 제외한다(docs/PRD.md 4.5.1).
                }
                counts[order.GetStatus()]++;
            }
            return counts;
        }

        std::vector<StockStatus> BuildStockStatuses(
            const std::vector<Model::Sample>& samples, const std::vector<Model::Order>& orders)
        {
            std::vector<StockStatus> stockStatuses;
            for (const auto& sample : samples)
            {
                const int pendingDemand = CalculatePendingDemandForSample(sample.GetSampleId(), orders);
                const StockLevel level = DetermineStockLevel(sample.GetStock(), pendingDemand);
                stockStatuses.push_back(StockStatus{ sample, pendingDemand, level });
            }
            return stockStatuses;
        }
    }
}
