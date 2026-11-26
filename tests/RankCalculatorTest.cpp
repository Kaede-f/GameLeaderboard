#include <cstdlib>
#include <iostream>
#include <string>
#include "service/RankCalculator.h"

namespace
{
class TestRunner
{
public:
    void testScore(int currentScore, bool isWin, int expected)
    {
        ++casesRun_;
        const int actual = calculateNewScore(currentScore, isWin);
        if (actual != expected)
        {
            ++casesFailed_;
            std::cerr << "[FAIL] Case " << casesRun_
                      << ": calculateNewScore(currentScore=" << currentScore
                      << ", isWin=" << (isWin ? "true" : "false")
                      << ") expected=" << expected << ", actual=" << actual << '\n';
        }
    }

    void testRank(int score, const std::string& tier, int division, int stars)
    {
        ++casesRun_;
        const RankInfo actual = calculateRank(score);
        if (actual.tier != tier || actual.division != division || actual.stars != stars)
        {
            ++casesFailed_;
            std::cerr << "[FAIL] Case " << casesRun_
                      << ": calculateRank(score=" << score << ")\n"
                      << "  expected: tier=" << tier << ", division=" << division
                      << ", stars=" << stars << '\n'
                      << "  actual:   tier=" << actual.tier << ", division=" << actual.division
                      << ", stars=" << actual.stars << '\n';
        }
    }

    int report() const
    {
        if (casesFailed_ != 0)
        {
            std::cerr << casesFailed_ << " of " << casesRun_ << " test cases failed.\n";
            return EXIT_FAILURE;
        }

        std::cout << "All " << casesRun_ << " test cases passed.\n";
        return EXIT_SUCCESS;
    }

private:
    int casesRun_ = 0;
    int casesFailed_ = 0;
};
}

int main()
{
    TestRunner tests;
    tests.testScore(100, true, 101);
    tests.testScore(100, false, 99);
    tests.testScore(735, true, 735);
    tests.testScore(735, false, 734);
    tests.testScore(0, true, 1);
    tests.testScore(0, false, 0);
    tests.testScore(-1, true, -1);
    tests.testScore(736, false, -1);

    // 非法积分测试
    tests.testRank(-1, "Invalid", -1, -1);
    tests.testRank(736, "Invalid", -1, -1);

    // 钻石及以下段位测试
    tests.testRank(0, "黑铁", 5, 0);
    tests.testRank(4, "黑铁", 5, 4);
    tests.testRank(5, "黑铁", 4, 0);
    tests.testRank(24, "黑铁", 1, 4);
    tests.testRank(25, "青铜", 5, 0);
    tests.testRank(93, "黄金", 2, 3);
    tests.testRank(174, "钻石", 1, 4);

    // 大师段位测试
    tests.testRank(175, "大师", 3, 0);
    tests.testRank(184, "大师", 3, 9);
    tests.testRank(185, "大师", 2, 0);
    tests.testRank(204, "大师", 1, 9);

    // 宗师段位测试
    tests.testRank(205, "宗师", 3, 0);
    tests.testRank(234, "宗师", 1, 9);

    // 王者段位测试
    tests.testRank(235, "王者", 0, 0);
    tests.testRank(735, "王者", 0, 500);

    return tests.report();
}
