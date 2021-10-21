#include "net_utl.h"

#include <boost/asio.hpp>


namespace peerplays {
namespace net {

std::string resolveHostAddr(const std::string & hostName) {
	using namespace boost::asio;
	io_service service;
	ip::tcp::resolver resolver(service);
	auto query = ip::tcp::resolver::query(hostName, "");
	auto iter = resolver.resolve(query);
	auto endpoint = *iter;
	auto addr = ((ip::tcp::endpoint)endpoint).address();
	return addr.to_string();
}

std::string stripProtoName(const std::string & url) {
	auto index = url.find("://");
	if (index == std::string::npos)
		return url;
	return url.substr(index + 3);
}

} // net
} // peerplays
