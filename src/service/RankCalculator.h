#pragma once
#include <string>

struct RankInfo {
    std::string tier;
    int division;
    int stars;
};

int calculateNewScore(int currentScore, bool isWin); // 非法输入返回 -1
RankInfo calculateRank(int score);