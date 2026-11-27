#pragma once

#include <cstdint>
#include <string>

// 解码成功时为合法业务字段；失败时三个字段均为 -1。
struct RankingFields {
    int score;
    int winRateCode;
    int elapsedSeconds;
};

// score: [0,735]，winRateCode: [0,1000]，elapsedSeconds: [0,31536000]。
// 时间为相对赛季开始的秒数；跨赛季尾部时间如何处理仍待业务确认。
// 任一字段非法返回 -1。合法结果为非负 45 位整数，可精确转换为 double。
// 位布局：[44..35] 积分，[34..25] 胜率，[24..0] 31536000-elapsedSeconds。
// 按编码降序比较即积分降序、胜率降序、时间升序；UID 不包含在编码中。
std::int64_t encodeRankingScore(
    int score,
    int winRateCode,
    int elapsedSeconds
);

// encoded 必须在 [0,2^45-1]，且提取出的业务字段也必须在上述合法范围。
// 非法整体编码或字段返回 {-1,-1,-1}；成功恢复原始相对秒数。
RankingFields decodeRankingScore(std::int64_t encoded);

// UID 使用 int，合法范围 [100000000,199999999]（固定九位，首位为 1）。
// 合法时直接返回十进制字符串，无须补零；非法时返回空字符串。
// 合法字符串的字典序与整数序一致，同编码时可按 UID 字符串降序兜底。
std::string encodeUid(int uid);
