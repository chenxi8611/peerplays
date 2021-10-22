#include <graphene/peerplays_sidechain/common/rpc_client.hpp>

#include <sstream>

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <fc/crypto/base64.hpp>
#include <fc/log/logger.hpp>
#include <fc/network/ip.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "net_utl.hpp"

namespace graphene { namespace peerplays_sidechain {

struct http_request {

   std::string method; // ex: "POST"
   std::string path;   // ex: "/"
   std::string headers;
   std::string body;
   std::string content_type; // ex: "application/json"

   http_request() {
   }

   http_request(const std::string &method_, const std::string &path_, const std::string &headers_, const std::string &body_, const std::string &content_type_) :
         method(method_),
         path(path_),
         headers(headers_),
         body(body_),
         content_type(content_type_) {
   }

   http_request(const std::string &method_, const std::string &path_, const std::string &headers_, const std::string &body_ = std::string()) :
         method(method_),
         path(path_),
         headers(headers_),
         body(body_),
         content_type("application/json") {
   }

   void clear() {
      method.clear();
      path.clear();
      headers.clear();
      body.clear();
      content_type.clear();
   }
};

struct http_response {

   uint16_t status_code;
   std::string body;

   void clear() {
      status_code = 0;
      body = decltype(body)();
   }
};

class https_call {
public:
   https_call(const std::string &host, uint16_t port = 0) :
         m_host(host),
         m_port(port) {
   }

   const std::string &host() const {
      return m_host;
   }

   uint16_t port() const {
      return m_port;
   }

   uint32_t response_size_limit_bytes() const {
      return 1024 * 1024;
   }

   bool exec(const http_request &request, http_response *response);

private:
   std::string m_host;
   uint16_t m_port;
};

namespace detail {

static const char cr = 0x0D;
static const char lf = 0x0A;
static const char *crlf = "\x0D\x0A";
static const char *crlfcrlf = "\x0D\x0A\x0D\x0A";

using namespace boost::asio;

[[noreturn]] static void throw_error(const std::string &msg) {
   throw std::runtime_error(msg);
}

class https_call_impl {
public:
   https_call_impl(const https_call &call, const http_request &request, http_response &response) :
         m_call(call),
         m_request(request),
         m_response(response),
         m_service(),
         m_context(ssl::context::tlsv12_client),
         m_socket(m_service, m_context),
         m_endpoint(),
         m_response_buf(call.response_size_limit_bytes()),
         m_content_length(0) {
      m_context.set_default_verify_paths();
   }

   void exec() {
      resolve();
      connect();
      send_request();
      process_response();
   }

private:
   const https_call &m_call;
   const http_request &m_request;
   http_response &m_response;

   io_service m_service;
   ssl::context m_context;
   ssl::stream<ip::tcp::socket> m_socket;
   ip::tcp::endpoint m_endpoint;
   streambuf m_response_buf;
   uint32_t m_content_length;

   void resolve() {

      // resolve TCP endpoint for host name

      ip::tcp::resolver resolver(m_service);
      auto query = ip::tcp::resolver::query(m_call.host(), "https");
      auto iter = resolver.resolve(query);
      m_endpoint = *iter;

      if (m_call.port() != 0)            // if port was specified
         m_endpoint.port(m_call.port()); // force set port
   }

   void connect() {

      // TCP connect

      m_socket.lowest_layer().connect(m_endpoint);

      // SSL connect

      m_socket.set_verify_mode(ssl::verify_peer);
      m_socket.handshake(ssl::stream_base::client);
   }

   void send_request() {

      streambuf request_buf;
      std::ostream stream(&request_buf);

      // start string: <method> <path> HTTP/1.0

      stream << m_request.method << " " << m_request.path << " HTTP/1.0" << crlf;

      // host

      stream << "Host: " << m_call.host();

      if (m_call.port() != 0) {
         //ASSERT(m_Endpoint.port() == m_Call.port());
         stream << ":" << m_call.port();
      }

      stream << crlf;

      // content

      if (!m_request.body.empty()) {
         stream << "Content-Type: " << m_request.content_type << crlf;
         stream << "Content-Length: " << m_request.body.size() << crlf;
      }

      // additional headers

      const auto &h = m_request.headers;

      if (!h.empty()) {
         if (h.size() < 2)
            throw_error("invalid headers data");
         stream << h;
         // ensure headers finished correctly
         if ((h.substr(h.size() - 2) != crlf))
            stream << crlf;
      }

      // other

      stream << "Accept: *\x2F*" << crlf;
      stream << "Connection: close" << crlf;

      // end

      stream << crlf;

      // content

      if (!m_request.body.empty())
         stream << m_request.body;

      // send

      write(m_socket, request_buf);
   }

