#include "gtest/gtest.h"
#include "message-tests.hpp"
#include "single-client-messaging-tests.hpp"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}