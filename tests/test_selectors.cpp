#include <gtest/gtest.h>
#include <lb/tcp/selectors.hpp>
#include <yaml-cpp/yaml.h>
#include <boost/asio.hpp>

TEST(RoundRobin, construction)
{
    YAML::Node selector_config = YAML::Load(
R"(algorithm: round_robin
endpoints:
    - url: http://www.google.com
    - ip: 127.0.0.1
      port: 8080
    - ip: 127.0.0.1
      port: 8081
    - ip: 127.0.0.1
      port: 8082
    - ip: 127.0.0.1
      port: 8083
)");

    lb::tcp::RoundRobinSelector selector;
    ASSERT_EQ(selector.Type(), lb::tcp::SelectorType::ROUND_ROBIN);
    selector.Configure(selector_config);
    boost::asio::ip::tcp::endpoint notused(boost::asio::ip::address::from_string("127.0.0.1"), 8080);

    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }
}

TEST(RoundRobin, basicUsage)
{
    YAML::Node selector_config = YAML::Load(
R"(algorithm: round_robin
endpoints:
    - url: http://www.google.com
    - ip: 127.0.0.1
      port: 8080
    - ip: 127.0.0.1
      port: 8081
    - ip: 127.0.0.1
      port: 8082
    - ip: 127.0.0.1
      port: 8083
)");

    lb::tcp::RoundRobinSelector selector;
    selector.Configure(selector_config);
    boost::asio::ip::tcp::endpoint notused(boost::asio::ip::address::from_string("127.0.0.1"), 8080);

    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }

    selector.ExcludeBackend(lb::tcp::Backend("127.0.0.1", 8080));
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }

    selector.ExcludeBackend(lb::tcp::Backend("127.0.0.1", 8081));
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }


    selector.ExcludeBackend(lb::tcp::Backend("127.0.0.1", 8082));
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }


    selector.ExcludeBackend(lb::tcp::Backend("127.0.0.1", 8083));
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }

    ASSERT_THROW(selector.ExcludeBackend(lb::tcp::Backend("http://www.google.com")), std::runtime_error);
}