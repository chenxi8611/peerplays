#include <graphene/peerplays_sidechain/common/rpc_client.hpp>

#include <sstream>
#include <string>

//#include <boost/asio.hpp>
//#include <boost/asio/ssl.hpp>

#include <boost/asio/buffer.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <curl/curl.h>

#include <fc/crypto/base64.hpp>
#include <fc/log/logger.hpp>
#include <fc/network/ip.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace graphene { namespace peerplays_sidechain {

using namespace boost::asio;

namespace detail {

// https_call_impl

class https_call_impl {
public:
   https_call_impl(https_call &call, const void *body_data, size_t body_size, http_response &response);
   void exec();

private:
   https_call &m_call;
   const void *m_body_data;
   size_t m_body_size;
   http_response &m_response;

   ssl::stream<ip::tcp::socket> m_socket;
   streambuf m_response_buf;
   int32_t m_content_length;

private:
   void connect();
   void send_request();
   void process_headers();
   void process_response();
};

static const char cr = 0x0D;
static const char lf = 0x0A;
static const char *crlf = "\x0D\x0A";
static const char *crlfcrlf = "\x0D\x0A\x0D\x0A";

https_call_impl::https_call_impl(https_call &call, const void *body_data, size_t body_size, http_response &response) :
      m_call(call),
      m_body_data(body_data),
      m_body_size(body_size),
      m_response(response),
      m_socket(m_call.m_service, m_call.m_context),
      m_response_buf(https_call::response_size_limit_bytes) {
}

void https_call_impl::exec() {
   connect();
   send_request();
   process_response();
}

void https_call_impl::connect() {

   // TCP connect

   m_socket.lowest_layer().connect(m_call.m_endpoint);

   // SSL connect

   if (!SSL_set_tlsext_host_name(m_socket.native_handle(), m_call.m_host.c_str())) {
      FC_THROW("SSL_set_tlsext_host_name failed");
   }

   m_socket.set_verify_mode(ssl::verify_peer);
   m_socket.handshake(ssl::stream_base::client);
}

void https_call_impl::send_request() {

   streambuf request;
   std::ostream stream(&request);

   // start string: <method> <path> HTTP/1.0

   stream << m_call.m_method << " " << m_call.m_path << " HTTP/1.0" << crlf;

   // host

   stream << "Host: " << m_call.m_host << ":" << m_call.m_endpoint.port() << crlf;

   // content

   if (m_body_size) {
      stream << "Content-Type: " << m_call.m_content_type << crlf;
      stream << "Content-Length: " << m_body_size << crlf;
   }

   // additional headers

   const auto &h = m_call.m_headers;

   if (!h.empty()) {
      if (h.size() < 2) {
         FC_THROW("invalid headers data");
      }
      stream << h;
      // ensure headers finished correctly
      if ((h.substr(h.size() - 2) != crlf))
         stream << crlf;
   }

   // other

   //      stream << "Accept: *\x2F*" << crlf;
   stream << "Accept: text/html, application/json" << crlf;
   stream << "Connection: close" << crlf;

   // end

   stream << crlf;

   // send headers

   write(m_socket, request);

   // send body

   if (m_body_size)
      write(m_socket, buffer(m_body_data, m_body_size));
}

void https_call_impl::process_headers() {

   std::istream stream(&m_response_buf);

   std::string http_version;
   stream >> http_version;
   stream >> m_response.status_code;

   boost::algorithm::trim(http_version);
   boost::algorithm::to_lower(http_version);

   if (!stream || http_version.substr(0, 5) != "http/") {
      FC_THROW("invalid response data");
   }

   // read/skip headers

   m_content_length = -1;

   for (;;) {
      std::string header;
      if (!std::getline(stream, header, lf) || (header.size() == 1 && header[0] == cr))
         break;
      if (m_content_length) // if content length is already known
         continue;          // continue skipping headers
      auto pos = header.find(':');
      if (pos == std::string::npos)
         continue;
      auto name = header.substr(0, pos);
      boost::algorithm::trim(name);
      boost::algorithm::to_lower(name);
      if (name != "content-length")
         continue;
      auto value = header.substr(pos + 1);
      boost::algorithm::trim(value);
      m_content_length = std::stol(value);
   }
}

void https_call_impl::process_response() {

   auto &socket = m_socket;
   auto &buf = m_response_buf;
   auto &content_length = m_content_length;
   auto &body = m_response.body;

   read_until(socket, buf, crlfcrlf);

   process_headers();

   // check content length

   if (content_length >= 0) {
      if (content_length < 2) { // minimum content is "{}"
         FC_THROW("invalid response body (too short)");
      }
      if (content_length > https_call::response_size_limit_bytes) {
         FC_THROW("response body size limit exceeded");
      }
   }

   boost::system::error_code ec;

   for (;;) {

      auto readed = read(socket, buf, transfer_at_least(1), ec);

      if (ec)
         break;

      if (!readed) {
         if (buf.size() == buf.max_size())
            FC_THROW("response body size limit exceeded");
         else
            FC_THROW("logic error: read failed but no error conditon");
      }

      if (content_length >= 0) {
         if (buf.size() > content_length) {
            FC_THROW("read more than content-length");
         }
      }
   }

   {
      boost::system::error_code ec;
      socket.shutdown(ec);
   }

   if ((ec != error::eof) &&
       (ec != ssl::error::stream_truncated)) {
      throw boost::system::system_error(ec);
   }

   if (content_length >= 0) {
      if (buf.size() != content_length) {
         FC_THROW("actual body size differs from content-length");
      }
   }

   auto size = buf.size();
   body.resize(size);
   if (size != buf.sgetn(&body[0], size)) {
      FC_THROW("stream read failed");
   }
}

} // namespace detail

