#pragma once

#include <string>

namespace peerplays {
namespace net {

std::string resolveHostAddr(const std::string & hostName);
std::string stripProtoName(const std::string & url);


} // net
} // peerplays
