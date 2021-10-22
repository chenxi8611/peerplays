#pragma once

#include <string>

namespace peerplays {
namespace net {

// resolve IP address by host name
// ex: api.hive.blog -> 52.79.10.214
std::string resolveHostAddr(const std::string & hostName);

// remove schema part from URL
// ex: http://api.hive.blog -> api.hive.blog
std::string stripProtoName(const std::string & url);


} // net
} // peerplays
