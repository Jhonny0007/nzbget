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

#include <regex>

#include "SystemInfo.h"
#include "Options.h"
#include "FileSystem.h"
#include "Log.h"
#include "Json.h"
#include "Xml.h"

#ifdef WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace SystemInfo
{
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

	using namespace boost;

#ifdef WIN32
	const char* FIND_CMD = "where ";
#else
	const char* FIND_CMD = "which ";
#endif

	const size_t BUFFER_SIZE = 256;

	UnpackerVersionParser UnpackerVersionParserFunc = [](const std::string& line) {
		// e.g. 7-Zip (a) 19.00 (x64) : Copyright (c) 1999-2018 Igor Pavlov : 2019-02-21
		// e.g. UNRAR 5.70 x64 freeware      Copyright (c) 1993-2019 Alexander Roshal
		std::regex pattern(R"([0-9]*\.[0-9]*)"); // float number
		std::smatch match;
		if (std::regex_search(line, match, pattern))
		{
			return match[0].str();
		}

		return std::string("");
		};

	SystemInfo::SystemInfo()
		: m_context{}
		, m_resolver{ m_context }
		, m_socket{ m_context }
	{
		InitCPU();
		InitOS();
		InitLibsVersions();

		int cpuinfo[4]; // Array to store CPUID results

		__cpuid(cpuinfo, 1);

		uint32_t eax = cpuinfo[0];
		uint32_t ebx = cpuinfo[1];
		uint32_t ecx = cpuinfo[2];
		uint32_t edx = cpuinfo[3];

		// Detect MMX
		bool has_mmx = (edx & (1 << 23)) != 0;

		// Detect MMX-Extended (not a separate feature, implied by MMX)

		// Detect SSE2
		bool has_sse2 = (edx & (1 << 26)) != 0;

		// Detect SSE3
		bool has_sse3 = (ecx & (1 << 0)) != 0;

		// Detect SSSE3
		bool has_ssse3 = (ecx & (1 << 9)) != 0;

		// Detect SSE4.1
		bool has_sse41 = (ecx & (1 << 19)) != 0;

		// Detect SSE4.2
		bool has_sse42 = (ecx & (1 << 20)) != 0;

		// Detect AES 
		bool has_aes = (ecx & (1 << 25)) != 0;

		// Detect CRC
		// (NOTE: CRC support is not directly signaled in CPUID, but is often tied to AVX2)
		bool has_avx2 = (ecx & (1 << 28)) != 0;

		std::cout << "CPU Features:" << std::endl;
		std::cout << "MMX: " << (has_mmx ? "Yes" : "No") << std::endl;
		std::cout << "SSE2: " << (has_sse2 ? "Yes" : "No") << std::endl;
		std::cout << "SSE3: " << (has_sse3 ? "Yes" : "No") << std::endl;
		std::cout << "SSSE3: " << (has_ssse3 ? "Yes" : "No") << std::endl;
		std::cout << "SSE4.1: " << (has_sse41 ? "Yes" : "No") << std::endl;
		std::cout << "SSE4.2: " << (has_sse42 ? "Yes" : "No") << std::endl;
		std::cout << "AES: " << (has_aes ? "Yes" : "No") << std::endl;
		std::cout << "AVX2: " << (has_avx2 ? "Yes (Likely has CRC support)" : "No") << std::endl;
	}

	SystemInfo::~SystemInfo()
	{
		if (m_socket.is_open())
		{
			m_socket.close();
		}
	}

	std::string ToJsonStr(const SystemInfo& sysInfo)
	{
		Json::JsonObject json;
		Json::JsonObject osJson;
		Json::JsonObject networkJson;
		Json::JsonObject cpuJson;
		Json::JsonArray toolsJson;
		Json::JsonArray librariesJson;

		const auto& os = sysInfo.GetOS();
		const auto& network = sysInfo.GetNetwork();
		const auto& cpu = sysInfo.GetCPU();
		const auto& tools = sysInfo.GetTools();
		const auto& libraries = sysInfo.GetLibraries();

		osJson["Name"] = os.name;
		osJson["Version"] = os.version;
		networkJson["PublicIP"] = network.publicIP;
		networkJson["PrivateIP"] = network.privateIP;
		cpuJson["Model"] = cpu.model;
		cpuJson["Arch"] = cpu.arch;

		for (const auto& tool : tools)
		{
			Json::JsonObject toolJson;
			toolJson["Name"] = tool.name;
			toolJson["Version"] = tool.version;
			toolJson["Path"] = tool.path;
			toolsJson.push_back(std::move(toolJson));
		}

		for (const auto& library : libraries)
		{
			Json::JsonObject libraryJson;
			libraryJson["Name"] = library.name;
			libraryJson["Version"] = library.version;
			librariesJson.push_back(std::move(libraryJson));
		}

		json["OS"] = std::move(osJson);
		json["CPU"] = std::move(cpuJson);
		json["Network"] = std::move(networkJson);
		json["Tools"] = std::move(toolsJson);
		json["Libraries"] = std::move(librariesJson);

		return Json::Serialize(json);
	}

	std::string ToXmlStr(const SystemInfo& sysInfo)
	{
		xmlNodePtr rootNode = xmlNewNode(NULL, BAD_CAST "value");
		xmlNodePtr structNode = xmlNewNode(NULL, BAD_CAST "struct");
		xmlNodePtr osNode = xmlNewNode(NULL, BAD_CAST "OS");
		xmlNodePtr networkNode = xmlNewNode(NULL, BAD_CAST "Network");
		xmlNodePtr cpuNode = xmlNewNode(NULL, BAD_CAST "CPU");
		xmlNodePtr toolsNode = xmlNewNode(NULL, BAD_CAST "Tools");
		xmlNodePtr librariesNode = xmlNewNode(NULL, BAD_CAST "Libraries");

		const auto& os = sysInfo.GetOS();
		const auto& network = sysInfo.GetNetwork();
		const auto& cpu = sysInfo.GetCPU();
		const auto& tools = sysInfo.GetTools();
		const auto& libraries = sysInfo.GetLibraries();

		Xml::AddNewNode(osNode, "Name", "string", os.name.c_str());
		Xml::AddNewNode(osNode, "Version", "string", os.name.c_str());
		Xml::AddNewNode(networkNode, "PublicIP", "string", network.publicIP.c_str());
		Xml::AddNewNode(networkNode, "PrivateIP", "string", network.privateIP.c_str());
		Xml::AddNewNode(cpuNode, "Model", "string", network.publicIP.c_str());
		Xml::AddNewNode(cpuNode, "PrivateIP", "string", network.privateIP.c_str());

		for (const auto& tool : tools)
		{
			Xml::AddNewNode(toolsNode, "Name", "string", tool.name.c_str());
			Xml::AddNewNode(toolsNode, "Version", "string", tool.version.c_str());
			Xml::AddNewNode(toolsNode, "Path", "string", tool.path.c_str());
		}

		for (const auto& library : libraries)
		{
			Xml::AddNewNode(librariesNode, "Name", "string", library.name.c_str());
			Xml::AddNewNode(librariesNode, "Version", "string", library.version.c_str());
		}

		xmlAddChild(structNode, osNode);
		xmlAddChild(structNode, networkNode);
		xmlAddChild(structNode, cpuNode);
		xmlAddChild(structNode, toolsNode);
		xmlAddChild(structNode, librariesNode);
		xmlAddChild(rootNode, structNode);

		std::string result = Xml::Serialize(rootNode);

		xmlFreeNode(rootNode);

		return result;
	}

	const std::vector<Library>& SystemInfo::GetLibraries() const
	{
		return m_libraries;
	}

	void SystemInfo::InitLibsVersions()
	{
		m_libraries.reserve(4);
		m_libraries.push_back({ "LibXML2", LIBXML_DOTTED_VERSION });

#if defined(HAVE_NCURSES_H) || defined(HAVE_NCURSES_NCURSES_H)
		m_libraries.push_back({ "ncurses", NCURSES_VERSION });
#endif

#ifndef DISABLE_GZIP
		m_libraries.push_back({ "Gzip", ZLIB_VERSION });
#endif

#ifdef HAVE_OPENSSL
		m_libraries.push_back({ "OpenSSL", OPENSSL_FULL_VERSION_STR });
#endif

#ifdef HAVE_LIBGNUTLS
		m_libraries.push_back({ "GnuTLS", GNUTLS_VERSION });
#endif
	}

	const CPU& SystemInfo::GetCPU() const
	{
		return m_cpu;
	}

	const OS& SystemInfo::GetOS() const
	{
		return m_os;
	}

	std::vector<Tool> SystemInfo::GetTools() const
	{
		std::vector<Tool> tools{
			GetPython(),
			GetSevenZip(),
			GetUnrar(),
		};

		return tools;
	}

	Tool SystemInfo::GetPython() const
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
			// e.g. Python 3.12.3
			std::string version = buffer;
			Util::Trim(version);
			size_t pos = version.find(" ");
			if (pos != std::string::npos)
			{
				python.version = version.substr(pos + 1);
			}
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

	Tool SystemInfo::GetUnrar() const
	{
		Tool tool;

		tool.name = "UnRAR";
		tool.path = GetUnpackerPath(g_Options->GetUnrarCmd());
		tool.version = GetUnpackerVersion(tool.path, "UNRAR", UnpackerVersionParserFunc);

		return tool;
	}

	Tool SystemInfo::GetSevenZip() const
	{
		Tool tool;

		tool.name = "7-Zip";
		tool.path = GetUnpackerPath(g_Options->GetSevenZipCmd());
		tool.version = GetUnpackerVersion(tool.path, tool.name.c_str(), UnpackerVersionParserFunc);

		return tool;
	}

	std::string SystemInfo::GetUnpackerPath(const char* unpackerCmd) const
	{
		if (Util::EmptyStr(unpackerCmd))
		{
			return "";
		}

		std::string path = unpackerCmd;
		Util::Trim(path);

		// getting the path itself without any keys
		return path.substr(0, path.find(" "));
	}

	std::string SystemInfo::GetUnpackerVersion(const std::string& path, const char* marker, const UnpackerVersionParser& parseVersion) const
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

	Network SystemInfo::GetNetwork() const
	{
		Network network{};

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
				return network;
			}

			size_t headersEndPos = response.find("\r\n\r\n");
			if (headersEndPos != std::string::npos)
			{
				response = response.substr(headersEndPos);
				Util::Trim(response);
				network.publicIP = std::move(response);
				network.privateIP = m_socket.local_endpoint().address().to_string();
			}
		}
		catch (const std::exception& e)
		{
			debug("Failed to get public and private IP: %s", e.what());
		}

		return network;
	}

