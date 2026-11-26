#include <cstdlib>
#include <iostream>
#include <limits>
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

    void testRank(int score, Tier tier, int division, int stars)
    {
        ++casesRun_;
        const RankInfo actual = calculateRank(score);
        if (actual.tier != tier || actual.division != division || actual.stars != stars)
        {
            ++casesFailed_;
            std::cerr << "[FAIL] Case " << casesRun_
                      << ": calculateRank(score=" << score << ")\n"
                      << "  expected: tier=" << getTierName(tier)
                      << '(' << static_cast<int>(tier) << "), division=" << division
                      << ", stars=" << stars << '\n'
                      << "  actual:   tier=" << getTierName(actual.tier)
                      << '(' << static_cast<int>(actual.tier) << "), division=" << actual.division
                      << ", stars=" << actual.stars << '\n';
        }
    }

    void testWinRate(int wins, int totalGames, int expected)
    {
        checkEqual("calculateWinRate(wins=" + std::to_string(wins)
                       + ", totalGames=" + std::to_string(totalGames) + ')',
                   calculateWinRate(wins, totalGames), expected);
    }

    void testTierName(Tier tier, int expectedId, const std::string& expectedName)
    {
        ++casesRun_;
        const int actualId = static_cast<int>(tier);
        const std::string actualName = getTierName(tier);
        if (actualId != expectedId || actualName != expectedName)
        {
            ++casesFailed_;
            std::cerr << "[FAIL] Case " << casesRun_ << ": getTierName / Tier numbering\n"
                      << "  expected: id=" << expectedId << ", name=" << expectedName << '\n'
                      << "  actual:   id=" << actualId << ", name=" << actualName << '\n';
        }
    }

    void testInheritedScore(int previousScore, int seasonsPassed, int expected)
    {
        checkEqual("calculateInheritedScore(previousScore=" + std::to_string(previousScore)
                       + ", seasonsPassed=" + std::to_string(seasonsPassed) + ')',
                   calculateInheritedScore(previousScore, seasonsPassed), expected);
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
    void checkEqual(const std::string& context, int actual, int expected)
    {
        ++casesRun_;
        if (actual != expected)
        {
            ++casesFailed_;
            std::cerr << "[FAIL] Case " << casesRun_ << ": " << context
                      << " expected=" << expected << ", actual=" << actual << '\n';
        }
    }

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
    tests.testRank(-1, Tier::Invalid, -1, -1);
    tests.testRank(736, Tier::Invalid, -1, -1);

    // 钻石及以下段位测试
    tests.testRank(0, Tier::BlackIron, 5, 0);
    tests.testRank(4, Tier::BlackIron, 5, 4);
    tests.testRank(5, Tier::BlackIron, 4, 0);
    tests.testRank(24, Tier::BlackIron, 1, 4);
    tests.testRank(25, Tier::Bronze, 5, 0);
    tests.testRank(93, Tier::Gold, 2, 3);
    tests.testRank(174, Tier::Diamond, 1, 4);

    // 大师段位测试
    tests.testRank(175, Tier::Master, 3, 0);
    tests.testRank(184, Tier::Master, 3, 9);
    tests.testRank(185, Tier::Master, 2, 0);
    tests.testRank(204, Tier::Master, 1, 9);

    // 宗师段位测试
    tests.testRank(205, Tier::Grandmaster, 3, 0);
    tests.testRank(234, Tier::Grandmaster, 1, 9);

    // 王者段位测试
    tests.testRank(235, Tier::Challenger, 0, 0);
    tests.testRank(735, Tier::Challenger, 0, 500);

    constexpr int maxInt = std::numeric_limits<int>::max();
    constexpr int minInt = std::numeric_limits<int>::min();

    // 胜率编码：百分数保留一位小数后乘 10；恰好一半时向上舍入。
    struct WinRateCase { int wins; int totalGames; int expected; };
    const WinRateCase winRateCases[] = {
        {0, 10, 0}, {10, 10, 1000}, {1, 2, 500},
        {1, 3, 333}, {2, 3, 667}, {1, 8, 125}, {1, 16, 63},
        {1, 2000, 1}, {1, 2001, 0}, {1, 1999, 1},
        {1999, 2000, 1000}, {2, 7, 286},
        {0, 0, 0},
        {-1, 10, -1}, {1, -1, -1}, {-1, -1, -1},
        {11, 10, -1}, {1, 0, -1}, {minInt, maxInt, -1},
        {0, minInt, -1}, {maxInt, maxInt - 1, -1},
        {maxInt, maxInt, 1000}, {maxInt - 1, maxInt, 1000},
        {maxInt / 2, maxInt, 500}, {maxInt / 3, maxInt, 333},
        {1, maxInt, 0}, {0, maxInt, 0}
    };
    for (const auto& test : winRateCases)
        tests.testWinRate(test.wins, test.totalGames, test.expected);

    // 独立检查枚举编号与名称，防止段位比较依赖名称转换函数。
    struct TierNameCase { Tier tier; int id; const char* name; };
    const TierNameCase tierNameCases[] = {
        {Tier::BlackIron, 0, "黑铁"}, {Tier::Bronze, 1, "青铜"},
        {Tier::Silver, 2, "白银"}, {Tier::Gold, 3, "黄金"},
        {Tier::Platinum, 4, "铂金"}, {Tier::Emerald, 5, "翡翠"},
        {Tier::Diamond, 6, "钻石"}, {Tier::Master, 7, "大师"},
        {Tier::Grandmaster, 8, "宗师"}, {Tier::Challenger, 9, "王者"},
        {Tier::Invalid, -1, "Invalid"},
        {static_cast<Tier>(-2), -2, "Invalid"},
        {static_cast<Tier>(10), 10, "Invalid"},
        {static_cast<Tier>(minInt), minInt, "Invalid"},
        {static_cast<Tier>(maxInt), maxInt, "Invalid"}
    };
    for (const auto& test : tierNameCases)
        tests.testTierName(test.tier, test.id, test.name);

    // 所有段位区间两端的单次继承；目标均为目标段位最低积分。
    struct SingleSeasonCase { int score; int expected; };
    const SingleSeasonCase singleSeasonCases[] = {
        {0, 0}, {24, 0}, {25, 0}, {49, 0}, {50, 25}, {74, 25},
        {75, 50}, {99, 50}, {100, 75}, {124, 75},
        {125, 100}, {149, 100}, {150, 125}, {174, 125},
        {175, 150}, {204, 150}, {205, 150}, {234, 150},
        {235, 150}, {735, 150}
    };
    for (const auto& test : singleSeasonCases)
        tests.testInheritedScore(test.score, 1, test.expected);

    // 零次继承必须保留原积分，包括非段位最低积分。
    for (const auto& test : singleSeasonCases)
        tests.testInheritedScore(test.score, 0, test.score);

    struct InheritanceCase { int score; int seasons; int expected; };
    const InheritanceCase inheritanceCases[] = {
        // 多次继承、达到黑铁所需次数及继续继承。
        {735, 2, 125}, {735, 3, 100}, {735, 6, 25}, {735, 7, 0}, {735, 8, 0},
        {175, 2, 125}, {205, 2, 125}, {150, 2, 100}, {125, 2, 75},
        {100, 2, 50}, {75, 2, 25}, {50, 2, 0}, {25, 2, 0},
        {24, 2, 0}, {0, 100, 0}, {174, 6, 0},
        // 负次数，以及积分、次数同时非法。
        {0, -1, -1}, {735, -1, -1}, {175, -3, -1}, {100, minInt, -1},
        {-1, -1, -1}, {736, -1, -1},
        // 非法积分即使在零次继承时也应拒绝。
        {-1, 0, -1}, {-1, 1, -1}, {736, 0, -1}, {736, 1, -1},
        {minInt, 0, -1}, {minInt, maxInt, -1},
        {maxInt, 0, -1}, {maxInt, maxInt, -1},
        // 极大次数覆盖全部十个段位；应快速收敛至黑铁。
        {24, maxInt, 0}, {49, maxInt, 0}, {74, maxInt, 0}, {99, maxInt, 0},
        {124, maxInt, 0}, {149, maxInt, 0}, {174, maxInt, 0},
        {204, maxInt, 0}, {234, maxInt, 0}, {735, maxInt, 0}
    };
    for (const auto& test : inheritanceCases)
        tests.testInheritedScore(test.score, test.seasons, test.expected);

    return tests.report();
}
