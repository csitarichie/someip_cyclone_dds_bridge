#pragma once

// tidy doesn't like basic EXPECT_EQ:
// do not call c-style vararg functions [cppcoreguidelines-pro-type-vararg,-warnings-as-errors]
#define EXPECT_EQ_NOLINT(a, b) EXPECT_EQ(a, b) // NOLINT
