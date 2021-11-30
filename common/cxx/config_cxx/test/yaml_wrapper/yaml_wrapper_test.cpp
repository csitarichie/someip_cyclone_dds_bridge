#include <cstdlib>
#include <iostream>
#include "gtest/gtest.h"

#include "adstutil_cxx/error_handler.hpp"
#include "adstutil_cxx/gtest_util.hpp"
#include "config_cxx/yaml_wrapper.hpp"

#define EXPECT_PAIREQ(a, b)          \
    EXPECT_EQ_NOLINT((a).second, b); \
    EXPECT_TRUE((a).first)

#define EXPECT_STRPAIREQ(a, b)           \
    EXPECT_STREQ((a).second.c_str(), b); \
    EXPECT_TRUE((a).first)

#define EXPECT_STRPAIREQ_NOT_FOUND(a, b) \
    EXPECT_STREQ((a).second.c_str(), b); \
    EXPECT_FALSE((a).first)

#define EXPECT_NOT_FOUND(a) EXPECT_FALSE((a).first)

using Error       = adst::common::Error;
using YamlWrapper = adst::common::YamlWrapper;

TEST(YamlWrapper, BasicTest) // NOLINT
{
    YamlWrapper YW(
        [](const Error& error) {
            std::cout << error.errorCodeName_.second << std::endl;
            GTEST_FAIL();
        },
        "", "test_message.yml");

    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/s", '/'), "foo");
    EXPECT_PAIREQ(YW.getValue<int>("/test_pb2.TestMessage/i", '/'), -7);
    EXPECT_PAIREQ(YW.getValue<bool>("/test_pb2.TestMessage/b", '/'), true);
    EXPECT_PAIREQ(YW.getValue<unsigned int>("/test_pb2.TestMessage/u", '/'), 88);
    EXPECT_PAIREQ(YW.getValue<float>("/test_pb2.TestMessage/fl", '/'), 3.1415927410125732);
    EXPECT_PAIREQ(YW.getValue<double>("/test_pb2.TestMessage/fl", '/'), 3.1415927410125732); // same, as type 'double'
    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/rep_str/0", '/'), "asd");
    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/rep_str/1", '/'), "qwe");
    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/rep_str/2", '/'), "yxc");
    EXPECT_PAIREQ(YW.getValue<int>("/test_pb2.TestMessage/rep_int/0", '/'), 1);
    EXPECT_PAIREQ(YW.getValue<int>("/test_pb2.TestMessage/rep_int/1", '/'), 2);
    EXPECT_PAIREQ(YW.getValue<int>("/test_pb2.TestMessage/rep_int/2", '/'), 3);
    EXPECT_PAIREQ(YW.getValue<bool>("/test_pb2.TestMessage/rep_bool/0", '/'), true);
    EXPECT_PAIREQ(YW.getValue<bool>("/test_pb2.TestMessage/rep_bool/1", '/'), true);
    EXPECT_PAIREQ(YW.getValue<bool>("/test_pb2.TestMessage/rep_bool/2", '/'), true);
    EXPECT_PAIREQ(YW.getValue<bool>("/test_pb2.TestMessage/rep_bool/3", '/'), false);
    EXPECT_PAIREQ(YW.getValue<bool>("/test_pb2.TestMessage/rep_bool/4", '/'), false);
    EXPECT_PAIREQ(YW.getValue<bool>("/test_pb2.TestMessage/rep_bool/5", '/'), false);
    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/external_msg/s", '/'), "a");
    EXPECT_PAIREQ(YW.getValue<int>("/test_pb2.TestMessage/external_msg/i", '/'), -12);
    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/nested_msg/s", '/'), "aaa");
    EXPECT_PAIREQ(YW.getValue<int>("/test_pb2.TestMessage/nested_msg/i", '/'), -500);

    YamlWrapper seqWrapper(
        [](const Error& error) {
            std::cout << error.errorCodeName_.second << std::endl;
            GTEST_FAIL();
        },
        "", "sequence.yml");
    EXPECT_STRPAIREQ(seqWrapper.getValue<std::string>("/0", '/'), "foo");
    EXPECT_PAIREQ(seqWrapper.getValue<int>("/3", '/'), 88);
    EXPECT_STRPAIREQ(seqWrapper.getValue<std::string>("/5/0", '/'), "asd");
    EXPECT_STRPAIREQ(seqWrapper.getValue<std::string>("/6/key", '/'), "str");
}

