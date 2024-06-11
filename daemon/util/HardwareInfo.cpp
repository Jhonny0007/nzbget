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

#include <iostream>
#include <regex>
#include "HardwareInfo.h"
#include "Options.h"
#include "ArticleWriter.h"

namespace HardwareInfo
{
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif
	using namespace std::chrono;
	using namespace std::literals;
	using namespace boost;

#ifdef WIN32
	const char* FIND_CMD = "where ";
#else
	const char* FIND_CMD = "readlink -f ";
#endif
	const size_t BUFFER_SIZE = 256;

	UnpackerVersionParser UnpackerVersionParserFunc = [](const std::string& line) {
		// 7-Zip (a) 19.00 (x64) : Copyright (c) 1999-2018 Igor Pavlov : 2019-02-21
		// UNRAR 5.70 x64 freeware      Copyright (c) 1993-2019 Alexander Roshal
		std::regex pattern(R"([0-9]*\.[0-9]*)"); // float number
		std::smatch match;
		if (std::regex_search(line, match, pattern))
		{
			return match[0].str();
		}

		return std::string("");
		};

	HardwareInfo::HardwareInfo()
		: m_context{}
		, m_resolver{ m_context }
		, m_socket{ m_context }
		, m_networkTimePoint{ system_clock::now() }
	{
		InitCPU();
		InitOS();
		InitLibsVersions();
		auto os = GetOS();
		std::cout << "OS name: " << os.name << std::endl;
		std::cout << "OS Version: " << os.version << std::endl;
		auto cpu = GetCPU();
		std::cout << "CPU Model: " << cpu.model << std::endl;
		std::cout << "CPU arch: " << cpu.arch << std::endl;
		auto diskState = GetDiskState();
		std::cout << "Available HD: " << diskState.freeSpace << std::endl;
		std::cout << "Total HD: " << diskState.totalSize << std::endl;
		std::cout << "Cache HD: " << diskState.articleCache << std::endl;
		auto network = GetNetwork();
		std::cout << "Public IP: " << network.publicIP << std::endl;
		std::cout << "Private IP: " << network.privateIP << std::endl;
		auto env = GetEnvironment();
		std::cout << "Conf. path: " << env.configPath << std::endl;
		std::cout << "ControlIP IP: " << env.controlIP << std::endl;
		std::cout << "ControlIP Port: " << env.controlPort << std::endl;
		std::cout << "Unrar: " << env.unrar.path << std::endl;
		std::cout << "Unrar version: " << env.unrar.version << std::endl;
		std::cout << "SevenZip: " << env.sevenZip.path << std::endl;
		std::cout << "SevenZip version: " << env.sevenZip.version << std::endl;
		std::cout << "Python path: " << env.python.path << std::endl;
		std::cout << "Python version: " << env.python.version << std::endl;
		std::cout << "OPENSSL: " << GetOpenSSLVersion() << std::endl;
		std::cout << "GNUTLS: " << GetGnuTLSVersion() << std::endl;
		std::cout << "Zlib: " << GetZLibVersion() << std::endl;
		std::cout << "Curses lib: " << GetCursesVersion() << std::endl;
		std::cout << "xml2 lib: " << GetLibXml2Version() << std::endl;
		GetNetwork();
	}

	HardwareInfo::~HardwareInfo()
	{
		if (m_socket.is_open())
		{
			m_socket.close();
		}
	}

	const std::string& HardwareInfo::GetLibXml2Version() const
	{
		return m_libXml2Version;
	}

	const std::string& HardwareInfo::GetCursesVersion() const
	{
		return m_cursesVersion;
	}

	const std::string& HardwareInfo::GetZLibVersion() const
	{
		return m_zLibVersion;
	}

	const std::string& HardwareInfo::GetOpenSSLVersion() const
	{
		return m_openSSLVersion;
	}

	const std::string& HardwareInfo::GetGnuTLSVersion() const
	{
		return m_gnuTLSLVersion;
	}

	void HardwareInfo::InitLibsVersions()
	{
		m_libXml2Version = LIBXML_DOTTED_VERSION;
#ifdef HAVE_NCURSES_H
		m_cursesVersion = NCURSES_VERSION;
#else
		m_cursesVersion = "Not used";
#endif
#ifndef DISABLE_GZIP
		m_zLibVersion = ZLIB_VERSION;
#else
		m_zLibVersion = "Not used";
#endif
#ifdef HAVE_OPENSSL
		m_openSSLVersion = OPENSSL_FULL_VERSION_STR;
#else
		m_openSSLVersion = "Not used";
#endif
#ifdef HAVE_LIBGNUTLS
		m_gnuTLSLVersion = GNUTLS_VERSION;
#else
		m_gnuTLSLVersion = "Not used";
#endif
	}

