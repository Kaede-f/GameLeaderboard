#include "RankingEncoder.h"

constexpr int MAX_SECONDS = 31536000;
constexpr int MAX_SCORE = 735;
constexpr int MAX_WIN_RATE_CODE = 1000;

constexpr int SCORE_SHIFT = 35;
constexpr int WIN_RATE_SHIFT = 25;

constexpr int64_t MAX_ENCODE_RANKING_SCORE = (1ll << 45) - 1;
constexpr uint64_t MASK_WIN_RATE = (1ll << 10) - 1;
constexpr uint64_t MASK_ELAPSED_SECONDS = (1ll << 25) - 1;

std::int64_t encodeRankingScore(
    int score,
    int winRateCode,
    int elapsedSeconds
)
{
    // 非法检查
    if (score < 0 || score > MAX_SCORE) return -1;
    if (winRateCode < 0 || winRateCode > MAX_WIN_RATE_CODE) return -1;
    if (elapsedSeconds < 0 || elapsedSeconds > MAX_SECONDS) return -1;

    int encodedSeconds = MAX_SECONDS - elapsedSeconds;

    return static_cast<uint64_t>(score) << SCORE_SHIFT |
        static_cast<uint64_t>(winRateCode) << WIN_RATE_SHIFT |
        encodedSeconds;
}

RankingFields decodeRankingScore(std::int64_t encoded)
{
    // 1. 检查整体编码是否合法
    if (encoded < 0 || encoded > MAX_ENCODE_RANKING_SCORE)
        return { -1, -1, -1 };

    // 2. 转换为无符号整数，进行位运算
    std::uint64_t value = static_cast<std::uint64_t>(encoded);

    // 3. 提取各个字段
    int score = static_cast<int>(value >> SCORE_SHIFT);

    int winRateCode = static_cast<int>(
        (value >> WIN_RATE_SHIFT) & MASK_WIN_RATE
        );

    int timeCode = static_cast<int>(
        value & MASK_ELAPSED_SECONDS
        );

    // 4. 检查业务数据范围
    if (score > MAX_SCORE ||
        winRateCode > MAX_WIN_RATE_CODE ||
        timeCode > MAX_SECONDS)
    {
        return { -1, -1, -1 };
    }

    // 5. 恢复原始相对时间
    int elapsedSeconds = MAX_SECONDS - timeCode;

    return { score, winRateCode, elapsedSeconds };
}

std::string encodeUid(int uid)
{
    if (uid < 100000000 || uid > 199999999) return "";
        
    return std::to_string(uid);
}
