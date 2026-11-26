#pragma once
#include <string>

// 段位编号契约：Invalid=-1，十个合法段位按强弱顺序编号为 0..9。
enum class Tier {
    Invalid = -1,
    BlackIron = 0,
    Bronze = 1,
    Silver = 2,
    Gold = 3,
    Platinum = 4,
    Emerald = 5,
    Diamond = 6,
    Master = 7,
    Grandmaster = 8,
    Challenger = 9
};

// 非法积分对应 {Tier::Invalid, -1, -1}。
// 合法时 division 为黑铁至钻石的 5..1、大师/宗师的 3..1、王者的 0；
// stars 为当前小段星数，王者为超过 235 分的星数（0..500）。
struct RankInfo {
    Tier tier;
    int division;
    int stars;
};

// currentScore 必须在 [0,735]；胜利 +1、失败 -1，结果限制在 [0,735]。
// 非法积分返回 -1。
int calculateNewScore(int currentScore, bool isWin);

// 将 [0,735] 积分转换为段位、小段和星数；非法积分返回上述无效 RankInfo。
RankInfo calculateRank(int score);

// 返回 round(wins / totalGames * 1000) 的整数编码，范围 [0,1000]，
// 对应 0.0%..100.0%，恰好一半时向上舍入；(0,0) 返回 0。
// wins<0、totalGames<0 或 wins>totalGames 返回 -1（包括 wins>0 且零场次）。
// 接受整个 int 输入范围，合法大整数运算不得因中间乘法溢出。
int calculateWinRate(int wins, int totalGames);

// 返回十个合法段位的中文名称；Tier::Invalid 及其他非法编号返回 "Invalid"。
std::string getTierName(Tier tier);

// previousScore 必须在 [0,735]，seasonsPassed 必须非负，否则返回 -1。
// 零次继承返回原积分；正次数逐季继承：黑铁/青铜到黑铁，白银至钻石降一档，
// 大师/宗师/王者到钻石；每次取目标段位最低积分，最低保持黑铁 0 分。
// 接受至 INT_MAX 次继承，收敛到黑铁后结果保持 0，无须逐次遍历剩余赛季。
int calculateInheritedScore(int previousScore, int seasonsPassed);
