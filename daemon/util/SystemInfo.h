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

#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <string>
#include "OSInfo.h"
#include "CPUInfo.h"
#include "NetworkInfo.h"

namespace SystemInfo
{
	using UnpackerVersionParser = std::function<std::string(const std::string&)>;

	struct Library
	{
		std::string name;
		std::string version;
	};

	struct Tool
	{
		std::string name;
		std::string version;
		std::string path;
	};

	class SystemInfo final
	{
	public:
		SystemInfo();
		~SystemInfo() = default;
		std::vector<Tool> GetTools() const;
		NetworkInfo GetNetworkInfo() const;
		const std::vector<Library>& GetLibraries() const;
		const CPUInfo& GetCPUInfo() const;
		const OSInfo& GetOSInfo() const;

	private:
		Tool GetPython() const;
		Tool GetSevenZip() const;
		Tool GetUnrar() const;
		void InitLibVersions();
		std::string ParseUnpackerVersion(const std::string& line) const;
		std::string GetUnpackerPath(const char* unpackerCmd) const;
		std::string GetUnpackerVersion(const std::string& path, const char* marker) const;

		CPUInfo m_cpu;
		OSInfo m_os;
		std::vector<Library> m_libraries;
	};

	std::string ToJsonStr(const SystemInfo& sysInfo);
	std::string ToXmlStr(const SystemInfo& sysInfo);
}

extern SystemInfo::SystemInfo* g_SystemInfo;

#endif

