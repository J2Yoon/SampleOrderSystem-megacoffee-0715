#ifdef RUN_TESTS

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#else

#include <iostream>

int main()
{
    std::cout << "SampleOrderSystem 초기화 완료. 개발이 이 지점부터 시작됩니다." << std::endl;
    return 0;
}

#endif
