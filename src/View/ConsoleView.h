#pragma once

#include <string>

#include "../Model/Order.h"

namespace View
{
    // 콘솔 입출력을 담당하는 공용 View 헬퍼.
    // Controller/Model은 화면 형식을 알지 못하며, 모든 출력 형식과 입력 파싱은 이 클래스가 담당한다
    // (CLAUDE.md 아키텍처 절: "콘솔 I/O는 View에만 존재").
    class ConsoleView
    {
    public:
        static void PrintTitle(const std::string& title);
        static void PrintLine(const std::string& text = "");
        static void PrintDivider();
        static void PrintError(const std::string& message);

        static std::string ReadLine(const std::string& prompt);
        static int ReadInt(const std::string& prompt);
        static double ReadDouble(const std::string& prompt);

        // 주문 목록 표시 화면(OrderView/OrderApprovalView)에서 공통으로 쓰는 표 형식 출력.
        static void PrintOrderTableHeader();
        static void PrintOrderRow(const Model::Order& order);
    };
}
