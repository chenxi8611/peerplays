#pragma once

#include <cstdint>
#include <string>

#include <fc/network/http/connection.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace graphene { namespace peerplays_sidechain {

struct http_response {

   uint16_t status_code;
   std::string body;

   void clear() {
      status_code = 0;
      body = decltype(body)();
   }
};

namespace detail {
class https_call_impl;
} // namespace detail

class https_call {
public:
   https_call(const std::string &host, const std::string &ip_addr, uint16_t port, const std::string &method, const std::string &path, const std::string &headers, const std::string &content_type);

   bool exec(const void *body_data, size_t body_size, http_response *response);

   const std::string &error_what() const;

private:
   friend class detail::https_call_impl;
   static constexpr auto response_size_limit_bytes = 1024 * 1024;
   std::string m_host;
   std::string m_method;
   std::string m_path;
   std::string m_headers;
   std::string m_content_type;
   std::string m_error_what;

   boost::asio::io_service m_service;
   boost::asio::ssl::context m_context;
   boost::asio::ip::tcp::endpoint m_endpoint;
};

}} // namespace graphene::peerplays_sidechain

namespace graphene { namespace peerplays_sidechain {

class rpc_client {
public:
   rpc_client(std::string url, uint32_t _port, std::string _user, std::string _password, bool _debug_rpc_calls);

protected:
   std::string retrieve_array_value_from_reply(std::string reply_str, std::string array_path, uint32_t idx);
   std::string retrieve_value_from_reply(std::string reply_str, std::string value_path);
   std::string send_post_request(std::string method, std::string params, bool show_log);

   std::string host;
   std::string ip;
   uint32_t port;
   std::string user;
   std::string password;
   bool debug_rpc_calls;

   uint32_t request_id;

   fc::http::header authorization;

private:
   https_call *https;
   //fc::http::reply send_post_request(std::string body, bool show_log);
   http_response send_post_request(std::string body, bool show_log);
};

}} // namespace graphene::peerplays_sidechain
