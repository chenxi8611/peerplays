#pragma once

#include <string>

namespace graphene { namespace peerplays_sidechain {

// resolve IP address by host name
// ex: api.hive.blog -> 52.79.10.214
std::string resolve_host_addr(const std::string &host_name);

// remove schema part from URL
// ex: http://api.hive.blog -> api.hive.blog
std::string strip_proto_name(const std::string &url);

}} // namespace graphene::peerplays_sidechain
