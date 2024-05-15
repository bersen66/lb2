#include <gtest/gtest.h>
#include <lb/tcp/selectors.hpp>
#include <yaml-cpp/yaml.h>
#include <boost/asio.hpp>
#include <lb/logging.hpp>

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


TEST(WeightedRoundRobin, basicUsage)
{
    YAML::Node selector_config = YAML::Load(
R"yaml(algorithm: weighted_round_robin
endpoints:
    - url: http://www.google.com
      weight: 1
    - ip: 127.0.0.1
      port: 8080
      weight: 2
    - ip: 127.0.0.1
      port: 8081
      weight: 3
    - ip: 127.0.0.1
      port: 8082
      weight: 4
    - ip: 127.0.0.1
      port: 8083
      weight: 5
)yaml");

    lb::tcp::WeightedRoundRobinSelector selector;
    selector.Configure(selector_config);
    boost::asio::ip::tcp::endpoint notused(boost::asio::ip::address::from_string("127.0.0.1"), 8080);

    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }
}

TEST(WeightedRoundRobin, withExcluding)
{
    YAML::Node selector_config = YAML::Load(
R"(algorithm: weighted_round_robin
endpoints:
    - url: http://www.google.com
      weight: 1
    - ip: 127.0.0.1
      port: 8080
      weight: 2
    - ip: 127.0.0.1
      port: 8081
      weight: 3
    - ip: 127.0.0.1
      port: 8082
      weight: 4
    - ip: 127.0.0.1
      port: 8083
      weight: 5
)");

    lb::tcp::WeightedRoundRobinSelector selector;
    selector.Configure(selector_config);
    boost::asio::ip::tcp::endpoint notused(boost::asio::ip::address::from_string("127.0.0.1"), 8080);

    DEBUG("All backends works");
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));  // 5

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8082));  // 4

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));  // 3

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));  // 2

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }

    selector.ExcludeBackend(lb::tcp::Backend("127.0.0.1", 8082));
    DEBUG("Excluded backend: 127.0.0.1:8082");
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));  // 5

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8081));  // 3

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));  // 2

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }

    selector.ExcludeBackend(lb::tcp::Backend("127.0.0.1", 8081));
    DEBUG("Excluded backend: 127.0.0.1:8081");
    for (int i = 0; i < 100; i++) {

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));  // 5

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8080));  // 2

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }

    selector.ExcludeBackend(lb::tcp::Backend("127.0.0.1", 8080));
    DEBUG("Excluded backend: 127.0.0.1:8080");
    for (int i = 0; i < 100; i++) {

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));  // 5

        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("http://www.google.com"));
    }

    selector.ExcludeBackend(lb::tcp::Backend("http://www.google.com"));

    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));
        ASSERT_EQ(selector.SelectBackend(notused), lb::tcp::Backend("127.0.0.1", 8083));  // 5
    }

    ASSERT_ANY_THROW(selector.ExcludeBackend(lb::tcp::Backend("127.0.0.1", 8083)));
}

TEST(WeightedRoundRobin, errorNoWeight)
{
    YAML::Node selector_config = YAML::Load(
R"(algorithm: weighted_round_robin
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

    lb::tcp::WeightedRoundRobinSelector selector;
    ASSERT_THROW( selector.Configure(selector_config), std::runtime_error); // No weights
}

TEST(LeastConnections, basicUsage)
{
    YAML::Node selector_config = YAML::Load(
R"(algorithm: least_connections
endpoints:
  - ip: "127.0.0.1"
    port: 8081
  - ip: "127.0.0.2"
    port: 8082
  - ip: "127.0.0.3"
    port: 8083
)");

    lb::tcp::LeastConnectionsSelector selector;
    selector.Configure(selector_config);
    boost::asio::ip::tcp::endpoint notused(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
    auto b1 = lb::tcp::Backend("127.0.0.1", 8081);
    auto b2 = lb::tcp::Backend("127.0.0.2", 8082);
    auto b3 = lb::tcp::Backend("127.0.0.3", 8083);

    ASSERT_EQ(selector.SelectBackend(notused), b1);
    ASSERT_EQ(selector.SelectBackend(notused), b3);
    ASSERT_EQ(selector.SelectBackend(notused), b2);

    ASSERT_EQ(selector.SelectBackend(notused), b2);
    ASSERT_EQ(selector.SelectBackend(notused), b3);
    ASSERT_EQ(selector.SelectBackend(notused), b1);


    selector.DecreaseConnectionCount(b1);
    selector.DecreaseConnectionCount(b1);

    ASSERT_EQ(selector.SelectBackend(notused), b1);
    ASSERT_EQ(selector.SelectBackend(notused), b1);

    selector.DecreaseConnectionCount(b2);
    selector.DecreaseConnectionCount(b2);

    ASSERT_EQ(selector.SelectBackend(notused), b2);
    ASSERT_EQ(selector.SelectBackend(notused), b2);

    selector.DecreaseConnectionCount(b3);
    selector.DecreaseConnectionCount(b3);

    ASSERT_EQ(selector.SelectBackend(notused), b3);
    ASSERT_EQ(selector.SelectBackend(notused), b3);
}


TEST(ConsistentHash, basicUsage)
{
    YAML::Node selector_config = YAML::Load(
R"(algorithm: consistent_hash
endpoints:
  - ip: 127.0.0.1
    port: 8081
  - ip: 127.0.0.2
    port: 8082
  - ip: 127.0.0.3
    port: 8083
)");

    lb::tcp::ConsistentHashSelector selector;
    selector.Configure(selector_config);
    boost::asio::ip::tcp::endpoint notused(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
    auto b1 = lb::tcp::Backend("127.0.0.1", 8081);
    auto b2 = lb::tcp::Backend("127.0.0.2", 8082);
    auto b3 = lb::tcp::Backend("127.0.0.3", 8083);

    selector.ExcludeBackend(b3);
    selector.ExcludeBackend(b2);
    ASSERT_THROW(selector.ExcludeBackend(b1), std::runtime_error); // No backend left

}