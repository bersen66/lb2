#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <yaml-cpp/yaml.h>

#include <lb/tcp/connector.hpp>
#include <atomic>

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

    tcp::Connector& Connector();

    void Terminate();

    void SetExitCode(int code);

    int GetExitCode() const;
private:
    friend void ConfigureApplication(Application& app, const std::string& config_path);
    Application();
    void LoadConfig(const std::string& config_path);
    void RegisterConnector(tcp::Connector* connector);
private:
    YAML::Node config;
    boost::asio::io_context io_context;
    boost::thread_group threads;
    tcp::Connector* connector_ptr; // not owner
    std::atomic<int> exit_code = EXIT_SUCCESS;
};

void ConfigureApplication(Application& app, const std::string& config_path);

int run(int argc, char** argv);


} // namespace lb