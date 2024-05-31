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
#include "HardwareInfo.h"
#include "Options.h"

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
	auto network = GetNetwork();
	std::cout << "Public IP: " << network.publicIP << std::endl;
	std::cout << "Private IP: " << network.privateIP << std::endl;
	auto env = GetEnvironment();
	std::cout << "Conf. path: " << env.confPath << std::endl;
	std::cout << "ControlIP IP: " << env.controlIP << std::endl;
	std::cout << "ControlIP Port: " << env.controlPort << std::endl;
}

HardwareInfo::Environment HardwareInfo::GetEnvironment() const
{
	HardwareInfo::Environment env;

	env.confPath = g_Options->GetConfigFilename();
	env.controlIP = g_Options->GetControlIp();
	env.controlPort = g_Options->GetControlPort();

	return env;
}

HardwareInfo::Network HardwareInfo::GetNetwork() const
{
	HardwareInfo::Network network;

	try {
		boost::asio::io_context context;
		boost::asio::ip::tcp::resolver resolver(context);
		boost::asio::ip::tcp::socket socket(context);
		boost::asio::connect(socket, resolver.resolve("icanhazip.com", "http"));

		std::string request = "GET / HTTP/1.1\r\nHost: icanhazip.com\r\n\r\n";
		boost::asio::write(socket, boost::asio::buffer(request));

		char reply[1024];
		size_t total_read = socket.read_some(boost::asio::buffer(reply, sizeof(reply)));

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
	else
	{
		cpu.model = "Unknown";
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
	else
	{
		cpu.arch = "Unknown";
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
		else
		{
			os.version = "Unknown";
		}
	}

	os.name = "Windows";
	return os;
}

HardwareInfo::DiskState HardwareInfo::GetDiskState(const char* root) const
{
	ULARGE_INTEGER freeBytesAvailable;
	ULARGE_INTEGER totalNumberOfBytes;

	if (GetDiskFreeSpaceEx(root, &freeBytesAvailable, &totalNumberOfBytes, nullptr))
	{
		return { freeBytesAvailable.QuadPart, totalNumberOfBytes.QuadPart };
	}

	return { 0, 0 };
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

	std::ifstream cpuinfo("/proc/sys/kernel/ostype");
	std::string line;
	if (std::getline(cpuinfo, line))
	{
		os.name = std::move(line);
		Util::Trim(os.name);
	}
	else
	{
		os.name = "Unknown";
	}

	std::ifstream osRelease("/proc/sys/kernel/osrelease");
	if (std::getline(osRelease, line))
	{
		os.version = std::move(line);
		Util::Trim(os.version);
	}
	else
	{
		os.version = "Unknown";
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
	else
	{
		cpu.model = "Unknown";
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
	else
	{
		os.name = "Unknown";
	}

	char osReleaseBuffer[128];
	if (sysctlbyname("kern.osrelease", &osReleaseBuffer, &len, nullptr, 0) == 0)
	{
		os.version = osReleaseBuffer;
		Util::Trim(os.version);
	}
	else
	{
		os.version = "Unknown";
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
	else
	{
		cpu.model = "Unknown";
	}

	cpu.arch = GetCPUArch();
	return cpu;
}

HardwareInfo::OS HardwareInfo::GetOS() const
{
	HardwareInfo::OS os;

	FILE* pipe = popen("sw_vers", "r");
	if (!pipe) {
		os.name = "Unknown";
		os.version = "Unknown";
		return os;
	}

	char buffer[128];
	std::string result = "";
	while (!feof(pipe)) {
		if (fgets(buffer, sizeof(buffer), pipe))
		{
			result += buffer;
		}
	}

	pclose(pipe);

	std::string productName = "ProductName:";
	size_t pos = result.find(productName);
	if (pos != std::string::npos) {
		size_t endPos = result.find("\n", pos);
		os.name = result.substr(pos + productName.size(), endPos - pos - productName.size());
		Util::Trim(os.name);
	}
	else
	{
		os.name = "Unknown";
	}

	std::string productVersion = "ProductVersion:";
	pos = result.find(productVersion);
	if (pos != std::string::npos) {
		size_t endPos = result.find("\n", pos);
		os.version = result.substr(pos + productVersion.size(), endPos - pos - productVersion.size());
		Util::Trim(os.version);
	}
	else
	{
		os.version = "Unknown";
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
		return "Unknown";
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
	return "Unknown";
}

HardwareInfo::DiskState HardwareInfo::GetDiskState(const char* root) const
{
	struct statvfs diskdata;
	if (statvfs(root, &diskdata) == 0)
	{
		size_t available = diskdata.f_bfree * diskdata.f_frsize;
		size_t total = diskdata.f_blocks * diskdata.f_frsize;
		return { available, total };
	}
	return { 0, 0 };
}
#endif