// https_call

https_call::https_call(const std::string &host, const std::string &ip_addr, uint16_t port, const std::string &method, const std::string &path, const std::string &headers, const std::string &content_type) :
      m_host(host),
      m_method(method),
      m_path(path),
      m_headers(headers),
      m_content_type(content_type),
      m_service(),
      m_context(ssl::context::tlsv12_client),
      m_endpoint(ip::address::from_string(ip_addr), port) {
   m_context.set_default_verify_paths();
   m_context.set_options(ssl::context::default_workarounds);
}

bool https_call::exec(const void *body_data, size_t body_size, http_response *response) {

   //ASSERT(response);
   auto &resp = *response;

   detail::https_call_impl impl(*this, body_data, body_size, resp);

   m_error_what = decltype(m_error_what)();

   try {
      try {
         resp.clear();
         impl.exec();
      } catch (const std::exception &e) {
         resp.clear();
         m_error_what = e.what();
         return false;
      }
   } catch (...) {
      resp.clear();
      m_error_what = "unknown exception";
      return false;
   }

   return true;
}

}} // namespace graphene::peerplays_sidechain

namespace graphene { namespace peerplays_sidechain {

static std::string resolve_host_addr(const std::string &host_name) {
   using namespace boost::asio;
   io_service service;
   ip::tcp::resolver resolver(service);
   auto query = ip::tcp::resolver::query(host_name, std::string());
   auto iter = resolver.resolve(query);
   auto endpoint = *iter;
   auto addr = ((ip::tcp::endpoint)endpoint).address();
   return addr.to_string();
}

static std::string strip_proto_name(const std::string &url, std::string *schema) {
   auto index = url.find("://");
   if (index == std::string::npos) {
      if (schema)
         schema->clear();
      return url;
   }
   if (schema)
      schema->assign(&url[0], &url[index]);
   return url.substr(index + 3);
}

}} // namespace graphene::peerplays_sidechain

