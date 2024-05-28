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

class HardwareInfo final
{
public:
	HardwareInfo();
	const std::string& GetCpuModel() const;
	const std::string& GetOS() const;
	const std::string& GetOSVersion() const;
	const std::string& GetArch() const;
	const std::string& GetDiskTotalSize() const;
private:
	void InitCpuModel();
	void InitOS();
	void InitOSVersion();
	void InitArch();
	void InitDiskTotalSize();
	std::string m_arch;
	std::string m_cpuModel;
	std::string m_os;
	std::string m_osVersion;
	std::string m_diskTotalSize; 

#ifdef WIN32
	const long m_win11BuildVersion = 22000;
	const long m_win10BuildVersion = 10240;
	const long m_win8BuildVersion = 9200;
	const long m_win7BuildVersion = 7600;
	const long m_winXPBuildVersion = 2600;
#endif
};

#endif
