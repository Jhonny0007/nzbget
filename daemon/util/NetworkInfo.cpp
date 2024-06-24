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


#include "nzbget.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "NetworkInfo.h"
#include "Util.h"
#include "Log.h"
#include <iostream>
#include <optional>
#include <stdexcept>

namespace SystemInfo
{
	namespace asio = boost::asio;
	namespace ssl = boost::asio::ssl;
	using tcp = boost::asio::ip::tcp;

	const char* ipService = "ip.nzbget.com";

	class HttpClient final
	{
	public:

		std::future<std::string> Get(const std::string& host, const std::string& service)
		{
			return std::async(std::launch::async, [&]
				{
					auto endpoints = m_resolver.resolve(host, service);
					boost::system::error_code ec;

					ssl::context m_sslContext{ ssl::context::tlsv13_client };
					m_sslContext.set_default_verify_paths();

					ssl::stream<tcp::socket> stream(m_context, m_sslContext);
					asio::connect(stream.lowest_layer(), endpoints, ec);

					if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
					{
						throw std::runtime_error("Failed to configure SNI TLS extension.");
					}

					stream.handshake(ssl::stream_base::handshake_type::client);

					std::string request = "GET / HTTP/1.1\r\n";
					request += "Host: " + host + "\r\n";
					request += std::string("User-Agent: nzbget/") + Util::VersionRevision() + "\r\n";
#ifndef DISABLE_GZIP
					request += "Accept-Encoding: gzip\r\n";
#endif
					request += "Connection: close\r\n\r\n";


					asio::write(stream, asio::buffer(request));

					asio::streambuf response;
					asio::read_until(stream, response, "\r\n");

					std::istream responseStream(&response);

					std::string httpVersion;
					unsigned statusCode;

					responseStream >> httpVersion;
					responseStream >> statusCode;

					if (!responseStream || httpVersion.find("HTTP/") == std::string::npos)
					{
						throw std::runtime_error(std::string("Invalid response"));
					}

					if (statusCode != 200)
					{
						throw std::runtime_error(std::string("Failed to get public and private IP. Status code: ") + std::to_string(statusCode));
					}

					asio::read_until(stream, response, "\r\n\r\n");

					std::string header;
					while (std::getline(responseStream, header) && header != "\r")
					{
						std::cout << header << std::endl;
					}

					asio::read(stream, response, ec);

					if (ec != asio::error::eof)
					{
						throw std::runtime_error(std::string("Failed to get the response body: ") + ec.message());
					}

					return std::string(response);
				}
			);
		}

	public:
		HttpClient();
		~HttpClient() = default;
	private:


		asio::io_context m_context;
		tcp::resolver m_resolver;
		ssl::context m_sslContext;
	};

	HttpClient::HttpClient()
		: m_context{}
		, m_resolver{ m_context }
		, m_sslContext{ ssl::context::tlsv13_client }
	{

	}

	NetworkInfo GetNetworkInfo()
	{
		NetworkInfo network{};

		HttpClient* hc = new HttpClient();
		auto resp = hc->Get(ipService, "https").get();

		try
		{
			asio::io_context io_context;
			tcp::resolver resolver{ io_context };

			auto endpoints = resolver.resolve(ipService, "https");

			ssl::context ctx{ ssl::context::tlsv13_client };
			ctx.set_default_verify_paths();
			boost::system::error_code ec;
			ssl::stream<tcp::socket> stream(io_context, ctx);
			asio::connect(stream.lowest_layer(), endpoints, ec);

			if (!SSL_set_tlsext_host_name(stream.native_handle(), ipService))
			{
				warn("Failed to configure SNI TLS extension.");
				return network;
			}

			stream.handshake(ssl::stream_base::handshake_type::client);

			std::string request = "GET / HTTP/1.1\r\n";
			request += std::string("Host: ") + ipService + "\r\n";
			request += std::string("User-Agent: nzbget/") + Util::VersionRevision() + "\r\n";
#ifndef DISABLE_GZIP
			request += "Accept-Encoding: gzip\r\n";
#endif
			request += "Connection: close\r\n\r\n";


			asio::write(stream, asio::buffer(request));

			asio::streambuf response;
			asio::read_until(stream, response, "\r\n");

			std::istream responseStream(&response);
			std::string httpVersion;
			unsigned statusCode;
			responseStream >> httpVersion;
			responseStream >> statusCode;

			if (statusCode != 200)
			{
				warn("Failed to get public and private IP. Status code: %u", statusCode);
			}

			asio::read_until(stream, response, "\r\n\r\n");

			std::string header;
			while (std::getline(responseStream, header) && header != "\r")
			{
				std::cout << header << std::endl;
			}

			asio::read(stream, response, ec);

			if (ec != asio::error::eof)
			{
				warn("Failed to get the response body: %s", ec.message());
				return network;
			}

			// char buffer[1024];
			// boost::system::error_code ec;
			// size_t totalSize = asio::read(stream, asio::buffer(buffer), ec);
			// if (ec != boost::asio::error::eof)
			// {
			// 	warn("Failed to get public and private IP: %s", ec.message());
			// 	return network;
			// }

			// std::string response(buffer, totalSize);
			// if (response.find("200 OK") == std::string::npos)
			// {
			// 	warn("Failed to get public and private IP: %s", buffer);
			// 	return network;
			// }

			// size_t headersEndPos = response.find("\r\n\r\n");
			// if (headersEndPos != std::string::npos)
			// {
			// 	response = response.substr(headersEndPos);
			// 	Util::Trim(response);
			// 	network.publicIP = std::move(response);
			// 	network.privateIP = stream.lowest_layer().local_endpoint().address().to_string();
			// }
		}
		catch (const std::exception& e)
		{
			warn("Failed to get public and private IP: %s", e.what());
		}

		return network;
	}
}
