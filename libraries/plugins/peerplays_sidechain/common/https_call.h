#pragma once

#include <string>

namespace peerplays {
namespace net {

struct HttpRequest {

	std::string method;			// ex: "POST"
	std::string path;			// ex: "/"
	std::string headers;
	std::string body;
	std::string contentType;	// ex: "application/json"

	HttpRequest() {}

	HttpRequest(const std::string & method_, const std::string & path_, const std::string & headers_, const std::string & body_, const std::string & contentType_) :
		method(method_),
		path(path_),
		headers(headers_),
		body(body_),
		contentType(contentType_)
	{}

	HttpRequest(const std::string & method_, const std::string & path_, const std::string & headers_, const std::string & body_ = std::string()) :
		method(method_),
		path(path_),
		headers(headers_),
		body(body_),
		contentType("application/json")
	{}

	void clear() {
		method.clear();
		path.clear();
		headers.clear();
		body.clear();
		contentType.clear();
	}

};

struct HttpResponse {

	uint16_t statusCode;
	std::string body;

	void clear() {
		statusCode = 0;
		body = decltype(body)();
	}

};

class HttpsCall {
public:

	static constexpr auto ResponseSizeLimitBytes = 1024 * 1024;

	HttpsCall(const std::string & host, uint16_t port = 0);

	bool exec(const HttpRequest & request, HttpResponse * response); 

	const std::string host() const {
		return m_Host;
	}

	uint16_t port() const {
		return m_Port;
	}

	uint32_t responseSizeLimitBytes() const {
		return ResponseSizeLimitBytes;
	}

private:
	std::string m_Host;
	uint16_t m_Port;
};


} // net
} // peerplays
