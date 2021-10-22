#include "net_utl.hpp"

#include <boost/asio.hpp>

namespace graphene { namespace peerplays_sidechain {

std::string resolve_host_addr(const std::string &host_name) {
   using namespace boost::asio;
   io_service service;
   ip::tcp::resolver resolver(service);
   auto query = ip::tcp::resolver::query(host_name, std::string());
   auto iter = resolver.resolve(query);
   auto endpoint = *iter;
   auto addr = ((ip::tcp::endpoint)endpoint).address();
   return addr.to_string();
}

std::string strip_proto_name(const std::string &url) {
   auto index = url.find("://");
   if (index == std::string::npos)
      return url;
   return url.substr(index + 3);
}

}} // namespace graphene::peerplays_sidechain