	const CPU& HardwareInfo::GetCPU() const
	{
		return m_cpu;
	}

	const OS& HardwareInfo::GetOS() const
	{
		return m_os;
	}

	Environment HardwareInfo::GetEnvironment() const
	{
		Environment env;

		env.configPath = g_Options->GetConfigFilename();
		env.controlIP = g_Options->GetControlIp();
		env.controlPort = g_Options->GetControlPort();
		env.python = GetPython();
		env.unrar = GetUnrar();
		env.sevenZip = GetSevenZip();

		return env;
	}

	Tool HardwareInfo::GetPython() const
	{
		Tool python;
		python.name = "Python";

		auto result = Util::FindPython();
		if (!result.has_value())
		{
			return python;
		}

		std::string cmd = result.get() + " --version";
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe)
		{
			return python;
		}

		char buffer[BUFFER_SIZE];
		if (!feof(pipe) && fgets(buffer, BUFFER_SIZE, pipe) != nullptr)
		{
			python.version = buffer;
			Util::Trim(python.version);
		}

		pclose(pipe);

		cmd = FIND_CMD + result.get();
		pipe = popen(cmd.c_str(), "r");
		if (!pipe)
		{
			return python;
		}

		if (!feof(pipe) && fgets(buffer, BUFFER_SIZE, pipe) != nullptr)
		{
			python.path = buffer;
			Util::Trim(python.path);
		}

		pclose(pipe);

		return python;
	}

	Tool HardwareInfo::GetUnrar() const
	{
		Tool tool;

		tool.name = "UnRAR";
		tool.path = GetUnpackerPath(g_Options->GetUnrarCmd());
		tool.version = GetUnpackerVersion(tool.path, "UNRAR", UnpackerVersionParserFunc);

		return tool;
	}

	Tool HardwareInfo::GetSevenZip() const
	{
		Tool tool;

		tool.name = "7-Zip";
		tool.path = GetUnpackerPath(g_Options->GetSevenZipCmd());
		tool.version = GetUnpackerVersion(tool.path, tool.name.c_str(), UnpackerVersionParserFunc);

		return tool;
	}

	std::string HardwareInfo::GetUnpackerPath(const char* unpackerCmd) const
	{
		if (Util::EmptyStr(unpackerCmd))
		{
			return "";
		}

		std::string path = unpackerCmd;
		Util::Trim(path);

		// getting the path itself without any keys
		path = path.substr(0, path.find(" "));

		auto result = FileSystem::GetRealPath(path);

		if (result.has_value())
		{
			return result.get();
		}

		return path;
	}

	std::string HardwareInfo::GetUnpackerVersion(const std::string& path, const char* marker, const UnpackerVersionParser& parseVersion) const
	{
		FILE* pipe = popen(path.c_str(), "r");
		if (!pipe)
		{
			return "";
		}

		std::string version;
		char buffer[BUFFER_SIZE];
		while (!feof(pipe))
		{
			if (fgets(buffer, BUFFER_SIZE, pipe) != nullptr)
			{
				if (strstr(buffer, marker))
				{
					version = parseVersion(buffer);
					break;
				}
			}
		}

		pclose(pipe);

		return version;
	}

	const Network& HardwareInfo::GetNetwork()
	{
		auto now = system_clock::now();
		if (!m_network.privateIP.empty() && !m_network.publicIP.empty() && (now - m_networkTimePoint) < 2h)
		{
			return m_network;
		}

		m_networkTimePoint = std::move(now);

		try {
			asio::connect(m_socket, m_resolver.resolve("icanhazip.com", "http"));

			std::string request = "GET / HTTP/1.1\r\nHost: icanhazip.com\r\n\r\n";
			asio::write(m_socket, asio::buffer(request));

			char buffer[1024];
			size_t totalSize = m_socket.read_some(asio::buffer(buffer, 1024));

			std::string response(buffer, totalSize);
			if (response.find("200 OK") == std::string::npos)
			{
				debug("Failed to get public and private IP: %s", buffer);
				return m_network;
			}

			size_t headersEndPos = response.find("\r\n\r\n");
			if (headersEndPos != std::string::npos)
			{
				response = response.substr(headersEndPos);
				Util::Trim(response);
				m_network.publicIP = std::move(response);
				m_network.privateIP = m_socket.local_endpoint().address().to_string();
			}
		}
		catch (std::exception& e)
		{
			debug("Failed to get public and private IP: %s", e.what());
		}

		return m_network;
	}

