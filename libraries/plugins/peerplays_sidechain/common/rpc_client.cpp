#include <graphene/peerplays_sidechain/common/rpc_client.hpp>

#include <sstream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <fc/crypto/base64.hpp>
#include <fc/log/logger.hpp>
#include <fc/network/ip.hpp>

#include <boost/algorithm/string/case_conv.hpp>

#include "https_call.h"
#include "net_utl.h"

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

		auto host = ip.substr(8);		// skip "https://"

		using namespace peerplays::net;

		HttpsCall call(host, port);
		HttpRequest request("POST", "/", authorization.key + ":" + authorization.val, body);
		HttpResponse response;

		if (call.exec(request, &response)) {
			reply.status = response.statusCode;
			reply.body.resize(response.body.size());
			memcpy(&reply.body[0], &response.body[0], response.body.size());
		}

		if (show_log) {
			std::string url = ip + ":" + std::to_string(port);
			ilog("### Request URL:    ${url}", ("url", url));
			ilog("### Request:        ${body}", ("body", body));
			ilog("### Response code:  ${code}", ("code", response.statusCode));
			ilog("### Response len:   ${len}", ("len", response.body.size()));
			std::stringstream ss(std::string(reply.body.begin(), reply.body.end()));
			ilog("### Response body:  ${ss}", ("ss", ss.str()));
		}

		return reply;

	}

	std::string host;

	if (start == "http:/")
		host = ip.substr(7);		// skip "http://"
	else
		host = ip;

	std::string url = "http://" + host + ":" + std::to_string(port);
	fc::ip::address addr;

	try {
		addr = fc::ip::address(host);
	} catch (...) {
		try {
			addr = fc::ip::address(peerplays::net::resolveHostAddr(host));
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
