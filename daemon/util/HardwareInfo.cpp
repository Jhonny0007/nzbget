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

#include "HardwareInfo.h"

HardwareInfo::HardwareInfo()
{
	InitCpuModel();
	InitOS();
	InitOSVersion();
	InitArch();
	auto s = GetDiskState();
}

#ifdef WIN32
void HardwareInfo::InitCpuModel()
{
	HKEY hKey;
	if (RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		0,
		KEY_READ,
		&hKey) == ERROR_SUCCESS)
	{
		char cpuModel[128];
		DWORD size = sizeof(cpuModel);
		if (RegQueryValueEx(hKey,
			"ProcessorNameString",
			nullptr,
			nullptr,
			(LPBYTE)cpuModel,
			&size) == ERROR_SUCCESS)
		{
			m_cpuModel = cpuModel;
			std::cout << "CPU Model: " << m_cpuModel << std::endl;
		}
		else
		{
			std::cout << "Failed to get CPU model" << std::endl;
		}
		RegCloseKey(hKey);

		return;
	}

	std::cout << "Failed to open Registry key" << std::endl;
}

void HardwareInfo::InitOS()
{
	m_os = "Windows";
}

void HardwareInfo::InitOSVersion()
{
	HKEY hKey;
	if (RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\MICROSOFT\\Windows NT\\CurrentVersion",
		0,
		KEY_READ,
		&hKey) == ERROR_SUCCESS)
	{
		char currentBuild[128];
		DWORD size = sizeof(currentBuild);
		if (RegQueryValueEx(hKey,
			"CurrentBuild",
			nullptr,
			nullptr,
			(LPBYTE)currentBuild,
			&size) == ERROR_SUCCESS)
		{
			long buildNum = std::atol(currentBuild);
			const std::string build = std::string("(Build ") + currentBuild + ")";
			std::cout << "Build number: " << buildNum << std::endl;
			RegCloseKey(hKey);

			if (buildNum >= m_win11BuildVersion)
			{
				m_osVersion = std::string("11 ") + build;
				return;
			}
			if (buildNum >= m_win10BuildVersion)
			{
				m_osVersion = std::string("10 ") + build;
				return;
			}
			if (buildNum >= m_win8BuildVersion)
			{
				m_osVersion = std::string("8 ") + build;
				return;
			}
			if (buildNum >= m_winXPBuildVersion)
			{
				m_osVersion = std::string("XP ") + build;
				return;
			}
			m_osVersion = std::move(build);
			return;
		}

		std::cout << "Failed to get currentBuild" << std::endl;

		RegCloseKey(hKey);
	}
}

void HardwareInfo::InitArch()
{
	HKEY hKey;
	DWORD dwBufferSize = MAX_PATH;
	char arch[128];

	if (RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
		0,
		KEY_READ,
		&hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, "PROCESSOR_ARCHITECTURE", NULL, NULL, (LPBYTE)arch, &dwBufferSize) == ERROR_SUCCESS)
		{
			m_arch = arch;
			RegCloseKey(hKey);
			return;
		}
	}

	RegCloseKey(hKey);
}

HardwareInfo::DiskState HardwareInfo::GetDiskState(const char* root) const
{
	ULARGE_INTEGER freeBytesAvailable;
	ULARGE_INTEGER totalNumberOfBytes;

	if (GetDiskFreeSpaceEx(root, &freeBytesAvailable, &totalNumberOfBytes, nullptr))
	{
		return { totalNumberOfBytes.QuadPart, freeBytesAvailable.QuadPart };
	}

	return { 0, 0 };
}
#endif

#ifdef __linux__
#include <fstream> 
void HardwareInfo::InitCpuModel()
{
	std::ifstream cpuinfo("/proc/cpuinfo");
	std::string line;

	while (std::getline(cpuinfo, line))
	{
		if (line.find("model name") != std::string::npos)
		{
			m_cpuModel = line.substr(line.find(":") + 2);
			std::cout << "CPU Model: " << m_cpuModel << std::endl;
			return;
		}
	}

	std::cout << "Failed to get CPU Model" << std::endl;
}

