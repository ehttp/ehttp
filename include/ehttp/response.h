#ifndef EHTTP_RESPONSE_H
#define EHTTP_RESPONSE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "request.h"
#include "util.h"

namespace ehttp
{
	/**
	 * Represents an HTTP Response.
	 * 
	 * Unlike \ref request, response is more of a generator than a simple
	 * container. The reason for this is obviously that our main order of
	 * business here is generating responses, rather than requests.
	 * 
	 * @todo Generate the Date header.
	 */
	class response : public std::enable_shared_from_this<response>
	{
	public:
		class chunk;
		
		/**
		 * Constructor
		 * @param req The request we're responding to, for context
		 */
		response(std::shared_ptr<request> req = 0);
		virtual ~response();
		
		/**
		 * Begins a response with the given status code, and optionally a
		 * custom reason string. If a custom reason is not given, the standard
		 * reason phrase associated with the response code ("OK" for 200, "Not
		 * Found" for 404, "Internal Server Error" for 500, etc.) is used.
		 * 
		 * @todo Let begin(0) reuse already set code and reason, but clear
		 * headers, to let you write nicer looking status handlers without
		 * clobbering data (possibly setting the wrong status code by accident)
		 * in the name of symmetry. That'd be pretty dumb.
		 */
		std::shared_ptr<response> begin(uint16_t code = 200, std::string custom_reason = "");
		
		/** 
		 * Sets a header.
		 * Headers can be modified freely until end() is called.
		 * @throws std::logic_error if end() has already been called.
		 */
		std::shared_ptr<response> header(std::string name, std::string value);
		
		/**
		 * Appends some data to the response body. Writes a chunk if end() has
		 * already been called and the response is chunked.
		 * 
		 * @throws std::logic_error if end() has already been called, and the
		 * response is not chunked.
		 */
		std::shared_ptr<response> write(const std::vector<char> &data);
		/// @overload
		std::shared_ptr<response> write(const std::string &data);
		
		/**
		 * Finalizes the response and calls #on_end.
		 * If chunked is true, the 'Transfer-Encoding' header will be set to
		 * 'chunked', and you should use begin_chunk() to write chunks to the
		 * stream, ending with a call to end_chunked().
		 * @throws std::runtime_error if #on_end isn't set.
		 */
		void end();
		
		/**
		 * Begins a chunk.
		 * Use chunk::end() to end it and write it out.\n
		 * There is no reference counting for this or anything - if you realize
		 * after you've begun a chunk that you don't actually need to send it,
		 * just don't call chunk::end() on it.
		 * @throws std::logic_error if the response isn't chunked.
		 */
		std::shared_ptr<chunk> begin_chunk();
		
		/**
		 * Makes the response chunked.
		 * If there is data in #body, it's written out as a chunk.
		 * 
		 * You normally don't have to call this yourself, as it will be called
		 * automatically by chunk::end(). It's mostly exposed for debugging.
		 */
		std::shared_ptr<response> make_chunked();
		
		
		
		/// The request we're responding to, for context
		std::shared_ptr<request> req;
		
		/// HTTP Status Code (eg. 200, 404, 500, ...)
		uint16_t code;
		/// Reason phrase for the status
		std::string reason;
		/// Response headers
		std::map<std::string,std::string,util::ci_less> headers;
		/// Response body for non-chunked transfers
		std::vector<char> body;
		
		/**
		 * Returns the response formatted according to the HTTP specification.
		 * For chunked responses, only the header will be formatted - chunks
		 * are responsible for formatting themselves.
		 */
		std::vector<char> to_http(bool headers_only = false);
		
		
		
		/// Callback for make_chunked() or end()
		std::function<void(std::shared_ptr<response> res, std::vector<char> data)> on_head;
		/// Callback for end() for non-chunked responses
		std::function<void(std::shared_ptr<response> res, std::vector<char> data)> on_body;
		/// Callback for chunk::end()
		std::function<void(std::shared_ptr<response> res, std::shared_ptr<response::chunk> chunk, std::vector<char> data)> on_chunk;
		/// Callback for end()
		std::function<void(std::shared_ptr<response> res)> on_end;
		
	private:
		struct impl;
		impl *p;
	};
	
	
	
	/**
	 * Represents a chunk in a \ref response.
	 */
	class response::chunk : public std::enable_shared_from_this<response::chunk>
	{
	public:
		/**
		 * \private (to remove it from Doxygen's output)
		 * Constructor, typically not called directly.
		 * Instead, you should use response::begin_chunk() to create chunks.
		 * This is exposed mainly for unit testing purposes.
		 * @param res The response the chunk is part of
		 */
		chunk(std::shared_ptr<response> res);
		virtual ~chunk();
		
		/**
		 * Appends data to the chunk body.
		 * \todo Make this throw an exception just like response::write() when
		 * attempting to modify an ended chunk.
		 */
		std::shared_ptr<response::chunk> write(const std::vector<char> &data);
		/// @overload
		std::shared_ptr<response::chunk> write(const std::string &data);
		
		/**
		 * Ends the response. Calls #res->on_chunk.
		 * @throws std::runtime_error if #res->on_chunk isn't set.
		 */
		std::shared_ptr<response> end();
		
		
		
		/// The response the chunk is a part of
		std::shared_ptr<response> res;
		/// The chunk body
		std::vector<char> body;
		
		/// Returns the chunk formatted according to the HTTP specification.
		std::vector<char> to_http();
	};
}

#endif
