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
#include "Util.h"

class HardwareInfo final
{
public:
	struct CPU
	{
		std::string model;
		std::string arch;
	};

	struct Environment
	{
		std::string confPath;
		std::string controlIP;
		std::string unrarPath;
		std::string sevenZipPath;
		std::string pythonPath;
		std::string pythonVersion;
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
	
	HardwareInfo();
	CPU GetCPU() const;
	Environment GetEnvironment() const;
	Network GetNetwork() const;
	OS GetOS() const;
	DiskState GetDiskState(const char* root = ".") const;

private:
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

#endif
