#include <iostream>
#include <fstream>
#include <cstdlib>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>

#include <lb/logging.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>


#include <lb/tcp/acceptor.hpp>
#include <lb/application.hpp>


namespace opt=boost::program_options;

namespace lb {

Application& Application::GetInstance()
{
    static Application app;
    return app;
}

const YAML::Node& Application::Config() const
{
    return config;
}

Application::Application()
    : io_context(boost::thread::hardware_concurrency())
{}

void Application::LoadConfig(const std::string& config_file)
{
    config = std::move(YAML::LoadFile(config_file));
}

void Application::ConfigureThreadPool(const YAML::Node& config)
{
    if (!config["thread_pool"].IsDefined()) {
        throw std::runtime_error("missed thread-pool configuration");
    }
    const YAML::Node& tp_config = config["thread_pool"];
    if (!tp_config.IsMap()) {
        throw std::runtime_error("thread_pool configuration must be a map");
    }

}

void ConfigureLogging(const YAML::Node& config)
{
    if (!config["logging"].IsDefined()) {
        throw std::runtime_error("missed logging node!");
    }

    const YAML::Node& logging_node = config["logging"];
    if (!logging_node.IsMap()) {
        throw std::runtime_error("logging field must be a map!");
    }

    std::vector<spdlog::sink_ptr> sinks;

    // configure console sinks
    if (logging_node["console"].IsDefined()) {
        const YAML::Node& console_node = logging_node["console"];
        if (!console_node.IsMap()) {
            throw std::runtime_error("console field must be a map");
        }

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        spdlog::level::level_enum lvl = spdlog::level::from_str(console_node["level"].as<std::string>());
        console_sink->set_level(lvl);

        std::string pattern = "[%H:%M:%S][%^%l%$][%s:%#] %v";
        if (console_node["pattern"].IsDefined()) {
            pattern = console_node["pattern"].as<std::string>();
        }
        console_sink->set_pattern(pattern);
        console_sink->set_color_mode(spdlog::color_mode::automatic);

        sinks.push_back(console_sink);
    }

    // configure basic file logging
    if (logging_node["file"].IsDefined()) {
        const YAML::Node& file_node = logging_node["file"];
        if (!file_node.IsMap()) {
            throw std::runtime_error("file field must be a map");
        }

        std::string name = "logs.txt";
        if (file_node["name"].IsDefined()) {
            name = file_node["name"].as<std::string>();
        }

        bool truncate = true;
        if (file_node["trucncate"].IsDefined()) {
            truncate = file_node["truncate"].as<bool>();
        }
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(name, truncate);

        spdlog::level::level_enum lvl = spdlog::level::from_str(file_node["level"].as<std::string>());
        file_sink->set_level(lvl);

        std::string pattern = "[%H:%M:%S][%^%l%$][%s:%#] %v";
        if (file_node["pattern"].IsDefined()) {
            pattern = file_node["pattern"].as<std::string>();
        }
        file_sink->set_pattern(pattern);
        sinks.push_back(file_sink);
    }

    auto logger = std::make_shared<spdlog::logger>("multi-sink", sinks.begin(), sinks.end());

    spdlog::register_logger(logger);
}

tcp::Acceptor::Configuration ConfigFromYAML(const YAML::Node& config)
{
    if (!config["acceptor"].IsDefined()) {
        throw std::runtime_error("missed acceptor node!");
    }

    const YAML::Node& acceptor_node = config["acceptor"];
    if (!acceptor_node.IsMap()) {
        throw std::runtime_error("acceptor node must be a map");
    }

    bool useIpV6 = false;
    if (!acceptor_node["ip_version"].IsDefined()) {
        int version = acceptor_node["ip_version"].as<int>();
        if (version != 4 || version != 6) {
            throw std::runtime_error("Invalid ip_version " + version);
        }
        useIpV6 = (version == 6);
    }

    return tcp::Acceptor::Configuration{
        .port=acceptor_node["port"].as<tcp::Acceptor::PortType>(),
        .useIpV6=useIpV6
    };
}

void PrintPrettyUsageMessage()
{
    opt::options_description colored_description("\033[1;36mAllowed options\033[0m");
    // .clang-format off
    colored_description.add_options()
        ("help", "\033[32mshow help message\033[0m")
        ("config", opt::value<std::string>()->default_value("config.yaml"),
         "\033[32mpath to config file\033[0m")
    ;
    // .clang-format on
    colored_description.print(std::cout, 20);
}

void HandleInterruptSignal(const boost::system::error_code& ec, int signum)
{
    if (ec) {
        CRITICAL("{}", ec.message());
    }
    WARN("Caught signal: {}", signum);
    INFO("Starting shutdown");
    ::lb::Application::GetInstance().Terminate();
    spdlog::get("multi-sink")->flush();
    return;
}

void Application::Start()
{
    INFO("Starting app");

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait(HandleInterruptSignal);

    tcp::Acceptor acceptor(io_context, ConfigFromYAML(Config()));
    acceptor.Run();

    io_context.run();
    INFO("Finishing app");
}

void Application::Terminate()
{
    io_context.stop();
}

int run(int argc, char** argv)
{
    opt::options_description desc("Allowed options");

    // .clang-format off
    desc.add_options()
        ("help", "show help message")
        ("config", opt::value<std::string>()->default_value("configs/config.yaml"),
         "path to config file")
    ;
    // .clang-format on

    opt::variables_map parsed_options;
    opt::store(opt::parse_command_line(argc, argv, desc), parsed_options);
    opt::notify(parsed_options);

    if (parsed_options.count("help")) {
        PrintPrettyUsageMessage();
        return EXIT_FAILURE;
    }

    if (parsed_options.count("config") == 0) {
        std::cout << "\033[1;31mMissed config argument!\033[m\n" << std::endl;
        PrintPrettyUsageMessage();
        return EXIT_FAILURE;
    }
    std::string config_path = parsed_options["config"].as<std::string>();
    try {
        Application& app = Application::GetInstance();
        app.LoadConfig(config_path);
        spdlog::info("Start configuring logs");
        ConfigureLogging(app.Config());
        INFO("Logging configured");
        app.Start();
    } catch (const std::exception& exc) {
        CRITICAL("{}", exc.what());
        CRITICAL("Exit code: {}", EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    INFO("Exit code: {}", EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

} // namespace lb