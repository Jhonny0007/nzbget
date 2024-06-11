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

#ifndef HARDWARE_INFO_H
#define HARDWARE_INFO_H

#include <string>
#include <iostream>
#include <chrono>
#include <boost/asio.hpp>
#include "Util.h"

namespace HardwareInfo
{
	using UnpackerVersionParser = std::function<std::string(const std::string&)>;

	struct CPU
	{
		std::string model;
		std::string arch;
	};

	struct Tool
	{
		std::string name;
		std::string version;
		std::string path;
	};

	struct Environment
	{
		Tool python;
		Tool sevenZip;
		Tool unrar;
		std::string configPath;
		std::string controlIP;
		int controlPort;
	};

	struct Network
	{
		std::string publicIP;
		std::string privateIP;
	};

	struct DiskState
	{
		size_t freeSpace;
		size_t totalSize;
		size_t articleCache;
	};

	struct OS
	{
		std::string name;
		std::string version;
	};

	class HardwareInfo final
	{
	public:
		HardwareInfo();
		~HardwareInfo();
		Environment GetEnvironment() const;
		const CPU& GetCPU() const;
		const Network& GetNetwork();
		const DiskState& GetDiskState(const char* root = ".")&;
		const OS& GetOS() const;
		const std::string& GetOpenSSLVersion() const;
		const std::string& GetGnuTLSVersion() const;
		const std::string& GetZLibVersion() const;
		const std::string& GetCursesVersion() const;
		const std::string& GetLibXml2Version() const;

	private:
		Tool GetPython() const;
		Tool GetSevenZip() const;
		Tool GetUnrar() const;
		void InitCPU();
		void InitOS();
		void InitLibrariesVersions();
		std::string GetUnpackerPath(const char* unpackerCmd) const;
		std::string GetUnpackerVersion(const std::string& path, const char* marker, const UnpackerVersionParser& parser) const;
		boost::asio::io_context m_context;
		boost::asio::ip::tcp::resolver m_resolver;
		boost::asio::ip::tcp::socket m_socket;
		std::chrono::time_point<std::chrono::system_clock> m_networkTimePoint;
		std::chrono::time_point<std::chrono::system_clock> m_diskStateTimePoint;
		Network m_network;
		DiskState m_diskState;
		CPU m_cpu;
		OS m_os;
		std::string m_openSSLVersion;
		std::string m_gnuTLSLVersion;
		std::string m_zLibVersion;
		std::string m_cursesVersion;
		std::string m_libXml2Version;

#ifndef WIN32
		std::string GetCPUArch() const;
#endif

#ifdef WIN32
		const long m_win11BuildVersion = 22000;
		const long m_win10BuildVersion = 10240;
		const long m_win8BuildVersion = 9200;
		const long m_win7BuildVersion = 7600;
		const long m_winXPBuildVersion = 2600;
#endif
	};
}

#endif
