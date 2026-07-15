#include "ProductionCalculator.h"

#include <cmath>

namespace Controller
{
    namespace ProductionCalculator
    {
        int CalculateActualProductionQuantity(int shortageQuantity, double yield)
        {
            return static_cast<int>(std::ceil(static_cast<double>(shortageQuantity) / yield));
        }

        Model::ProductionQueueItem::MinutesDuration CalculateTotalProductionTime(
            double averageProductionMinutesPerUnit, int actualProductionQuantity)
        {
            return Model::ProductionQueueItem::MinutesDuration(
                averageProductionMinutesPerUnit * actualProductionQuantity);
        }
    }
}
