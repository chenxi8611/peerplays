#include "https_call.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/buffer.hpp>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>

//#include <iostream>

namespace peerplays {
namespace net {

namespace detail {

static const char Cr = 0x0D;
static const char Lf = 0x0A;
static const char * CrLf = "\x0D\x0A";
static const char * CrLfCrLf = "\x0D\x0A\x0D\x0A";

using namespace boost::asio;

[[noreturn]] static void throwError(const std::string & msg) {
	throw std::runtime_error(msg);
}

class Impl {
public:

	Impl(const HttpsCall & call, const HttpRequest & request, HttpResponse & response) : 
		m_Call(call),
		m_Request(request),
		m_Response(response),
		m_Service(),
		m_Context(ssl::context::tlsv12_client),
		m_Socket(m_Service, m_Context),
		m_Endpoint(),
		m_ResponseBuf(call.responseSizeLimitBytes()),
		m_ContentLength(0)
	{
		m_Context.set_default_verify_paths();
	}

	void exec() {
		resolve();
		connect();
		sendRequest();
		processResponse();
	}

private:

	const HttpsCall & m_Call;
	const HttpRequest & m_Request;
	HttpResponse & m_Response;

    io_service m_Service;
    ssl::context m_Context;
	ssl::stream<ip::tcp::socket> m_Socket;
	ip::tcp::endpoint m_Endpoint;
	streambuf m_ResponseBuf;
	uint32_t m_ContentLength;

	void resolve() {

		// resolve TCP endpoint for host name

	    ip::tcp::resolver resolver(m_Service);
		auto query = ip::tcp::resolver::query(m_Call.host(), "https");
		auto iter = resolver.resolve(query);
		m_Endpoint = *iter;

		if (m_Call.port() != 0)										// if port was specified
			m_Endpoint.port(m_Call.port());							// force set port 

	}

	void connect() {

		// TCP connect 

		m_Socket.lowest_layer().connect(m_Endpoint);

		// SSL connect

		m_Socket.set_verify_mode(ssl::verify_peer);
		m_Socket.handshake(ssl::stream_base::client);
	}

	void sendRequest() {

		streambuf requestBuf;
		std::ostream stream(&requestBuf);

		// start string: method path HTTP/1.0

        stream << m_Request.method << " " << m_Request.path << " HTTP/1.0" << CrLf;

		// host

		stream << "Host: " << m_Call.host() << ":" << m_Endpoint.port() << CrLf;

		// content

		if (!m_Request.body.empty()) {
			stream << "Content-Type: " << m_Request.contentType << CrLf;
			stream << "Content-Length: " << m_Request.body.size() << CrLf;
		}

		// additional headers

		const auto & h = m_Request.headers;

		if (!h.empty()) {
			if (h.size() < 2)
				throw 1;
			stream << h;
			if ((h.substr(h.size() - 2) != CrLf))
				stream << CrLf;
		}

		// other

		stream << "Accept: *\x2F*" << CrLf;
		stream << "Connection: close" << CrLf;

		// end

		stream << CrLf;

		// content

		if (!m_Request.body.empty())
			stream << m_Request.body;

		// send

		write(m_Socket, requestBuf);
	}

	void helper1() {

		std::istream stream(&m_ResponseBuf);

		std::string httpVersion;
		stream >> httpVersion;
		stream >> m_Response.statusCode;
					  
		if (!stream || httpVersion.substr(0, 5) != "HTTP/") {
			throw "invalid response";
		}

		// read/skip headers

		for(;;) {
			std::string header;
			if (!std::getline(stream, header, Lf) || (header.size() == 1 && header[0] == Cr))
				break;
			if (m_ContentLength)							// if content length is already known
				continue;									// continue skipping headers
			auto pos = header.find(':');
			if (pos == std::string::npos)
				continue;
			auto name = header.substr(0, pos);
			boost::algorithm::trim(name);
			boost::algorithm::to_lower(name);
			if (name !=	"content-length")
				continue;
			auto value = header.substr(pos + 1);
			boost::algorithm::trim(value);
			m_ContentLength = std::stol(value);
		}

	}

	void processResponse() {

		auto & socket = m_Socket;
		auto & buf = m_ResponseBuf;
		auto & contentLength = m_ContentLength;
		auto & body = m_Response.body;

		read_until(socket, buf, CrLfCrLf);

		helper1();

		if (contentLength < 2)	// minimum content is "{}"
			throwError("invalid response body (too short)");

		if (contentLength > m_Call.responseSizeLimitBytes())
			throwError("response body size limit exceeded");

		auto avail = buf.size();

		if (avail > contentLength)
			throwError("invalid response body (content length mismatch)");

		body.resize(contentLength);

		if (avail) {
			if (avail != buf.sgetn(&body[0], avail)) {
				throwError("stream read failed");
			}
		}

		auto rest = contentLength - avail;

		boost::system::error_code errorCode;

		read(socket, buffer(&body[avail], rest), errorCode);

		socket.shutdown(errorCode);

	}

};

} // detail

// HttpsCall

HttpsCall::HttpsCall(const std::string & host, uint16_t port) : 
	m_Host(host),
	m_Port(port)
{}

bool HttpsCall::exec(const HttpRequest & request, HttpResponse * response) {

//	ASSERT(response);
	auto & resp = *response;

	detail::Impl impl(*this, request, resp);

	try {
		resp.clear();
		impl.exec();
	} catch (...) {
		resp.clear();
		return false;
	}

	return true;
}

} // net
} // peerplays
