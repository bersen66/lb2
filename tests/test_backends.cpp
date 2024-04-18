#include <gtest/gtest.h>
#include <lb/tcp/selectors.hpp>


TEST(Backends, construction) {
    {
        lb::tcp::Backend backend("127.0.0.1", 8080);
        ASSERT_EQ(backend.AsEndpoint().address().to_string(), "127.0.0.1");
        ASSERT_EQ(backend.AsEndpoint().port(), 8080);
    }
    {
        lb::tcp::Backend backend("https://www.example.com");
        ASSERT_TRUE(backend.IsUrl());
        ASSERT_EQ(backend.AsUrl().Hostname(), "www.example.com");
        ASSERT_EQ(backend.AsUrl().Port(), "443");
        ASSERT_EQ(backend.AsUrl().Path(), "");
        ASSERT_EQ(backend.AsUrl().Query(), "");
        ASSERT_EQ(backend.AsUrl().Fragment(), "");
    }
    {
        lb::tcp::Backend backend("https://www.example.com/blog/article/search?docid=720&hl=en#dayone");
        ASSERT_TRUE(backend.IsUrl());
        ASSERT_EQ(backend.AsUrl().Hostname(), "www.example.com");
        ASSERT_EQ(backend.AsUrl().Port(), "443");
        ASSERT_EQ(backend.AsUrl().Path(), "/blog/article/search");
        ASSERT_EQ(backend.AsUrl().Query(), "docid=720&hl=en");
        ASSERT_EQ(backend.AsUrl().Fragment(), "dayone");
    }

    {
        lb::tcp::Backend backend("https://www.example.com/blog/article/search?docid=720&hl=en");
        ASSERT_TRUE(backend.IsUrl());
        ASSERT_EQ(backend.AsUrl().Hostname(), "www.example.com");
        ASSERT_EQ(backend.AsUrl().Port(), "443");
        ASSERT_EQ(backend.AsUrl().Path(), "/blog/article/search");
        ASSERT_EQ(backend.AsUrl().Query(), "docid=720&hl=en");
        ASSERT_EQ(backend.AsUrl().Fragment(), "");
    }
    {
        lb::tcp::Backend backend("https://www.example.com/blog/article/search?docid=720&hl=en#dayone");
        ASSERT_TRUE(backend.IsUrl());
        ASSERT_EQ(backend.AsUrl().Hostname(), "www.example.com");
        ASSERT_EQ(backend.AsUrl().Port(), "443");
        ASSERT_EQ(backend.AsUrl().Path(), "/blog/article/search");
        ASSERT_EQ(backend.AsUrl().Query(), "docid=720&hl=en");
        ASSERT_EQ(backend.AsUrl().Fragment(), "dayone");
    }
    {
        lb::tcp::Backend ipv6Backend("5be8:dde9:7f0b:d5a7:bd01:b3be:9c69:573b", 8080);
        ASSERT_TRUE(ipv6Backend.IsIpEndpoint());
        ASSERT_EQ(ipv6Backend.AsEndpoint().address().to_string(), "5be8:dde9:7f0b:d5a7:bd01:b3be:9c69:573b");
        ASSERT_EQ(ipv6Backend.AsEndpoint().port(), 8080);
    }
}