#include "ConsoleView.h"

#include <iomanip>
#include <iostream>

#include "../Model/OrderStatus.h"

namespace View
{
    void ConsoleView::PrintTitle(const std::string& title)
    {
        PrintDivider();
        std::cout << title << "\n";
        PrintDivider();
    }

    void ConsoleView::PrintLine(const std::string& text)
    {
        std::cout << text << "\n";
    }

    void ConsoleView::PrintDivider()
    {
        std::cout << "==================================================\n";
    }

    void ConsoleView::PrintError(const std::string& message)
    {
        std::cout << "[오류] " << message << "\n";
    }

    std::string ConsoleView::ReadLine(const std::string& prompt)
    {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    int ConsoleView::ReadInt(const std::string& prompt)
    {
        while (true)
        {
            std::cout << prompt;
            std::string line;
            std::getline(std::cin, line);
            try
            {
                size_t consumedCharacterCount = 0;
                int value = std::stoi(line, &consumedCharacterCount);
                if (consumedCharacterCount == line.size())
                {
                    return value;
                }
            }
            catch (...)
            {
                // 아래에서 오류 메시지를 출력하고 재입력 받는다.
            }
            PrintError("숫자를 입력해주세요.");
        }
    }

    double ConsoleView::ReadDouble(const std::string& prompt)
    {
        while (true)
        {
            std::cout << prompt;
            std::string line;
            std::getline(std::cin, line);
            try
            {
                size_t consumedCharacterCount = 0;
                double value = std::stod(line, &consumedCharacterCount);
                if (consumedCharacterCount == line.size())
                {
                    return value;
                }
            }
            catch (...)
            {
                // 아래에서 오류 메시지를 출력하고 재입력 받는다.
            }
            PrintError("숫자를 입력해주세요.");
        }
    }

    void ConsoleView::PrintOrderTableHeader()
    {
        std::cout << std::left << std::setw(20) << "주문번호" << std::setw(10) << "시료ID"
            << std::setw(16) << "고객명" << std::setw(8) << "수량" << "상태\n";
    }

    void ConsoleView::PrintOrderRow(const Model::Order& order)
    {
        std::cout << std::left << std::setw(20) << order.GetOrderId() << std::setw(10) << order.GetSampleId()
            << std::setw(16) << order.GetCustomerName() << std::setw(8) << order.GetQuantity()
            << Model::OrderStatusToString(order.GetStatus()) << "\n";
    }
}
