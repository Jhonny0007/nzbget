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
#include <sstream>
#include <iostream>
#include "HardwareInfo.h"
#include "Options.h"
#include "ArticleWriter.h"

using namespace boost;

HardwareInfo::HardwareInfo()
{
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
	std::cout << "Conf. path: " << env.confPath << std::endl;
	std::cout << "ControlIP IP: " << env.controlIP << std::endl;
	std::cout << "ControlIP Port: " << env.controlPort << std::endl;
	std::cout << "Unrar: " << env.unrarPath << std::endl;
	std::cout << "SevenZip: " << env.sevenZipPath << std::endl;
	std::cout << "Python path: " << env.pythonPath << std::endl;
	std::cout << "Python version: " << env.pythonVersion << std::endl;
}

HardwareInfo::Environment HardwareInfo::GetEnvironment() const
{
	HardwareInfo::Environment env;

	env.confPath = g_Options->GetConfigFilename();
	env.controlIP = g_Options->GetControlIp();
	env.controlPort = g_Options->GetControlPort();
	env.unrarPath = g_Options->GetUnrarCmd();
	env.sevenZipPath = g_Options->GetSevenZipCmd();
	std::string cmd = std::string(g_Options->GetSevenZipCmd());
	std::stringstream ss;
	char buffer[128];
	FILE* pipe = popen(cmd.c_str(), "r");
	int i = 0;
	if (pipe) {
		while (!feof(pipe) && i != 2)
		{
			if (fgets(buffer, 128, pipe) != nullptr && i == 1)
			{
				ss << buffer;
			}
			++i;
		}
		i = 0;
		pclose(pipe);
	}

	size_t startPos = ss.str().find(")");
	size_t endPos = ss.str().find("(");
	if (startPos != std::string::npos && endPos != std::string::npos)
	{
		std::string version = ss.str().substr(startPos + 1, endPos);
		Util::Trim(version);
	}

	ss.str("");
	ss.clear();
	cmd = std::string(g_Options->GetUnrarCmd());
	pipe = popen(cmd.c_str(), "r");
	if (pipe) {
		while (!feof(pipe) && i != 2)
		{
			if (fgets(buffer, 128, pipe) != nullptr && i == 1)
			{
				ss << buffer;
			}
			++i;
		}
		pclose(pipe);
	}
	std::string version = ss.str().substr(sizeof("UNRAR"), 4);
	Util::Trim(version);

	auto pythonResult = Util::FindPython();
	if (pythonResult.has_value())
	{
		std::string cmd = pythonResult.get() + " --version";
		pipe = popen(cmd.c_str(), "r");
		if (pipe) {
			while (!feof(pipe))
			{
				if (fgets(buffer, 128, pipe) != nullptr)
				{
					ss << buffer;
				}
			}
			pclose(pipe);
		}
		env.pythonVersion = buffer;
	}

	ss.str("");
	ss.clear();
	if (pythonResult.has_value())
	{
#ifdef WIN32
		std::string findCmd = "where ";
#else
		std::string findCmd = "which ";
#endif
		std::string cmd = findCmd + pythonResult.get();
		pipe = popen(cmd.c_str(), "r");
		if (pipe) {
			while (!feof(pipe))
			{
				if (fgets(buffer, 128, pipe) != nullptr)
				{
					ss << buffer;
				}
			}
			pclose(pipe);
		}
		env.pythonPath = buffer;
	}

	return env;
}

HardwareInfo::Network HardwareInfo::GetNetwork() const
{
	HardwareInfo::Network network;

	try {
		asio::io_context context;
		asio::ip::tcp::resolver resolver(context);
		asio::ip::tcp::socket socket(context);
		asio::connect(socket, resolver.resolve("icanhazip.com", "http"));

		std::string request = "GET / HTTP/1.1\r\nHost: icanhazip.com\r\n\r\n";
		asio::write(socket, asio::buffer(request));

		char reply[1024];
		size_t total_read = socket.read_some(asio::buffer(reply, sizeof(reply)));

		std::string response(reply, total_read);
		size_t pos = response.find("\r\n\r\n");

		if (pos != std::string::npos)
		{
			response = response.substr(pos);
			Util::Trim(response);
			network.publicIP = response;
			network.privateIP = socket.local_endpoint().address().to_string();
		}
	}
	catch (std::exception& e)
	{
		network.publicIP = "Unknown";
		network.privateIP = "Unknown";
	}

	return network;
}

#ifdef WIN32
HardwareInfo::CPU HardwareInfo::GetCPU() const
{
	HardwareInfo::CPU cpu;

	int len = 128;
	char cpuModelBuffer[128];
	char cpuArchBuffer[128];
	if (Util::RegReadStr(
		HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		"ProcessorNameString",
		cpuModelBuffer,
		&len))
	{
		cpu.model = cpuModelBuffer;
		Util::Trim(cpu.model);
	}

	if (Util::RegReadStr(
		HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
		"PROCESSOR_ARCHITECTURE",
		cpuArchBuffer,
		&len))
	{
		cpu.arch = cpuArchBuffer;
		Util::Trim(cpu.arch);
	}

	return cpu;
}