#ifdef WIN32
	void HardwareInfo::InitCPU()
	{
		int len = BUFFER_SIZE;
		char cpuModelBuffer[BUFFER_SIZE];
		char cpuArchBuffer[BUFFER_SIZE];
		if (Util::RegReadStr(
			HKEY_LOCAL_MACHINE,
			"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
			"ProcessorNameString",
			cpuModelBuffer,
			&len))
		{
			m_cpu.model = cpuModelBuffer;
			Util::Trim(m_cpu.model);
		}
		else
		{
			debug("Failed to get CPU model. Couldn't read the Windows Registry.");
			m_cpu.model = "Unknown";
		}

		if (Util::RegReadStr(
			HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
			"PROCESSOR_ARCHITECTURE",
			cpuArchBuffer,
			&len))
		{
			m_cpu.arch = cpuArchBuffer;
			Util::Trim(m_cpu.arch);
		}
		else
		{
			debug("Failed to get CPU arch. Couldn't read the Windows Registry.");
			m_cpu.arch = "Unknown";
		}
	}

	void HardwareInfo::InitOS()
	{
		int len = BUFFER_SIZE;
		char buffer[BUFFER_SIZE];
		if (Util::RegReadStr(
			HKEY_LOCAL_MACHINE,
			"SOFTWARE\\MICROSOFT\\Windows NT\\CurrentVersion",
			"CurrentBuild",
			buffer,
			&len))
		{
			long buildNum = std::atol(buffer);
			if (buildNum == 0)
			{
				m_os.version = "Unknown";
			}
			else if (buildNum >= m_win11BuildVersion)
			{
				m_os.version = "11";
			}
			else if (buildNum >= m_win10BuildVersion)
			{
				m_os.version = "10";
			}
			else if (buildNum >= m_win8BuildVersion)
			{
				m_os.version = "8";
			}
			else if (buildNum >= m_winXPBuildVersion)
			{
				m_os.version = "XP";
			}
		}
		else
		{
			debug("Failed to get OS version. Couldn't read the Windows Registry.");
			m_os.version = "Unknown";
		}

		m_os.name = "Windows";
	}

	DiskState HardwareInfo::GetDiskState(const char* root) const
	{
		DiskState diskState;

		ULARGE_INTEGER freeBytesAvailable;
		ULARGE_INTEGER totalNumberOfBytes;

		int64 articleCache = g_ArticleCache->GetAllocated();
		uint32 articleCacheHi, articleCacheLo;
		Util::SplitInt64(articleCache, &articleCacheHi, &articleCacheLo);

		if (GetDiskFreeSpaceEx(root, &freeBytesAvailable, &totalNumberOfBytes, nullptr))
		{
			diskState.freeSpace = freeBytesAvailable.QuadPart;
			diskState.totalSize = totalNumberOfBytes.QuadPart;
			diskState.articleCache = static_cast<size_t>(articleCache);

			return diskState;
		}

		return diskState;
	}
#endif

#ifdef __linux__
#include <fstream>
	void HardwareInfo::InitCPU()
	{
		m_cpu.model = "Unknown";
		m_cpu.arch = GetCPUArch();

		std::ifstream cpuinfo("/proc/cpuinfo");
		if (!cpuinfo.is_open())
		{
			debug("Failed to read CPU model. Couldn't read '/proc/cpuinfo'.");
			return;
		}

		std::string line;
		while (std::getline(cpuinfo, line))
		{
			if (line.find("model name") != std::string::npos)
			{
				m_cpu.model = line.substr(line.find(":") + 2);
				Util::Trim(m_cpu.model);
				break;
			}
		}
	}

	void HardwareInfo::InitOS()
	{
		m_os.name = "Unknown";
		m_os.version = "Unknown";

		std::ifstream osInfo("/etc/os-release");
		if (!osInfo.is_open())
		{
			debug("Faield to get OS info. Couldn't read '/etc/os-release'.");
			return;
		}

		std::string line;
		while (std::getline(osInfo, line))
		{
			if (!m_os.name.empty() && !m_os.version.empty())
			{
				break;
			}

			if (m_os.name.empty() && line.find("NAME=") == 0)
			{
				m_os.name = line.substr(line.find("=") + 1);
				Util::Trim(m_os.name);
				continue;
			}

			if (m_os.version.empty() && line.find("VERSION_ID=") == 0)
			{
				m_os.version = line.substr(line.find("=") + 1);
				Util::Trim(m_os.version);
				continue;
			}

			if (m_os.version.empty() && line.find("BUILD_ID=") == 0)
			{
				m_os.version = line.substr(line.find("="));
				Util::Trim(m_os.version);
				continue;
			}
		}
	}
