#include <lb/application.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <cstdlib>

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
{}

void Application::LoadConfig(const std::string& config_file)
{
    config = YAML::LoadFile(config_file);
}

void Application::Start()
{

}

int run(int argc, char** argv)
{
    opt::options_description desc("\033[32mAllowed options\033[0m");
    desc.add_options()
        ("help", "show help message")
        ("config", opt::value<std::string>(), "path to config file")
    ;

    opt::variables_map parsed_options;
    opt::store(opt::parse_command_line(argc, argv, desc), parsed_options);
    opt::notify(parsed_options);

    if (parsed_options.count("help")) {
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }

    if (parsed_options.count("config") == 0) {
        std::cout << "\033[1;31mMissed config argument!\033[m\n"
                  << desc << std::endl;
        return EXIT_FAILURE;
    }
    std::string config_path = parsed_options["config"].as<std::string>();
    try {
        Application& app = Application::GetInstance();
        app.LoadConfig(config_path);
        app.Start();
    } catch (const std::exception& exc) {
        std::cerr << "\033[1;31m" << exc.what()  << "\033[m\n" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

} // namespace lb