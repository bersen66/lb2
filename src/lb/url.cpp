#include <lb/url.hpp>
#include <lb/logging.hpp>

namespace lb
{

    std::string DefaultProtocolPort(const std::string &protocol)
    {
        static const std::unordered_map<std::string, std::string> protocol_to_default_port = {
            {"http", "80"},
            {"https", "443"},
            {"ftp", "21"},
            {"ssh", "22"},
        };

        if (auto it = protocol_to_default_port.find(protocol); it != protocol_to_default_port.end())
        {
            return it->second;
        }
        STACKTRACE("Unknown protocol: {}. Specify port mannualy", protocol);
    }

    bool Url::IsUrl(const std::string &src)
    {
        auto match_results = MatchUrl(src);
        return match_results;
    }

    Url::Url(const std::string &string)
    {
        auto match_results = MatchUrl(string);
        if (!match_results)
        {
            STACKTRACE("Invalid url: {}", string);
        }

        protocol = !match_results.get<2>().to_string().empty() ? match_results.get<2>().to_string() : "http";
        hostname = match_results.get<3>().to_string();
        port = !match_results.get<5>().to_string().empty() ? match_results.get<5>().to_string() : DefaultProtocolPort(protocol);
        path = match_results.get<6>().to_string();
        query = match_results.get<8>().to_string();
        fragment = match_results.get<10>().to_string();
    }

    const std::string &Url::Protocol() const noexcept
    {
        return protocol;
    }

    const std::string &Url::Hostname() const noexcept
    {
        return hostname;
    }

    const std::string &Url::Port() const noexcept
    {
        return port;
    }

    const std::string &Url::Path() const noexcept
    {
        return path;
    }

    const std::string &Url::Query() const noexcept
    {
        return query;
    }

    const std::string &Url::Fragment() const noexcept
    {
        return fragment;
    }

    bool operator==(const Url &lhs, const Url &rhs)
    {
        return std::tie(lhs.Protocol(), lhs.Hostname(), lhs.Port(), lhs.Path(), lhs.Query(), lhs.Fragment())
               == std::tie(rhs.Protocol(), rhs.Hostname(), rhs.Port(), rhs.Path(), rhs.Query(), rhs.Fragment());
    }

} // namespace lb