#endif

#if defined(__unix__) && !defined(__linux__)
	void HardwareInfo::InitCPU()
	{
		size_t len = BUFFER_SIZE;
		char cpuModel[BUFFER_SIZE];
		if (sysctlbyname("hw.model", &cpuModel, &len, nullptr, 0) == 0)
		{
			m_cpu.model = cpuModel;
			Util::Trim(m_cpu.model);
		}
		else
		{
			debug("Failed to get CPU model. Couldn't read 'hw.model'.");
			m_cpu.model = "Unknown";
		}

		cpu.arch = GetCPUArch();
	}

	void HardwareInfo::InitOS()
	{
		size_t len = BUFFER_SIZE;
		char buffer[len];
		if (sysctlbyname("kern.ostype", &buffer, &len, nullptr, 0) == 0)
		{
			m_os.name = buffer;
			Util::Trim(m_os.name);
		}
		else
		{
			debug("Failed to get OS name. Couldn't read 'kern.ostype'.");
			m_os.name = "Unknown";
		}

		len = BUFFER_SIZE;

		if (sysctlbyname("kern.osrelease", &buffer, &len, nullptr, 0) == 0)
		{
			m_os.version = buffer;
			Util::Trim(os.version);
		}
		else
		{
			debug("Failed to get OS version. Failed to read 'kern.osrelease'.");
			m_os.version = "Unknown";
		}
	}
#endif

#ifdef __APPLE__
	void HardwareInfo::InitCPU() const
	{
		size_t len = BUFFER_SIZE;
		char buffer[BUFFER_SIZE];
		if (sysctlbyname("machdep.cpu.brand_string", &buffer, &len, nullptr, 0) == 0)
		{
			m_cpu.model = buffer;
			Util::Trim(m_cpu.model);
		}
		else
		{
			debug("Failed to get CPU model. Couldn't read 'machdep.cpu.brand_string'.");
			m_cpu.model = "Unknown";
		}

		m_cpu.arch = GetCPUArch();
	}

	void HardwareInfo::InitOS()
	{
		m_os.name = "Unknown";
		m_os.version = "Unknown";

		FILE* pipe = popen("sw_vers", "r");
		if (!pipe)
		{
			debug("Failed to get OS info. Couldn't read 'sw_vers'.");
			return;
		}

		char buffer[BUFFER_SIZE];
		std::string result = "";
		while (!feof(pipe))
		{
			if (fgets(buffer, sizeof(buffer), pipe))
			{
				result += buffer;
			}
		}

		pclose(pipe);

		std::string productName = "ProductName:";
		size_t pos = result.find(productName);
		if (pos != std::string::npos)
		{
			size_t endPos = result.find("\n", pos);
			m_os.name = result.substr(pos + productName.size(), endPos - pos - productName.size());
			Util::Trim(m_os.name);
		}

		std::string productVersion = "ProductVersion:";
		pos = result.find(productVersion);
		if (pos != std::string::npos)
		{
			size_t endPos = result.find("\n", pos);
			m_os.version = result.substr(pos + productVersion.size(), endPos - pos - productVersion.size());
			Util::Trim(os.version);
		}
	}
#endif

#ifndef WIN32
	std::string HardwareInfo::GetCPUArch() const
	{
		const char* cmd = "uname -m";
		FILE* pipe = popen(cmd, "r");
		if (!pipe)
		{
			debug("Failed to get CPU arch. Couldn't read 'uname -m'.");
			return "Unknown";
		}

		char buffer[BUFFER_SIZE];
		while (!feof(pipe))
		{
			if (fgets(buffer, BUFFER_SIZE, pipe))
			{
				pclose(pipe);
				Util::Trim(buffer);
				return buffer;
			}
		}

		pclose(pipe);

		debug("Failed to find CPU arch.");

		return "Unknown";
	}

	DiskState HardwareInfo::GetDiskState(const char* root) const
	{
		int64 articleCache = g_ArticleCache->GetAllocated();
		uint32 articleCacheHi, articleCacheLo;
		Util::SplitInt64(articleCache, &articleCacheHi, &articleCacheLo);
		struct statvfs diskdata;

		if (statvfs(root, &diskdata) == 0)
		{
			size_t available = diskdata.f_bfree * diskdata.f_frsize;
			size_t total = diskdata.f_blocks * diskdata.f_frsize;
			return { available, total, static_cast<size_t>(articleCache) };
		}

		return { 0, 0, static_cast<size_t>(articleCache) };
	}
#endif

}
