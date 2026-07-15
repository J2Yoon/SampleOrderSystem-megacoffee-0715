#include "ConsoleView.h"

#include <iostream>

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
}
