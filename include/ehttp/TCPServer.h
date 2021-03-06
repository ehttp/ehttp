#ifndef EHTTP_TCPSERVER_H
#define EHTTP_TCPSERVER_H

#include <asio.hpp>

#include <cstdint>
#include <string>
#include <deque>
#include <thread>
#include <functional>
#include <memory>

namespace ehttp
{
	using namespace asio;
	using namespace asio::ip;
	
	/**
	 * A simple ASIO-based TCP server.
	 * 
	 * You can use this to easily listen for incoming connections, but you are
	 * in no way required to - if you have another TCP server that works better
	 * for you, feel free to use that.
	 */
	class TCPServer
	{
	public:
		class Connection;
		
		/**
		 * Constructor.
		 * @param workers The number of worker threads to create
		 */
		TCPServer(unsigned int workers = 0);
		virtual ~TCPServer();
		
		/**
		 * Attempts to listen for incoming connections.
		 * 
		 * Note that this is one of the things that are most prone to failing,
		 * so even if you have no other error checking code, at least check
		 * the result of this.
		 * 
		 * A few examples of cases where listen() may fail:
		 * - Another application is running on the same port
		 * - Another instance of your application is already running
		 * - Your application crashed, and the OS hasn't unbound its port yet
		 * - You don't have permission to open that port
		 * - You're on Windows, and your firewall or antivirus is messing with you
		 * - The underlying implementation may be bugged on your platform
		 * - Something unrelated to your application just started malfunctioning
		 * 
		 * While you can't be expected to be held responsible for any of these,
		 * you are expected to log an error and return failure if you can't
		 * bind, rather than just silently malfunctioning.
		 * 
		 * @param endpoint The ASIO endpoint to listen on
		 */
		asio::error_code listen(const tcp::endpoint &endpoint);
		
		/**
		 * Attempts to listen for incoming connections.
		 * @overload
		 * @param address The address (0.0.0.0, 127.0.0.1, ...) to listen on
		 * @param port The port to listen on
		 */
		asio::error_code listen(const std::string &address, const uint16_t &port);
		
		/**
		 * Attempts to listen for incoming connections.
		 * @overload
		 * @param port The port to listen on
		 */
		asio::error_code listen(const uint16_t &port);
		
		/**
		 * Runs the server on the current thread.
		 * This function will not return until the server is stopped, either by
		 * calling stop() or in response to a SIGINT or SIGTERM.
		 */
		void run();
		
		/**
		 * Stops a running server.
		 * This will return immediately, but run() may take a moment before it
		 * returns, as it will wait until all currently queued events have been
		 * processed.
		 */
		void stop();
		
		/**
		 * Runs one turn of the server loop.
		 * If you have an existing run loop in your application, you can call
		 * this function as a part of it (or just once in a while, really), to
		 * neatly integrate ehttp into your application without having to give
		 * it its own thread.
		 */
		void poll();
		
		
		
		/**
		 * Callback for when a connection is established.
		 * 
		 * @param connection The newly established connection
		 */
		std::function<void(std::shared_ptr<TCPServer::Connection> connection)> onConnected;
		
		/**
		 * Callback for when a connection receives data.
		 * 
		 * @param connection The connection that received the data
		 * @param data Pointer to the response data; this is only guaranteed to be valid until this callback returns
		 * @param size Size of the response data
		 */
		std::function<void(std::shared_ptr<TCPServer::Connection> connection, const char *data, std::size_t size)> onData;
		
		/**
		 * Callback for when a connection is disconnected.
		 * 
		 * This is called regardless of if the connection was cleanly closed or
		 * if it was closed in response to an error (after #onError), so you
		 * may do any cleanup here regardless.
		 * 
		 * @param connection The newly disconnected connection
		 */
		std::function<void(std::shared_ptr<TCPServer::Connection> connection)> onDisconnected;
		
		/**
		 * Callback for when there's a problem.
		 * 
		 * Note that this is not called when there is an error that's expected
		 * and handled by the server or connection, such as EOFs.
		 * 
		 * @param error The ASIO Error code
		 */
		std::function<void(asio::error_code error)> onError;
		
	protected:
		/// Gets the server ready to accept a new connection
		virtual void accept();
		
		
		
		/// Overridable emitter for #onConnected
		virtual void eventConnected(std::shared_ptr<TCPServer::Connection> connection) {
			if(onConnected) onConnected(connection);
		}
		
		/// Overridable emitter for #onData
		virtual void eventData(std::shared_ptr<TCPServer::Connection> connection, const char *data, std::size_t size) {
			if(onData) onData(connection, data, size);
		}
		
		/// Overridable emitter for #onDisconnected
		virtual void eventDisconnected(std::shared_ptr<TCPServer::Connection> connection) {
			if(onDisconnected) onDisconnected(connection);
		}
		
		/// Overridable emitter for #onError
		virtual void eventError(asio::error_code error) {
			if(onError) onError(error);
		}
		
	private:
		struct impl;
		impl *p;
	};
	
	
	
	/**
	 * Represents a connection for \ref TCPServer.
	 * 
	 * Note that a connection will retain a shared_ptr to itself, thus
	 * preventing it from getting deleted until it has disconnected.\n
	 * When you are done with a connection, you should thus always call
	 * disconnect() on it to prevent it from just laying around.
	 */
	class TCPServer::Connection : public std::enable_shared_from_this<TCPServer::Connection>
	{
	public:
		/**
		 * Constructor.
		 * @param srv The parent server
		 * @param service The ASIO service
		 */
		Connection(TCPServer *srv, io_service &service);
		virtual ~Connection();
		
		/// Returns the connection's ASIO socket
		tcp::socket& socket();
		
		/**
		 * Writes a chunk of data to the stream.
		 * The data is copied into an internal write buffer to ensure that it
		 * stays valid.
		 */
		void write(std::vector<char> data, std::function<void(const asio::error_code &error, std::size_t bytes_transferred)> callback = nullptr);
		
		/**
		 * Disconnects from the client, and allows the connection to be deleted
		 * if there are no more references to it.
		 */
		void disconnect();
		
		
		
		/// Arbitrary, user-assigned pointer, to be used for context data
		std::shared_ptr<void> userdata;
		
		
		
		/**
		 * \private
		 * Called when the socket is connected.
		 * 
		 * For internal use only.
		 */
		void connected();
		
		/**
		 * \private
		 * Called in a loop to read data form the stream.
		 * 
		 * For internal use only.
		 */
		void readChunk();
		
		/**
		 * \private
		 * Called from a write() to write the next thing in the write queue.
		 * 
		 * For internal use only.
		 */
		void writeNext();
		
	private:
		struct impl;
		impl *p;
	};
}

#endif
