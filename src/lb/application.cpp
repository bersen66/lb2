#include <iostream>
#include <fstream>
#include <cstdlib>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>

#include <lb/logging.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <lb/tcp/connector.hpp>
#include <lb/application.hpp>
#include <lb/tcp/acceptor.hpp>



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
{
}

void Application::LoadConfig(const std::string& config_file)
{
    config = std::move(YAML::LoadFile(config_file));
}

void ConfigureLogging(const YAML::Node& config)
{
    if (!config["logging"].IsDefined()) {
        EXCEPTION("missed logging node!");
    }

    const YAML::Node& logging_node = config["logging"];
    if (!logging_node.IsMap()) {
        STACKTRACE("logging field must be a map!");
    }

    std::vector<spdlog::sink_ptr> sinks;

    // configure console sinks
    if (logging_node["console"].IsDefined()) {
        const YAML::Node& console_node = logging_node["console"];
        if (!console_node.IsMap()) {
            STACKTRACE("console field must be a map");
        }

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        spdlog::level::level_enum lvl = spdlog::level::from_str(console_node["level"].as<std::string>());
        console_sink->set_level(lvl);
        console_sink->should_log(lvl);

        std::string pattern = "[%H:%M:%S][%t][%^%l%$][%s:%#] %v";
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
            STACKTRACE("file field must be a map");
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
        file_sink->should_log(lvl);
        file_sink->set_level(lvl);

        std::string pattern = "[%H:%M:%S][%t][%^%l%$][%s:%#] %v";
        if (file_node["pattern"].IsDefined()) {
            pattern = file_node["pattern"].as<std::string>();
        }
        file_sink->set_pattern(pattern);
        sinks.push_back(file_sink);
    }

    auto logger = std::make_shared<spdlog::logger>("multi-sink", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    spdlog::register_logger(logger);
}

std::size_t ConfigureThreadPool(const YAML::Node& config)
{
    std::size_t threads_num = boost::thread::hardware_concurrency();
    DEBUG("threads_num = {}", threads_num);
    threads_num += (threads_num == 0);

    if (config["thread_pool"].IsDefined()) {
        const YAML::Node& tp_config = config["thread_pool"];
        if (!tp_config.IsMap()) {
            EXCEPTION("thread_pool configuration must be a map");
        }

        if (tp_config["threads_number"].as<std::string>() == "auto") {
            DEBUG("Selected automatically threads_num={}", threads_num);
            return threads_num;
        }

        if (tp_config["threads_number"].IsScalar()) {
            threads_num = tp_config["threads_number"].as<std::size_t>();
            DEBUG("Selected explicitly threads_num={}", threads_num);
        }
    }

    return threads_num;
}

tcp::Acceptor::Configuration ConfigFromYAML(const YAML::Node& config)
{
    if (!config["acceptor"].IsDefined()) {
        EXCEPTION("missed acceptor node!");
    }

    const YAML::Node& acceptor_node = config["acceptor"];
    if (!acceptor_node.IsMap()) {
        EXCEPTION("acceptor node must be a map");
    }

    bool useIpV6 = false;
    if (!acceptor_node["ip_version"].IsDefined()) {
        int version = acceptor_node["ip_version"].as<int>();
        if (version != 4 || version != 6) {
            EXCEPTION("Invalid ip_version {}", version);
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
    try {
        if (ec) {
            CRITICAL("{}", ec.message());
        }
        WARN("Caught signal: {}", signum);
        INFO("Starting shutdown");
        ::lb::Application::GetInstance().Terminate();
        spdlog::get("multi-sink")->flush();
    } catch (const std::exception& exc) {
        CRITICAL("Error at signal handler: {}", exc.what());
    }

    return;
}

void Application::RegisterConnector(tcp::Connector* connector)
{
    connector_ptr = connector;
}

void MakeThisThreadMaximumPriority()
{
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
}

void ThreadRoutine(boost::asio::io_context& ioc, boost::barrier& barrier)
{
    barrier.wait();
    MakeThisThreadMaximumPriority();

    Application& app = Application::GetInstance();
    try {
        INFO("Starting io_context");
        ioc.run();
        INFO("Exiting io_context");
    } catch (const std::exception& exc) {
        CRITICAL("Exception at io_context: {}", exc.what());
        app.SetExitCode(EXIT_FAILURE);
        app.Terminate();
    } catch (...) {
        CRITICAL("Unknown exception at io_context");
        app.Terminate();
        app.SetExitCode(EXIT_FAILURE);
    }
    app.SetExitCode(EXIT_SUCCESS);
}

void Application::Start()
{
    INFO("Starting app");

    tcp::Acceptor acceptor(io_context, ConfigFromYAML(Config()));
    acceptor.Run();

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&acceptor](const boost::system::error_code& ec, int signum) {
        acceptor.Stop();
        HandleInterruptSignal(ec, signum);
    });

    lb::tcp::SelectorPtr selector = lb::tcp::DetectSelector(Config());
    lb::tcp::Connector connector(io_context, selector);
    RegisterConnector(&connector);

    std::size_t threads_num = ConfigureThreadPool(Config());
    INFO("Threads num={}", threads_num);
    boost::barrier barrier(threads_num);
    for (std::size_t i = 0; i + 1 < threads_num; ++i) {
        threads.create_thread([this, &barrier](){
            ThreadRoutine(io_context, barrier);
        });
    }
    ThreadRoutine(io_context, barrier);
    threads.join_all();
    INFO("Finishing app");
}

void Application::Terminate()
{
    io_context.stop();
}

tcp::Connector& Application::Connector()
{
    return *connector_ptr;
}

void Application::SetExitCode(int code)
{
    if (code != EXIT_SUCCESS) {
        exit_code = code;
    }
}

int Application::GetExitCode() const
{
    return exit_code;
}

void ConfigureApplication(lb::Application& app, const std::string& config_path) {
    try {
        app.LoadConfig(config_path);
        spdlog::info("Start configuring logs");
        ConfigureLogging(app.Config());
        DEBUG("Logging configured");
    } catch (const std::exception& exc) {
        CRITICAL("Exception while configuring app: {}", exc.what());
        CRITICAL("Exit code: {}", EXIT_FAILURE);
        std::abort();
    }
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
    Application& app = Application::GetInstance();
    ConfigureApplication(app, config_path);
    app.Start();
    INFO("Exit code: {}", app.GetExitCode());
    return app.GetExitCode();
}

} // namespace lb