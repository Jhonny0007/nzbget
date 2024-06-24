/*
 *  This file is part of nzbget. See <https://nzbget.com>.
 *
 *  Copyright (C) 2024 Denis <denis@nzbget.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <thread>
#include <boost/asio.hpp>

#ifndef DISABLE_TLS
#include <boost/asio/ssl.hpp>
#endif

namespace HttpClient
{
#ifndef DISABLE_TLS
	using Socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
#else
	using Socket = boost::asio::ip::tcp::socket;
#endif

	using Headers = std::unordered_map<std::string, std::string>;
	using Endpoints = boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>;

	class HttpClient final
	{
	public:
		struct Response
		{
			Headers headers;
			std::string body;
			unsigned statusCode;
		};

		std::future<Response> GET(const std::string& host, const std::string& service);

		HttpClient();
		~HttpClient() = default;
	private:
		void Connect(Socket& socket, const Endpoints& endpoints);
		void Write(Socket& socket, const std::string& method, const std::string& host);
		std::string GetHeaders(const std::string& method, const std::string& host) const;
		unsigned ReadStatusCode(Socket& socket, boost::asio::streambuf& buf);
		Headers ReadHeaders(Socket& socket, boost::asio::streambuf& buf);
		std::string ReadBody(Socket& socket, boost::asio::streambuf& buf);

#ifndef DISABLE_TLS
		Socket GetSocket();

		void DoHandshake(Socket& socket, const std::string& host);

		boost::asio::ssl::context m_sslContext;
#else
		Socket GetSocket();
#endif
		boost::asio::io_context m_context;
		boost::asio::ip::tcp::resolver m_resolver;

	};
}

#endif
