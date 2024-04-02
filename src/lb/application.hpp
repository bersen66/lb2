#pragma once

#include <boost/asio.hpp>
#include <yaml-cpp/yaml.h>

#include <lb/tcp/connector.hpp>

namespace lb {


class Application {
public:
    static Application& GetInstance();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;
    ~Application() = default;

    const YAML::Node& Config() const;

    void Start();

    void ConfigureThreadPool(const YAML::Node& config);

    tcp::Connector& Connector();

    void Terminate();
private:
    friend int run(int argc, char** argv);
    Application();
    void LoadConfig(const std::string& config_path);
private:
    YAML::Node config;
    boost::asio::io_context io_context;
    tcp::Connector connector;
};

int run(int argc, char** argv);

} // namespace lb