   void process_headers() {

      std::istream stream(&m_response_buf);

      std::string http_version;
      stream >> http_version;
      stream >> m_response.status_code;

      if (!stream || http_version.substr(0, 5) != "HTTP/") {
         throw_error("invalid response");
      }

      // read/skip headers

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

   void process_response() {

      auto &socket = m_socket;
      auto &buf = m_response_buf;
      auto &content_length = m_content_length;
      auto &body = m_response.body;

      read_until(socket, buf, crlfcrlf);

      process_headers();

      // check content length

      if (content_length < 2) // minimum content is "{}"
         throw_error("invalid response body (too short)");

      if (content_length > m_call.response_size_limit_bytes())
         throw_error("response body size limit exceeded");

      // read body

      auto avail = buf.size(); // size of body data already stored in the buffer

      if (avail > content_length)
         throw_error("invalid response body (content length mismatch)");

      body.resize(content_length);

      if (avail) {
         // copy already existing data
         if (avail != buf.sgetn(&body[0], avail)) {
            throw_error("stream read failed");
         }
      }

      auto rest = content_length - avail; // size of remaining part of response body

      boost::system::error_code error_code;

      read(socket, buffer(&body[avail], rest), error_code); // read remaining part

      socket.shutdown(error_code);
   }
};

} // namespace detail

bool https_call::exec(const http_request &request, http_response *response) {

   //	ASSERT(response);
   auto &resp = *response;

   detail::https_call_impl impl(*this, request, resp);

   try {
      resp.clear();
      impl.exec();
   } catch (...) {
      resp.clear();
      return false;
   }

   return true;
}

}} // namespace graphene::peerplays_sidechain

namespace graphene { namespace peerplays_sidechain {

rpc_client::rpc_client(std::string _ip, uint32_t _port, std::string _user, std::string _password, bool _debug_rpc_calls) :
      ip(_ip),
      port(_port),
      user(_user),
      password(_password),
      debug_rpc_calls(_debug_rpc_calls),
      request_id(0) {
   authorization.key = "Authorization";
   authorization.val = "Basic " + fc::base64_encode(user + ":" + password);
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

   if (reply.status == 200) {
      return ss.str();
   }

   if (json.count("error") && !json.get_child("error").empty()) {
      wlog("RPC call ${function} with body ${body} failed with reply '${msg}'", ("function", __FUNCTION__)("body", body.str())("msg", ss.str()));
   }
   return "";
}

fc::http::reply rpc_client::send_post_request(std::string body, bool show_log) {

   fc::http::reply reply;
   auto start = ip.substr(0, 6);
   boost::algorithm::to_lower(start);

   if (start == "https:") {

      auto host = ip.substr(8); // skip "https://"

      https_call call(host, port);
      http_request request("POST", "/", authorization.key + ":" + authorization.val, body);
      http_response response;

      if (call.exec(request, &response)) {
         reply.status = response.status_code;
         reply.body.resize(response.body.size());
         memcpy(&reply.body[0], &response.body[0], response.body.size());
      }

      if (show_log) {
         std::string url = ip + ":" + std::to_string(port);
         ilog("### Request URL:    ${url}", ("url", url));
         ilog("### Request:        ${body}", ("body", body));
         ilog("### Response code:  ${code}", ("code", response.status_code));
         ilog("### Response len:   ${len}", ("len", response.body.size()));
         std::stringstream ss(std::string(reply.body.begin(), reply.body.end()));
         ilog("### Response body:  ${ss}", ("ss", ss.str()));
      }

      return reply;
   }

   std::string host;

   if (start == "http:/")
      host = ip.substr(7); // skip "http://"
   else
      host = ip;

   std::string url = "http://" + host + ":" + std::to_string(port);
   fc::ip::address addr;

   try {
      addr = fc::ip::address(host);
   } catch (...) {
      try {
         addr = fc::ip::address(resolve_host_addr(host));
      } catch (...) {
         if (show_log) {
            std::string url = ip + ":" + std::to_string(port);
            ilog("### Request URL:    ${url}", ("url", url));
            ilog("### Request:        ${body}", ("body", body));
            ilog("### Request: error: host address resolve failed");
         }
         return reply;
      }
   }

   try {

      fc::http::connection conn;
      conn.connect_to(fc::ip::endpoint(addr, port));

      //if (wallet.length() > 0) {
      //   url = url + "/wallet/" + wallet;
      //}

      reply = conn.request("POST", url, body, fc::http::headers{authorization});

   } catch (...) {
   }

   if (show_log) {
      ilog("### Request URL:    ${url}", ("url", url));
      ilog("### Request:        ${body}", ("body", body));
      std::stringstream ss(std::string(reply.body.begin(), reply.body.end()));
      ilog("### Response:       ${ss}", ("ss", ss.str()));
   }

   return reply;
}

}} // namespace graphene::peerplays_sidechain
