#include <gtest/gtest.h>
#include <lb/application.hpp>

class TestingEnvironment : public ::testing::Environment {
public:
    void SetUp() override
    {
        std::string config_path = "configs/config.yaml";
        if (char* path = getenv("LB_CONFIG")) {
            config_path = std::string(path);
        }
        auto& app  = lb::Application::GetInstance();
        lb::ConfigureApplication(app, config_path);
    }

    ~TestingEnvironment() override {}
};

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    auto* env = ::testing::AddGlobalTestEnvironment(new TestingEnvironment());
    return RUN_ALL_TESTS();
}