#include <lb/url.hpp>
#include <gtest/gtest.h>


TEST(UrlTests, multiple)
{
    lb::Url url("https://www.example.co.uk:443/blog/article/search?docid=720&hl=en#dayone");
    ASSERT_EQ(url.Protocol(), "https");
    ASSERT_EQ(url.Hostname(), "www.example.co.uk");
    ASSERT_EQ(url.Port(), "443");
    ASSERT_EQ(url.Path(), "/blog/article/search");
    ASSERT_EQ(url.Query(), "docid=720&hl=en");
    ASSERT_EQ(url.Fragment(), "dayone");
}

TEST(UrlTests, multipleNoPort)
{
    lb::Url url("https://www.example.co.uk/blog/article/search?docid=720&hl=en#dayone");
    ASSERT_EQ(url.Protocol(), "https");
    ASSERT_EQ(url.Hostname(), "www.example.co.uk");
    ASSERT_EQ(url.Port(), "443");
    ASSERT_EQ(url.Path(), "/blog/article/search");
    ASSERT_EQ(url.Query(), "docid=720&hl=en");
    ASSERT_EQ(url.Fragment(), "dayone");
}

TEST(UrlTests, noPort)
{
    lb::Url url("https://www.example.co.uk/blog/article/search?docid=720&hl=en#dayone");
    ASSERT_EQ(url.Protocol(), "https");
    ASSERT_EQ(url.Hostname(), "www.example.co.uk");
    ASSERT_EQ(url.Port(), "443");
    ASSERT_EQ(url.Path(), "/blog/article/search");
    ASSERT_EQ(url.Query(), "docid=720&hl=en");
    ASSERT_EQ(url.Fragment(), "dayone");
}
TEST(UrlTests, noQuery)
{
    lb::Url url("https://www.example.co.uk/blog/article/search#dayone");
    ASSERT_EQ(url.Protocol(), "https");
    ASSERT_EQ(url.Hostname(), "www.example.co.uk");
    ASSERT_EQ(url.Port(), "443");
    ASSERT_EQ(url.Path(), "/blog/article/search");
    ASSERT_EQ(url.Query(), "");
    ASSERT_EQ(url.Fragment(), "dayone");
}

TEST(UrlTests, noPath)
{
    lb::Url url("https://www.example.co.uk/blog/article/search?docid=720&hl=en");
    ASSERT_EQ(url.Protocol(), "https");
    ASSERT_EQ(url.Hostname(), "www.example.co.uk");
    ASSERT_EQ(url.Port(), "443");
    ASSERT_EQ(url.Path(), "/blog/article/search");
    ASSERT_EQ(url.Query(), "docid=720&hl=en");
    ASSERT_EQ(url.Fragment(), "");
}