#ifndef EHTTP_HTTPSERVER_H
#define EHTTP_HTTPSERVER_H

#include "TCPServer.h"
#include "HTTPRequestParser.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPRouter.h"

#include <functional>

namespace ehttp
{
	/**
	 * A convenience class that combines TCPServer and HTTPRequestParser.
	 * 
	 * If your application does not already have an HTTP or TCP server set up,
	 * or simply does not have any special needs that would necessitate a
	 * custom implementation, you can easily drop this in to create a server
	 * that handles the most common use case out of the box: serving HTTP
	 * requests on a given port.
	 * 
	 * This also serves as an example of a custom server implementation, and as
	 * such it's more extensively documented than the rest of the library.
	 */
	class HTTPServer : public TCPServer
	{
	public:
		using TCPServer::TCPServer;
		
		/**
		 * Optional router for handling requests.
		 */
		std::shared_ptr<HTTPRouter> router;
		
		/**
		 * Callback for when a new request is received.
		 * 
		 * @param connection The connection that received the request
		 * @param req The received request
		 * @param res A response object that writes back to the connection
		 */
		std::function<void(std::shared_ptr<HTTPServer::Connection> connection, std::shared_ptr<HTTPRequest> req, std::shared_ptr<HTTPResponse> res)> onRequest;
		
	protected:
		/// Overridable emitter for #onRequest
		virtual void eventRequest(std::shared_ptr<HTTPServer::Connection> connection, std::shared_ptr<HTTPRequest> req, std::shared_ptr<HTTPResponse> res) {
			if(onRequest) onRequest(connection, req, res);
		}
		
		virtual void eventConnected(std::shared_ptr<HTTPServer::Connection> connection) override;
		virtual void eventDisconnected(std::shared_ptr<HTTPServer::Connection> connection) override;
		virtual void eventData(std::shared_ptr<HTTPServer::Connection> connection, const char *data, std::size_t size) override;
		virtual void eventError(asio::error_code error) override;
		
		/// \private Struct for keeping context data
		struct context {
			HTTPRequestParser parser;	///< A parser for the connection
		};
		/// Map between connections and contexts
		std::map<std::shared_ptr<HTTPServer::Connection>, context> contexts;
	};
}

#endif