HardwareInfo::OS HardwareInfo::GetOS() const
{
	HardwareInfo::OS os;

	int len = 128;
	char buffer[128];
	if (Util::RegReadStr(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\MICROSOFT\\Windows NT\\CurrentVersion",
		"CurrentBuild",
		buffer,
		&len))
	{
		long buildNum = std::atol(buffer);
		if (buildNum >= m_win11BuildVersion)
		{
			os.version = "11";
		}
		else if (buildNum >= m_win10BuildVersion)
		{
			os.version = "10";
		}
		else if (buildNum >= m_win8BuildVersion)
		{
			os.version = "8";
		}
		else if (buildNum >= m_winXPBuildVersion)
		{
			os.version = "XP";
		}
	}

	os.name = "Windows";
	return os;
}

HardwareInfo::DiskState HardwareInfo::GetDiskState(const char* root) const
{
	ULARGE_INTEGER freeBytesAvailable;
	ULARGE_INTEGER totalNumberOfBytes;
	int64 articleCache = g_ArticleCache->GetAllocated();
	uint32 articleCacheHi, articleCacheLo;
	Util::SplitInt64(articleCache, &articleCacheHi, &articleCacheLo);

	if (GetDiskFreeSpaceEx(root, &freeBytesAvailable, &totalNumberOfBytes, nullptr))
	{
		return { freeBytesAvailable.QuadPart, freeBytesAvailable.QuadPart, static_cast<size_t>(articleCache) };
	}

	return { 0, 0, static_cast<size_t>(articleCache) };
}
#endif

#ifdef __linux__
#include <fstream>
HardwareInfo::CPU HardwareInfo::GetCPU() const
{
	HardwareInfo::CPU cpu;

	std::ifstream cpuinfo("/proc/cpuinfo");
	std::string line;

	while (std::getline(cpuinfo, line))
	{
		if (line.find("model name") != std::string::npos)
		{
			cpu.model = line.substr(line.find(":") + 2);
			Util::Trim(cpu.model);
			break;
		}
	}
	cpu.arch = GetCPUArch();
	return cpu;
}

HardwareInfo::OS HardwareInfo::GetOS() const
{
	HardwareInfo::OS os;

	std::ifstream osInfo("/etc/os-release");
	if (!osInfo.is_open())
	{
		return os;
	}

	size_t startPos = 0;
	std::string line;
	while (std::getline(osInfo, line))
	{
		if (os.name && os.version)
		{
			break;
		}

		if (!os.name && line.find("NAME") != std::string::npos)
		{
			startPos = sizeof("NAME=");
			os.name = line.substr(startPos);
			Util::Trim(os.name);
			continue;
		}

		if (!os.version && line.find("VERSION_ID") != std::string::npos)
		{
			startPos = sizeof("VERSION_ID=");
			os.version = line.substr(startPos);
			Util::Trim(os.version);
			continue;
		}

		if (!os.version && line.find("BUILD_ID") != std::string::npos)
		{
			startPos = sizeof("BUILD_ID=");
			os.version = line.substr(startPos);
			Util::Trim(os.version);
			continue;
		}
	}

	return os;
}
#endif

#if defined(__unix__) && !defined(__linux__)
HardwareInfo::CPU HardwareInfo::GetCPU() const
{
	HardwareInfo::CPU cpu;

	size_t len = 128;
	char cpuModel[128];
	if (sysctlbyname("hw.model", &cpuModel, &len, nullptr, 0) == 0)
	{
		cpu.model = cpuModel;
		Util::Trim(cpu.model);
	}

	cpu.arch = GetCPUArch();
	return cpu;
}

HardwareInfo::OS HardwareInfo::GetOS() const
{
	HardwareInfo::OS os;

	size_t len = 128;
	char osNameBuffer[128];
	if (sysctlbyname("kern.ostype", &osNameBuffer, &len, nullptr, 0) == 0)
	{
		os.name = osNameBuffer;
		Util::Trim(os.name);
	}

	char osReleaseBuffer[128];
	if (sysctlbyname("kern.osrelease", &osReleaseBuffer, &len, nullptr, 0) == 0)
	{
		os.version = osReleaseBuffer;
		Util::Trim(os.version);
	}

	return os;
}
#endif

#ifdef __APPLE__
HardwareInfo::CPU HardwareInfo::GetCPU() const
{
	HardwareInfo::CPU cpu;

	size_t len = 128;
	char buffer[128];
	if (sysctlbyname("machdep.cpu.brand_string", &buffer, &len, nullptr, 0) == 0)
	{
		cpu.model = buffer;
		Util::Trim(cpu.model);
	}

	cpu.arch = GetCPUArch();
	return cpu;
}

HardwareInfo::OS HardwareInfo::GetOS() const
{
	HardwareInfo::OS os;

	FILE* pipe = popen("sw_vers", "r");
	if (!pipe)
	{
		return os;
	}

	char buffer[128];
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
		os.name = result.substr(pos + productName.size(), endPos - pos - productName.size());
		Util::Trim(os.name);
	}

	std::string productVersion = "ProductVersion:";
	pos = result.find(productVersion);
	if (pos != std::string::npos)
	{
		size_t endPos = result.find("\n", pos);
		os.version = result.substr(pos + productVersion.size(), endPos - pos - productVersion.size());
		Util::Trim(os.version);
	}

	return os;
}
#endif

#ifndef WIN32
std::string HardwareInfo::GetCPUArch() const
{
	const char* cmd = "uname -m";
	FILE* pipe = popen(cmd, "r");
	if (!pipe)
	{
		return "";
	}

	char buffer[128];
	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe))
		{
			pclose(pipe);
			Util::Trim(buffer);
			return buffer;
		}
	}

	pclose(pipe);

	return "";
}

HardwareInfo::DiskState HardwareInfo::GetDiskState(const char* root) const
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
