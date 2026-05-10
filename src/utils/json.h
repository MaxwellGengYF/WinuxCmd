#pragma once

#include "json.hpp"


namespace nlohmann {
using ::nlohmann::adl_serializer;
using ::nlohmann::basic_json;
using ::nlohmann::json;
using ::nlohmann::json_pointer;
using ::nlohmann::ordered_json;
using ::nlohmann::ordered_map;
}  // namespace nlohmann

NLOHMANN_JSON_NAMESPACE_BEGIN

namespace detail {
using NLOHMANN_JSON_NAMESPACE::detail::json_sax_dom_callback_parser;
using NLOHMANN_JSON_NAMESPACE::detail::unknown_size;
}  // namespace detail

using NLOHMANN_JSON_NAMESPACE::adl_serializer;
using NLOHMANN_JSON_NAMESPACE::basic_json;
using NLOHMANN_JSON_NAMESPACE::json;
using NLOHMANN_JSON_NAMESPACE::json_pointer;
using NLOHMANN_JSON_NAMESPACE::ordered_json;
using NLOHMANN_JSON_NAMESPACE::ordered_map;
using NLOHMANN_JSON_NAMESPACE::to_string;

NLOHMANN_JSON_NAMESPACE_END