TEST(YamlWrapper, TypeConversions) // NOLINT
{
    YamlWrapper YW(
        [](const Error& error) {
            std::cout << error.errorCodeName_.second << std::endl;
            GTEST_FAIL();
        },
        "", "test_message.yml");

    // n bit number (signed/unsigned) conversions
    EXPECT_PAIREQ(YW.getValue<unsigned int>("/test_pb2.TestMessage/i", '/'), -7); // you can retrieve int as unsigned
                                                                                  // int
    EXPECT_PAIREQ(YW.getValue<unsigned int>("/test_pb2.TestMessage/i", '/'),
                  0xFFFFFFFF - 6); // same line as above but in uint
    EXPECT_PAIREQ(YW.getValue<unsigned char>("/test_pb2.TestMessage/nested_msg/i", '/'),
                  -500 + 2 * 256); // the value is -500 originally on 8 bit it's 0xC (12)
    EXPECT_PAIREQ(YW.getValue<char>("/test_pb2.TestMessage/nested_msg/i", '/'),
                  -500 + 2 * 256); // the value is -500 originally on 8 bit it's 0xC (12)
    EXPECT_PAIREQ(YW.getValue<short>("/test_pb2.TestMessage/nested_msg/i", '/'),
                  -500); // the value is -500 originally on 8 bit it's 0xC (12)
}

TEST(YamlWrapper, Invalid) // NOLINT
{
    YamlWrapper YW(
        [](const Error& error) {
            std::cout << error.errorCodeName_.second << std::endl;
            GTEST_FAIL();
        },
        "", "test_message.yml");

    // values that are non-existant
    EXPECT_STRPAIREQ_NOT_FOUND(YW.getValue<std::string>("/noSuchNode", '/'), "Not Found");
    EXPECT_NOT_FOUND(YW.getValue<int>("/noSuchNode", '/'));
    EXPECT_NOT_FOUND(YW.getValue<bool>("/noSuchNode", '/'));
    EXPECT_NOT_FOUND(YW.getValue<float>("/noSuchNode", '/'));
    EXPECT_NOT_FOUND(YW.getValue<float>("", '/'));
    EXPECT_NOT_FOUND(YW.getValue<float>("/", '/'));
    EXPECT_STRPAIREQ_NOT_FOUND(YW.getValue<std::string>("/test_pb2.TestMessage/external_msg/s/a"), "Not Found");

    // good value, bad type
    EXPECT_NOT_FOUND(YW.getValue<bool>("/test_pb2.TestMessage/fl", '/')); // non-zero float as bool --> not found
    EXPECT_NOT_FOUND(YW.getValue<int>("/test_pb2.TestMessage/fl", '/'));  // non-zero float as int --> not found
    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/fl", '/'),
                     "3.1415927410125732");                              // non-zero float --> float as string
    EXPECT_NOT_FOUND(YW.getValue<bool>("/test_pb2.TestMessage/i", '/')); // non-zero int as bool --> not found
    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/i", '/'), "-7"); // non-zero int --> int as string
    EXPECT_NOT_FOUND(YW.getValue<int>("/test_pb2.TestMessage/b", '/'));               // bool as int --> not found
    EXPECT_NOT_FOUND(YW.getValue<int>("/test_pb2.TestMessage/b", '/'));               // non-zero int --> int as string

    // interim nodes
    EXPECT_NOT_FOUND(YW.getValue<int>("/test_pb2.TestMessage/rep_int", '/'));
    EXPECT_STRPAIREQ(YW.getValue<std::string>("/test_pb2.TestMessage/external_msg"), "YamlMappingValue");
}

TEST(YamlWrapper, DeathTest) // NOLINT
{
    EXPECT_DEATH(YamlWrapper YW( // NOLINT
                     [](const Error& error) {
                         std::cout << error.errorCodeName_.second << std::endl;
                         std::cerr << error.errorCodeName_.second;
                         exit(1);
                     },
                     "", "badPath.ext");
                 , "file read Failed:badPath.ext");
}
