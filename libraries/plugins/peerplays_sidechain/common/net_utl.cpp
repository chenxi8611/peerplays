#include "https_call.h"

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
	auto addr = endpoint.address();
	return addr.to_string();
}


} // net
} // peerplays
