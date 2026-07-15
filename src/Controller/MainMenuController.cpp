#include "MainMenuController.h"

namespace Controller
{
    MainMenuController::MainMenuController(
        SampleController& sampleController,
        Persistence::IOrderRepository& orderRepository,
        ProductionLineController& productionLineController)
        : sampleController_(sampleController)
        , orderRepository_(orderRepository)
        , productionLineController_(productionLineController)
    {
    }

    MainMenuSummary MainMenuController::GetSummary() const
    {
        const auto samples = sampleController_.GetAllSamples();

        MainMenuSummary summary;
        summary.registeredSampleCount = static_cast<int>(samples.size());
        for (const auto& sample : samples)
        {
            summary.totalStock += sample.GetStock();
        }
        summary.totalOrderCount = static_cast<int>(orderRepository_.GetAll().size());
        summary.productionQueuePendingItemCount = productionLineController_.GetPendingItemCount();
        return summary;
    }
}
