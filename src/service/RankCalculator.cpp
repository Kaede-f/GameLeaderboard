#include "RankCalculator.h"
#include <vector>
#include <algorithm>

constexpr int MIN_SCORE = 0;
constexpr int MAX_SCORE = 735;
const std::vector<std::string> tiers = {"黑铁", "青铜", "白银", "黄金", "铂金", "翡翠", "钻石", "大师", "宗师", "王者"};

int calculateNewScore(int currentScore, bool isWin)
{
	if (currentScore < MIN_SCORE || currentScore > MAX_SCORE) return -1;
	
	int newScore = currentScore + (isWin ? 1 : -1);

	return std::clamp(newScore, MIN_SCORE, MAX_SCORE);
}

RankInfo calculateRank(int score)
{
    // 非法积分
    if (score < MIN_SCORE || score > MAX_SCORE)
        return { Tier::Invalid, -1, -1 };

    RankInfo rankInfo;

    // 钻石及以下
    if (score < 175)
    {
        rankInfo.tier = static_cast<Tier>(score / 25);
        rankInfo.division = 5 - (score % 25) / 5;
        rankInfo.stars = score % 5;
    }
    // 大师与宗师
    else if (score < 235)
    {
        int offset = score - 175;

        rankInfo.tier = static_cast<Tier>(7 + offset / 30);
        rankInfo.division = 3 - (offset % 30) / 10;
        rankInfo.stars = offset % 10;
    }
    // 王者
    else
    {
        rankInfo.tier = Tier::Challenger;
        rankInfo.division = 0;
        rankInfo.stars = score - 235;
    }

    return rankInfo;
}

std::string getTierName(Tier tier)
{
    int id = static_cast<int>(tier);

    if (id < 0 || id >= static_cast<int>(tiers.size())) return "Invalid";

    return tiers[id];
}

int calculateWinRate(int wins, int totalGames)
{
    if (wins < 0 || totalGames < 0 || wins > totalGames) return -1;

    if (totalGames == 0) return 0;

    return static_cast<int>((wins * 1000ll + totalGames / 2) / totalGames);
}

int calculateInheritedScore(int previousScore, int seasonsPassed)
{
    Tier tier = calculateRank(previousScore).tier;
    if (tier == Tier::Invalid) return -1;

    if (seasonsPassed < 0) return -1;

    if (seasonsPassed == 0) return previousScore;

    int tierId = static_cast<int>(tier);

    for (int i = 0; i < seasonsPassed && tierId > 0; i++)
    {
        if (tierId >= 7) tierId = 6;
        else
        {
            tierId = std::max(0, tierId - 1);
        }
    }

    int inheritedScore = tierId * 25;

    return inheritedScore;
}