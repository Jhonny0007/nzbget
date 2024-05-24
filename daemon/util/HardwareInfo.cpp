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
		char cpuModel[256];
		DWORD size = sizeof(cpuModel);
		if (RegQueryValueEx(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)cpuModel, &size) == ERROR_SUCCESS)
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
#endif

#ifdef __linux__
#include <fstream> 
void HardwareInfo::InitCpuModel()
{
	std::ifstream cpuinfo("/proc/cpuinfo");
	std::string line;

	while (std::getline(cpuinfo, line)) {
		if (line.find("model name") != std::string::npos) {
			m_cpuModel = line.substr(line.find(":") + 2);
			std::cout << "CPU Model: " << m_cpuModel << std::endl;
			return;
		}
	}

	std::cout << "Failed to get CPU Model" << std::endl;
}
#endif

#if defined(__unix__)
void HardwareInfo::InitCpuModel()
{
	char cpuModel[256];
	size_t len = sizeof(cpuModel);
	if (sysctlbyname("hw.model", &cpuModel, &len, NULL, 0) == 0)
	{
		m_cpuModel = cpuModel;
		std::cout << "CPU Model: " << m_cpuModel << std::endl;
		return;
	}

	std::cout << "Failed to get CPU Model" << std::endl;
}
#endif

#if defined(__APPLE__)
void HardwareInfo::InitCpuModel()
{
	char cpuModel[256];
	size_t len = sizeof(cpuModel);
	if (sysctlbyname("machdep.cpu.brand_string", &cpuModel, &len, NULL, 0) == 0)
	{
		m_cpuModel = cpuModel;
		std::cout << "CPU Model: " << m_cpuModel << std::endl;
		return;
	}

	std::cout << "Failed to get CPU Model" << std::endl;
}
#endif

const std::string& HardwareInfo::GetCpuModel()
{
	return m_cpuModel;
}
