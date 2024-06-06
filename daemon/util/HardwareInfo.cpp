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
#include "HardwareInfo.h"
#include "Options.h"
#include "ArticleWriter.h"

namespace HardwareInfo
{

	using namespace boost;

#ifdef WIN32
	const char* FIND_CMD = "where ";
#else
	const char* FIND_CMD = "readlink -f ";
#endif
	const size_t BUFFER_SIZE = 256;

	HardwareInfo::HardwareInfo()
		: m_context{}
		, m_resolver{ m_context }
		, m_socket{ m_context }
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
		std::cout << "Conf. path: " << env.configPath << std::endl;
		std::cout << "ControlIP IP: " << env.controlIP << std::endl;
		std::cout << "ControlIP Port: " << env.controlPort << std::endl;
		std::cout << "Unrar: " << env.unrar.path << std::endl;
		std::cout << "SevenZip: " << env.sevenZip.path << std::endl;
		std::cout << "Python path: " << env.python.path << std::endl;
		std::cout << "Python version: " << env.python.version << std::endl;
	}

	HardwareInfo::~HardwareInfo()
	{
		if (m_socket.is_open())
		{
			m_socket.close();
		}
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
		Tool unrar;
		unrar.name = "UnRAR";

		std::string path = g_Options->GetUnrarCmd();
		if (path.empty())
		{
			return unrar;
		}

		std::string unrarPath = path.substr(0, path.find(" "));

		if (FileSystem::FileExists(unrarPath.c_str()))
		{
			unrar.path = std::move(unrarPath);
		}
		else
		{
			std::string absolutePath = std::string(g_Options->GetAppDir()) + PATH_SEPARATOR + unrarPath;
			if (!FileSystem::FileExists(absolutePath.c_str()))
			{
				return unrar;
			}

			unrar.path = std::move(absolutePath);
		}

		FILE* pipe = popen(unrar.path.c_str(), "r");
		if (!pipe)
		{
			return unrar;
		}

		char buffer[BUFFER_SIZE];
		while (!feof(pipe))
		{
			if (fgets(buffer, BUFFER_SIZE, pipe) != nullptr)
			{
				// UNRAR 5.70 x64 freeware      Copyright (c) 1993-2019 Alexander Roshal
				if (strstr(buffer, "UNRAR"))
				{
					std::string version = std::string(buffer).substr(sizeof("UNRAR"), 4);
					Util::Trim(version);
					unrar.version = std::move(version);
					break;
				}
			}
		}

		pclose(pipe);

		return unrar;
	}

	Tool HardwareInfo::GetSevenZip() const
	{
		Tool sevenZip;
		sevenZip.name = "7-Zip";

		std::string path = g_Options->GetSevenZipCmd();
		if (path.empty())
		{
			return sevenZip;
		}

		path = path.substr(0, path.find(" "));

		std::string sevenZipPath = path.substr(0, path.find(" "));

		if (FileSystem::FileExists(sevenZipPath.c_str()))
		{
			sevenZip.path = std::move(sevenZipPath);
		}
		else
		{
			std::string absolutePath = std::string(g_Options->GetAppDir()) + PATH_SEPARATOR + sevenZipPath;
			if (!FileSystem::FileExists(absolutePath.c_str()))
			{
				return sevenZip;
			}

			sevenZip.path = std::move(absolutePath);
		}

		FILE* pipe = popen(sevenZip.path.c_str(), "r");
		if (!pipe)
		{
			return sevenZip;
		}

		char buffer[BUFFER_SIZE];
		while (!feof(pipe))
		{
			if (fgets(buffer, BUFFER_SIZE, pipe) != nullptr)
			{
				// 7-Zip (a) 19.00 (x64) : Copyright (c) 1999-2018 Igor Pavlov : 2019-02-21
				if (strstr(buffer, "7-Zip"))
				{
					std::string line = std::string(buffer);
					size_t startPos = line.find(")");
					size_t endPos = line.find("(");
					std::string version = line.substr(startPos + 1, endPos - 1);
					Util::Trim(version);
					sevenZip.version = std::move(version);
					break;
				}
			}
		}

		pclose(pipe);

		return sevenZip;
	}

	Network HardwareInfo::GetNetwork()
	{
		Network network;

		try {
			asio::connect(m_socket, m_resolver.resolve("icanhazip.com", "http"));

			std::string request = "GET / HTTP/1.1\r\nHost: icanhazip.com\r\n\r\n";
			asio::write(m_socket, asio::buffer(request));

			char buffer[1024];
			size_t totalSize = m_socket.read_some(asio::buffer(buffer, 1024));

			std::string response(buffer, totalSize);
			if (response.find("200 OK") == std::string::npos) {
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
		catch (std::exception& e)
		{
			debug("Failed to get public and private IP: %s", e.what());
		}

		return network;
	}

#ifdef WIN32
	CPU HardwareInfo::GetCPU() const
	{
		CPU cpu;

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

	OS HardwareInfo::GetOS() const
	{
		OS os;

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

	DiskState HardwareInfo::GetDiskState(const char* root) const
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
	CPU HardwareInfo::GetCPU() const
	{
		CPU cpu;

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

	OS HardwareInfo::GetOS() const
	{
		OS os;

		std::ifstream osInfo("/etc/os-release");
		if (!osInfo.is_open())
		{
			return os;
		}

		std::string line;
		while (std::getline(osInfo, line))
		{
			if (!os.name.empty() && !os.version.empty())
			{
				break;
			}

			if (os.name.empty() && line.find("NAME=") == 0)
			{
				os.name = line.substr(line.find("=") + 1);
				Util::Trim(os.name);
				continue;
			}

			if (os.version.empty() && line.find("VERSION_ID=") == 0)
			{
				os.version = line.substr(line.find("=") + 1);
				Util::Trim(os.version);
				continue;
			}

			if (os.version.empty() && line.find("BUILD_ID=") == 0)
			{
				os.version = line.substr(line.find("="));
				Util::Trim(os.version);
				continue;
			}
		}

		return os;
	}
#endif

#if defined(__unix__) && !defined(__linux__)
	CPU HardwareInfo::GetCPU() const
	{
		CPU cpu;

		size_t len = BUFFER_SIZE;
		char cpuModel[BUFFER_SIZE];
		if (sysctlbyname("hw.model", &cpuModel, &len, nullptr, 0) == 0)
		{
			cpu.model = cpuModel;
			Util::Trim(cpu.model);
		}

		cpu.arch = GetCPUArch();
		return cpu;
	}

	OS HardwareInfo::GetOS() const
	{
		OS os;

		size_t len = BUFFER_SIZE;
		char osNameBuffer[BUFFER_SIZE];
		if (sysctlbyname("kern.ostype", &osNameBuffer, &len, nullptr, 0) == 0)
		{
			os.name = osNameBuffer;
			Util::Trim(os.name);
		}

		char osReleaseBuffer[BUFFER_SIZE];
		if (sysctlbyname("kern.osrelease", &osReleaseBuffer, &len, nullptr, 0) == 0)
		{
			os.version = osReleaseBuffer;
			Util::Trim(os.version);
		}

		return os;
	}
#endif

#ifdef __APPLE__
	CPU HardwareInfo::GetCPU() const
	{
		CPU cpu;

		size_t len = BUFFER_SIZE;
		char buffer[BUFFER_SIZE];
		if (sysctlbyname("machdep.cpu.brand_string", &buffer, &len, nullptr, 0) == 0)
		{
			cpu.model = buffer;
			Util::Trim(cpu.model);
		}

		cpu.arch = GetCPUArch();
		return cpu;
	}

	OS HardwareInfo::GetOS() const
	{
		OS os;

		FILE* pipe = popen("sw_vers", "r");
		if (!pipe)
		{
			return os;
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

		return "";
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
