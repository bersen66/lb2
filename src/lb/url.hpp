#pragma once

#include <ctre.hpp>
#include <string_view>

namespace lb {

class Url {
private:
    // TODO: add ip as hostnames
    static constexpr auto url_pattern = ctll::fixed_string{"((\\w+):\\/\\/)?([^/\\s:]+)(:(\\d{2,5}))?([^?\\s#]*)([?]([^\\s#]*))?([#]([^\\s]*))?"};
    static constexpr auto MatchUrl = ctre::match<url_pattern>; // parsing functor
public:
    Url(const std::string& string);
    const std::string& Protocol() const noexcept;
    const std::string& Hostname() const noexcept;
    const std::string& Port() const noexcept;
    const std::string& Path() const noexcept;
    const std::string& Query() const noexcept;
    const std::string& Fragment() const noexcept;

    static bool IsUrl(const std::string& src);
private:
    std::string protocol;
    std::string hostname;
    std::string port;
    std::string path;
    std::string query;
    std::string fragment;
};

bool operator==(const Url& lhs, const Url& rhs);

} // namespace url
