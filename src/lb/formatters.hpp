#pragma once

#include <spdlog/fmt/ostr.h>
#include <boost/stacktrace.hpp>
#include <boost/beast.hpp>

namespace lb::tcp {class Backend;}
namespace YAML {class Node;}

template <> struct fmt::formatter<boost::stacktrace::stacktrace> : ostream_formatter{};
template <> struct fmt::formatter<YAML::Node> : ostream_formatter{};
template <> struct fmt::formatter<lb::tcp::Backend> : ostream_formatter{};
template <> struct fmt::formatter<boost::beast::http::request<boost::beast::http::string_body>> : ostream_formatter{};
template <> struct fmt::formatter<boost::beast::http::response<boost::beast::http::string_body>> : ostream_formatter{};