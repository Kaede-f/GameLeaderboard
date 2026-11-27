#include "service/RankingEncoder.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

static_assert(std::is_same_v<decltype(&encodeUid), std::string (*)(int)>);

namespace
{
constexpr int maxSeconds = 31536000;
constexpr std::int64_t scoreUnit = 34359738368LL;
constexpr std::int64_t rateUnit = 33554432LL;
constexpr std::int64_t limit45 = 35184372088832LL;
constexpr unsigned randomSeed = 0x0032026u;
constexpr int sampleCount = 2048;

std::string describe(const RankingFields& fields)
{
    return "{score=" + std::to_string(fields.score)
        + ", winRateCode=" + std::to_string(fields.winRateCode)
        + ", elapsedSeconds=" + std::to_string(fields.elapsedSeconds) + '}';
}

bool sameFields(const RankingFields& left, const RankingFields& right)
{
    return left.score == right.score && left.winRateCode == right.winRateCode
        && left.elapsedSeconds == right.elapsedSeconds;
}

class TestRunner
{
public:
    void check(const std::string& category, const std::string& context,
               bool passed, const std::string& details = {})
    {
        ++casesRun_;
        ++categories_[category];
        if (!passed)
        {
            ++casesFailed_;
            if (casesFailed_ <= 20)
                std::cerr << "[FAIL] Case " << casesRun_ << " [" << category << "] "
                          << context << ' ' << details << '\n';
        }
    }

    template <typename T>
    void equal(const std::string& category, const std::string& context,
               const T& actual, const T& expected)
    {
        std::ostringstream details;
        if (actual != expected)
            details << "expected=" << expected << ", actual=" << actual;
        check(category, context, actual == expected, details.str());
    }

    void fields(const std::string& category, const std::string& context,
                const RankingFields& actual, const RankingFields& expected)
    {
        check(category, context, sameFields(actual, expected),
              "expected=" + describe(expected) + ", actual=" + describe(actual));
    }

