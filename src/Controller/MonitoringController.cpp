#include "MonitoringController.h"

namespace Controller
{
    MonitoringController::MonitoringController(
        Persistence::IOrderRepository& orderRepository,
        SampleController& sampleController)
        : orderRepository_(orderRepository)
        , sampleController_(sampleController)
    {
    }

    std::map<Model::OrderStatus, int> MonitoringController::GetOrderCountsByStatus() const
    {
        return MonitoringService::CountOrdersByStatus(orderRepository_.GetAll());
    }

    std::vector<StockStatus> MonitoringController::GetStockStatuses() const
    {
        return MonitoringService::BuildStockStatuses(sampleController_.GetAllSamples(), orderRepository_.GetAll());
    }
}
