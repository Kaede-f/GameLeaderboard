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
        return { "Invalid", -1, -1 };

    RankInfo rankInfo;

    // 钻石及以下
    if (score < 175)
    {
        rankInfo.tier = tiers[score / 25];
        rankInfo.division = 5 - (score % 25) / 5;
        rankInfo.stars = score % 5;
    }
    // 大师与宗师
    else if (score < 235)
    {
        int offset = score - 175;

        rankInfo.tier = tiers[7 + offset / 30];
        rankInfo.division = 3 - (offset % 30) / 10;
        rankInfo.stars = offset % 10;
    }
    // 王者
    else
    {
        rankInfo.tier = tiers[9];
        rankInfo.division = 0;
        rankInfo.stars = score - 235;
    }

    return rankInfo;
}