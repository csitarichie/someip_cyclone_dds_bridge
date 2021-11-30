#pragma once

// RSZRSZ I don't really like the idea of detached threads
// this could cause weird errors. but I don't have any better idea for now.
#include <future>

#define TEST_TIMEOUT_BEGIN                                           \
    std::promise<bool> promisedFinished;                             \
    auto               futureResult = promisedFinished.get_future(); \
    std::thread([&](std::promise<bool>& finished) {
#define TEST_TIMEOUT_FAIL_END(X)             \
    finished.set_value(true);                \
    }, std::ref(promisedFinished)).detach(); \
    EXPECT_TRUE(futureResult.wait_for(std::chrono::milliseconds(X)) != std::future_status::timeout);

#define TEST_TIMEOUT_SUCCESS_END(X)          \
    finished.set_value(true);                \
    }, std::ref(promisedFinished)).detach(); \
    EXPECT_FALSE(futureResult.wait_for(std::chrono::milliseconds(X)) != std::future_status::timeout);

// usage
// TEST(SomeUint, LongCalculationTimeout)
//{
//    TEST_TIMEOUT_BEGIN
//    EXPECT_EQ(10, longCalculationFunction());
//    TEST_TIMEOUT_FAIL_END(1000)
//}