namespace graphene { namespace peerplays_sidechain {

rpc_client::rpc_client(std::string url, uint32_t _port, std::string _user, std::string _password, bool _debug_rpc_calls) :
      port(_port),
      user(_user),
      password(_password),
      debug_rpc_calls(_debug_rpc_calls),
      request_id(0) {

   authorization.key = "Authorization";
   authorization.val = "Basic " + fc::base64_encode(user + ":" + password);

   std::string schema;
   host = strip_proto_name(url, &schema);
   boost::algorithm::to_lower(schema);

   try {
      fc::ip::address temp(host); // try to convert host string to valid IPv4 address
      ip = host;
   } catch (...) {
      try {
         ip = resolve_host_addr(host);
         fc::ip::address temp(ip);
      } catch (...) {
         elog("Failed to resolve Hive node address ${ip}", ("ip", url));
         FC_ASSERT(false);
      }
   }

   if (schema == "https")
      https = new https_call(host, ip, port, "POST", "/", authorization.key + ":" + authorization.val, "application/json");
   else
      https = 0;
}

std::string rpc_client::retrieve_array_value_from_reply(std::string reply_str, std::string array_path, uint32_t idx) {
   std::stringstream ss(reply_str);
   boost::property_tree::ptree json;
   boost::property_tree::read_json(ss, json);
   if (json.find("result") == json.not_found()) {
      return "";
   }
   auto json_result = json.get_child("result");
   if (json_result.find(array_path) == json_result.not_found()) {
      return "";
   }

   boost::property_tree::ptree array_ptree = json_result;
   if (!array_path.empty()) {
      array_ptree = json_result.get_child(array_path);
   }
   uint32_t array_el_idx = -1;
   for (const auto &array_el : array_ptree) {
      array_el_idx = array_el_idx + 1;
      if (array_el_idx == idx) {
         std::stringstream ss_res;
         boost::property_tree::json_parser::write_json(ss_res, array_el.second);
         return ss_res.str();
      }
   }

   return "";
}

std::string rpc_client::retrieve_value_from_reply(std::string reply_str, std::string value_path) {
   std::stringstream ss(reply_str);
   boost::property_tree::ptree json;
   boost::property_tree::read_json(ss, json);
   if (json.find("result") == json.not_found()) {
      return "";
   }
   auto json_result = json.get_child("result");
   if (json_result.find(value_path) == json_result.not_found()) {
      return "";
   }
   return json_result.get<std::string>(value_path);
}

std::string rpc_client::send_post_request(std::string method, std::string params, bool show_log) {
   std::stringstream body;

   request_id = request_id + 1;

   body << "{ \"jsonrpc\": \"2.0\", \"id\": " << request_id << ", \"method\": \"" << method << "\"";

   if (!params.empty()) {
      body << ", \"params\": " << params;
   }

   body << " }";

   const auto reply = send_post_request(body.str(), show_log);

   if (reply.body.empty()) {
      wlog("RPC call ${function} failed", ("function", __FUNCTION__));
      return "";
   }

   std::stringstream ss(std::string(reply.body.begin(), reply.body.end()));
   boost::property_tree::ptree json;
   boost::property_tree::read_json(ss, json);

   if (reply.status_code == 200) {
      return ss.str();
   }

   if (json.count("error") && !json.get_child("error").empty()) {
      wlog("RPC call ${function} with body ${body} failed with reply '${msg}'", ("function", __FUNCTION__)("body", body.str())("msg", ss.str()));
   }
   return "";
}

http_response rpc_client::send_post_request(std::string body, bool show_log) {

   http_response response;

   if (https) {

      https->exec(body.c_str(), body.size(), &response);

   } else {

      std::string url = "http://" + host + ":" + std::to_string(port);
      fc::ip::address addr(ip);

      try {

         fc::http::connection conn;
         conn.connect_to(fc::ip::endpoint(addr, port));

         //if (wallet.length() > 0) {
         //   url = url + "/wallet/" + wallet;
         //}

         auto r = conn.request("POST", url, body, fc::http::headers{authorization});
         response.status_code = r.status;
         response.body.assign(r.body.begin(), r.body.end());

      } catch (...) {
      }
   }

   if (show_log) {
      std::string url = host + ":" + std::to_string(port);
      ilog("### Request URL:    ${url}", ("url", url));
      ilog("### Request:        ${body}", ("body", body));
      std::stringstream ss(std::string(response.body.begin(), response.body.end()));
      ilog("### Response:       ${ss}", ("ss", ss.str()));
   }

   return response;
}

}} // namespace graphene::peerplays_sidechain