    int report() const
    {
        for (const auto& [category, count] : categories_)
            std::cout << category << ": " << count << " cases\n";
        std::cout << "Random seed=" << randomSeed << ", ranking samples=" << sampleCount << '\n';
        if (casesFailed_ != 0)
        {
            if (casesFailed_ > 20)
                std::cerr << "Further failure details suppressed after the first 20.\n";
            std::cerr << casesFailed_ << " of " << casesRun_ << " test cases failed.\n";
            return EXIT_FAILURE;
        }
        std::cout << "All " << casesRun_ << " test cases passed.\n";
        return EXIT_SUCCESS;
    }

private:
    int casesRun_ = 0;
    int casesFailed_ = 0;
    std::map<std::string, int> categories_;
};

// Independent arithmetic oracle: intentionally uses multiplication rather than shifts/masks.
std::int64_t rawCode(int score, int rate, int reversedSeconds)
{
    return score * scoreUnit + rate * rateUnit + reversedSeconds;
}

void testValid(TestRunner& tests, const RankingFields& expected)
{
    const std::string context = describe(expected);
    const auto reference = rawCode(expected.score, expected.winRateCode,
                                   maxSeconds - expected.elapsedSeconds);
    const auto actual = encodeRankingScore(expected.score, expected.winRateCode,
                                           expected.elapsedSeconds);
    tests.equal("encode-valid", context, actual, reference);
    tests.fields("decode-valid", "encoded=" + std::to_string(reference),
                 decodeRankingScore(reference), expected);

    const auto decoded = decodeRankingScore(actual);
    const auto reencoded = encodeRankingScore(decoded.score, decoded.winRateCode,
                                              decoded.elapsedSeconds);
    tests.check("roundtrip", context,
                sameFields(decoded, expected) && reencoded == actual && actual == reference,
                "decoded=" + describe(decoded) + ", reencoded=" + std::to_string(reencoded)
                    + ", encoded=" + std::to_string(actual));

    // Check exact value recovery and distinguish adjacent integers, including near the maximum.
    const bool exactDouble = actual >= 0 && actual < limit45
        && static_cast<std::int64_t>(static_cast<double>(actual)) == actual
        && static_cast<double>(actual + 1) - static_cast<double>(actual) == 1.0;
    tests.check("double-precision", context, exactDouble,
                "encoded=" + std::to_string(actual));
}

void testInvalidInputs(TestRunner& tests)
{
    constexpr int minInt = std::numeric_limits<int>::min();
    constexpr int maxInt = std::numeric_limits<int>::max();
    const RankingFields invalidInputs[] = {
        {-1, 0, 0}, {736, 0, 0}, {minInt, 0, 0}, {maxInt, 0, 0},
        {0, -1, 0}, {0, 1001, 0}, {0, minInt, 0}, {0, maxInt, 0},
        {0, 0, -1}, {0, 0, 31536001}, {0, 0, minInt}, {0, 0, maxInt},
        {-1, -1, -1}, {maxInt, maxInt, maxInt}
    };
    for (const auto& fields : invalidInputs)
        tests.equal("encode-invalid", describe(fields),
                    encodeRankingScore(fields.score, fields.winRateCode, fields.elapsedSeconds),
                    std::int64_t{-1});

    const std::int64_t invalidCodes[] = {
        -1, std::numeric_limits<std::int64_t>::min(),
        limit45, limit45 + 1, limit45 * 2, std::numeric_limits<std::int64_t>::max(),
        limit45 - 1,
        rawCode(736, 0, 0), rawCode(1023, 0, 0),
        rawCode(0, 1001, 0), rawCode(0, 1023, 0),
        rawCode(0, 0, 31536001), rawCode(0, 0, 33554431),
        rawCode(735, 1000, 31536001), rawCode(736, 1001, 0)
    };
    for (const auto encoded : invalidCodes)
        tests.fields("decode-invalid", "encoded=" + std::to_string(encoded),
                     decodeRankingScore(encoded), {-1, -1, -1});
}

struct Row
{
    RankingFields fields;
    int uid;
    std::int64_t encoded;
    std::string uidText;
};

// Oracle compares business fields directly, without consulting any encoded value.
bool originalOrder(const Row& left, const Row& right)
{
    if (left.fields.score != right.fields.score)
        return left.fields.score > right.fields.score;
    if (left.fields.winRateCode != right.fields.winRateCode)
        return left.fields.winRateCode > right.fields.winRateCode;
    if (left.fields.elapsedSeconds != right.fields.elapsedSeconds)
        return left.fields.elapsedSeconds < right.fields.elapsedSeconds;
    return left.uid > right.uid;
}

void checkOrder(TestRunner& tests, const std::string& category,
                const std::vector<Row>& actual, const std::vector<Row>& expected)
{
    const auto mismatch = std::mismatch(actual.begin(), actual.end(), expected.begin(),
                                      [](const Row& a, const Row& b) { return a.uid == b.uid; });
    const bool passed = mismatch.first == actual.end();
    std::string details;
    if (!passed)
        details = "position=" + std::to_string(mismatch.first - actual.begin())
            + ", expected UID=" + std::to_string(mismatch.second->uid)
            + ", actual UID=" + std::to_string(mismatch.first->uid);
    tests.check(category, "2048 rows, score DESC / rate DESC / time ASC / UID DESC",
                passed, details);
}

void testPriorities(TestRunner& tests)
{
    struct Pair { RankingFields higher; RankingFields lower; };
    const Pair pairs[] = {
        {{1, 0, maxSeconds}, {0, 1000, 0}},
        {{735, 0, maxSeconds}, {734, 1000, 0}},
        {{100, 1, maxSeconds}, {100, 0, 0}},
        {{100, 1000, maxSeconds}, {100, 999, 0}},
        {{100, 500, 0}, {100, 500, 1}},
        {{100, 500, maxSeconds - 1}, {100, 500, maxSeconds}}
    };
    for (const auto& pair : pairs)
    {
        const auto high = encodeRankingScore(pair.higher.score, pair.higher.winRateCode,
                                             pair.higher.elapsedSeconds);
        const auto low = encodeRankingScore(pair.lower.score, pair.lower.winRateCode,
                                            pair.lower.elapsedSeconds);
        tests.check("priority", describe(pair.higher) + " > " + describe(pair.lower),
                    high >= 0 && low >= 0 && high > low
                        && static_cast<double>(high) > static_cast<double>(low),
                    "higher code=" + std::to_string(high) + ", lower code=" + std::to_string(low));
    }
}

void testRandomRanking(TestRunner& tests, std::mt19937& random)
{
    std::vector<Row> rows;
    for (int i = 0; i < sampleCount; ++i)
    {
        RankingFields fields{static_cast<int>(random() % 736),
                             static_cast<int>(random() % 1001),
                             static_cast<int>(random() % 31536001)};
        // Force complete score ties so the fourth-key UID ordering is exercised.
        if (i % 8 == 0)
            fields = {123, 456, 789};
        testValid(tests, fields);
        const int uid = 100000000 + i * 45001;
        rows.push_back({fields, uid,
                        encodeRankingScore(fields.score, fields.winRateCode, fields.elapsedSeconds),
                        encodeUid(uid)});
    }
    std::shuffle(rows.begin(), rows.end(), random);
    auto expected = rows;
    std::sort(expected.begin(), expected.end(), originalOrder);

    auto encodedOrder = rows;
    std::sort(encodedOrder.begin(), encodedOrder.end(), [](const Row& a, const Row& b) {
        if (a.encoded != b.encoded) return a.encoded > b.encoded;
        return a.uidText > b.uidText;
    });
    checkOrder(tests, "random-sort-integer", encodedOrder, expected);

    auto doubleOrder = rows;
    std::sort(doubleOrder.begin(), doubleOrder.end(), [](const Row& a, const Row& b) {
        const double left = static_cast<double>(a.encoded);
        const double right = static_cast<double>(b.encoded);
        if (left != right) return left > right;
        return a.uidText > b.uidText;
    });
    checkOrder(tests, "random-sort-double", doubleOrder, expected);
}

void testUid(TestRunner& tests, std::mt19937& random)
{
    struct UidCase { int uid; const char* expected; };
    const UidCase cases[] = {
        {100000000, "100000000"}, {100000001, "100000001"},
        {123456789, "123456789"}, {199999998, "199999998"}, {199999999, "199999999"},
        {-1, ""}, {0, ""}, {99999999, ""}, {200000000, ""}, {999999999, ""},
        {std::numeric_limits<int>::min(), ""}, {std::numeric_limits<int>::max(), ""}
    };
    for (const auto& test : cases)
        tests.equal("uid-boundary", "UID=" + std::to_string(test.uid),
                    encodeUid(test.uid), std::string(test.expected));

    std::vector<int> integerUids{100000000, 100000001, 199999998, 199999999};
    std::vector<std::string> textUids;
    for (int i = 0; i < 1024; ++i)
    {
        const int uid = 100000000 + static_cast<int>(random() % 100000000);
        const auto actual = encodeUid(uid);
        tests.check("uid-random", "UID=" + std::to_string(uid),
                    actual == std::to_string(uid) && actual.size() == 9 && actual.front() == '1',
                    "expected=" + std::to_string(uid) + ", actual=" + actual);
        integerUids.push_back(uid);
    }
    for (const int uid : integerUids)
        textUids.push_back(encodeUid(uid));
    std::sort(integerUids.begin(), integerUids.end());
    std::sort(textUids.begin(), textUids.end());
    bool ascending = true;
    for (std::size_t i = 0; i < integerUids.size(); ++i)
        ascending = ascending && textUids[i] == std::to_string(integerUids[i]);
    tests.check("uid-sort", "1028 UIDs ascending", ascending);

    std::sort(integerUids.begin(), integerUids.end(), [](int a, int b) { return a > b; });
    std::sort(textUids.begin(), textUids.end(), [](const auto& a, const auto& b) { return a > b; });
    bool descending = true;
    for (std::size_t i = 0; i < integerUids.size(); ++i)
        descending = descending && textUids[i] == std::to_string(integerUids[i]);
    tests.check("uid-sort", "1028 UIDs descending", descending);
}
}

int main()
{
    TestRunner tests;
    for (const int score : {0, 1, 735})
        for (const int rate : {0, 1, 1000})
            for (const int seconds : {0, 1, maxSeconds})
                testValid(tests, {score, rate, seconds});
    for (const RankingFields fields : {RankingFields{100, 667, 123456},
                                      RankingFields{735, 999, 31535999},
                                      RankingFields{93, 286, 86400}})
        testValid(tests, fields);

    testInvalidInputs(tests);
    testPriorities(tests);
    std::mt19937 random(randomSeed);
    testRandomRanking(tests, random);
    testUid(tests, random);
    return tests.report();
}