#ifdef WIN32
	void SystemInfo::InitCPU()
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
			debug("Failed to get CPU model. Couldn't read Windows Registry.");
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
			debug("Failed to get CPU arch. Couldn't read Windows Registry.");
		}
	}

	void SystemInfo::InitOS()
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
				m_os.version = "";
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
			debug("Failed to get OS version. Couldn't read Windows Registry.");
		}

		m_os.name = "Windows";
	}
#endif

#ifdef __linux__
#include <fstream>
	bool SystemInfo::IsRunningInDocker() const
	{
		return FileSystem::FileExists("/.dockerenv");
	}

	void SystemInfo::InitCPU()
	{
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
				return;
			}
		}

		debug("Failed to find CPU model.");
	}

	void SystemInfo::InitOS()
	{
		std::ifstream osInfo("/etc/os-release");
		if (!osInfo.is_open())
		{
			debug("Failed to get OS info. Couldn't read '/etc/os-release'.");
			return;
		}

		std::string line;
		while (std::getline(osInfo, line))
		{
			if (!m_os.name.empty() && !m_os.version.empty())
			{
				return;
			}

			if (m_os.name.empty() && line.find("NAME=") == 0)
			{
				m_os.name = line.substr(line.find("=") + 1);
				Util::Trim(m_os.name);

				if (IsRunningInDocker())
				{
					m_os.name += " (Running in Docker)";
				}

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

		debug("Failed to find OS info.");
	}
#endif

#if defined(__unix__) && !defined(__linux__)
	void SystemInfo::InitCPU()
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
		}

		m_cpu.arch = GetCPUArch();
	}

	void SystemInfo::InitOS()
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
		}

		len = BUFFER_SIZE;

		if (sysctlbyname("kern.osrelease", &buffer, &len, nullptr, 0) == 0)
		{
			m_os.version = buffer;
			Util::Trim(m_os.version);
		}
		else
		{
			debug("Failed to get OS version. Failed to read 'kern.osrelease'.");
		}
	}
#endif

#ifdef __APPLE__
	void SystemInfo::InitCPU()
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
		}

		m_cpu.arch = GetCPUArch();
	}

	void SystemInfo::InitOS()
	{
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
			Util::Trim(m_os.version);
		}
	}
#endif

#ifndef WIN32
	std::string SystemInfo::GetCPUArch() const
	{
		const char* cmd = "uname -m";
		FILE* pipe = popen(cmd, "r");
		if (!pipe)
		{
			debug("Failed to get CPU arch. Couldn't read 'uname -m'.");

			return "";
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

		return "";
	}
#endif

}
