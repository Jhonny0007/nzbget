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

namespace SystemInfo
{
	namespace asio = boost::asio;
	namespace ssl = boost::asio::ssl;

	NetworkInfo GetNetworkInfo()
	{
		NetworkInfo network{};

		try
		{
			asio::io_context io_context;
			asio::ip::tcp::resolver resolver(io_context);
			auto endpoints = resolver.resolve("ip.nzbget.com", "443");

			ssl::context ctx{ ssl::context::tlsv13_client };
			ctx.set_default_verify_paths();

			ssl::stream<asio::ip::tcp::socket> stream(io_context, ctx);
			asio::connect(stream.lowest_layer(), endpoints);

			if (!SSL_set_tlsext_host_name(stream.native_handle(), "ip.nzbget.com"))
			{
				warn("Failed to configure SNI TLS extension.");
				return network;
			}

			stream.handshake(ssl::stream_base::handshake_type::client);

			std::string request = "GET / HTTP/1.1\r\n";
			request += "Host: ip.nzbget.com\r\n";
			request += "User-Agent: nzbget/24.2\r\n";
#ifndef DISABLE_GZIP
			request += "Accept-Encoding: gzip\r\n";
#endif
			request += "Connection: close\r\n\r\n";

			// Send the request
			asio::write(stream, asio::buffer(request));

			char buffer[1024];
			size_t totalSize = stream.read_some(asio::buffer(buffer, 1024));

			std::string response(buffer, totalSize);
			if (response.find("200 OK") == std::string::npos)
			{
				warn("Failed to get public and private IP: %s", buffer);
				return network;
			}

			size_t headersEndPos = response.find("\r\n\r\n");
			if (headersEndPos != std::string::npos)
			{
				response = response.substr(headersEndPos);
				Util::Trim(response);
				network.publicIP = std::move(response);
				network.privateIP = stream.lowest_layer().local_endpoint().address().to_string();
			}
		}
		catch (const std::exception& e)
		{
			warn("Failed to get public and private IP: %s", e.what());
		}

		return network;
	}
}