void HardwareInfo::InitOS()
{
	std::ifstream cpuinfo("/proc/sys/kernel/ostype");
	std::string line;
	if (std::getline(cpuinfo, line))
	{
		m_os = std::move(line);
		std::cout << "OS: " << m_os << std::endl;
	}
}

void HardwareInfo::InitOSVersion()
{
	std::ifstream cpuinfo("/proc/sys/kernel/osrelease");
	std::string line;
	if (std::getline(cpuinfo, line))
	{
		m_osVersion = std::move(line);
		std::cout << "version: " << m_osVersion << std::endl;
	}
}
#endif

#if defined(__unix__) && !defined(__linux__)
void HardwareInfo::InitCpuModel()
{
	char cpuModel[128];
	size_t len = sizeof(cpuModel);
	if (sysctlbyname("hw.model", &cpuModel, &len, nullptr, 0) == 0)
	{
		m_cpuModel = cpuModel;
		std::cout << "CPU Model: " << m_cpuModel << std::endl;
		return;
	}

	std::cout << "Failed to get CPU Model" << std::endl;
}

void HardwareInfo::InitOS()
{
	char os[128];
	size_t len = sizeof(os);
	if (sysctlbyname("kern.ostype", &os, &len, nullptr, 0) == 0)
	{
		m_os = os;
		std::cout << "OS: " << m_os << std::endl;
		return;
	}
	std::cout << "Failed to get OS" << std::endl;
}

void HardwareInfo::InitOSVersion()
{
	char version[128];
	size_t len = sizeof(version);
	if (sysctlbyname("kern.osrelease", &version, &len, nullptr, 0) == 0)
	{
		m_osVersion = version;
		std::cout << "OS Version: " << m_osVersion << std::endl;
		return;
	}
	std::cout << "Failed to get OS Version" << std::endl;
}
#endif

#ifdef __APPLE__
void HardwareInfo::InitCpuModel()
{
	char cpuModel[128];
	size_t len = sizeof(cpuModel);
	if (sysctlbyname("machdep.cpu.brand_string", &cpuModel, &len, nullptr, 0) == 0)
	{
		m_cpuModel = cpuModel;
		std::cout << "CPU Model: " << m_cpuModel << std::endl;
		return;
	}

	std::cout << "Failed to get CPU Model" << std::endl;
}

void HardwareInfo::InitOS()
{
	std::string cmd = "sw_vers -productName";
	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe)
	{
		return;
	}

	char buffer[128];
	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe))
		{
			m_os += buffer;
		}
	}
	std::cout << "OS: " << m_os << std::endl;
	pclose(pipe);
}

void HardwareInfo::InitOSVersion()
{
	std::string cmd = "sw_vers -productVersion";
	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe)
	{
		return;
	}

	char buffer[128];
	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe) != nullptr)
		{
			m_osVersion += buffer;
			std::cout << "OS Version: " << m_osVersion << std::endl;
		}
	}

	pclose(pipe);
}
#endif

#if defined(__unix__) || defined(__APPLE__)
void HardwareInfo::InitArch()
{
	const char* cmd = "uname -m";
	FILE* pipe = popen(cmd, "r");
	if (!pipe)
	{
		std::cout << "Failed to get Arch" << std::endl;
		return;
	}

	char buffer[128];
	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe))
		{
			m_arch += buffer;
		}
	}
	std::cout << "Arch: " << m_arch << std::endl;
	pclose(pipe);
}

HardwareInfo::DiskState HardwareInfo::GetDiskState(const char* root) const
{
	struct statvfs diskdata;
	if (statvfs(root, &diskdata) == 0)
	{	
		size_t total = diskdata.f_blocks * diskdata.f_frsize;
		size_t available = diskdata.f_bfree * diskdata.f_frsize;
		std::cout << "Total HD: " << total << std::endl;
		std::cout << "Available HD: " << available << std::endl;
		return { total, available };
	}
	return { 0, 0 }
}
#endif

const std::string& HardwareInfo::GetCpuModel() const
{
	return m_cpuModel;
}

const std::string& HardwareInfo::GetOS() const
{
	return m_os;
}

const std::string& HardwareInfo::GetOSVersion() const
{
	return m_osVersion;
}

const std::string& HardwareInfo::GetArch() const
{
	return m_arch;
}
