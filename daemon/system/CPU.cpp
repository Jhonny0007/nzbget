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

#include "CPU.h"
#include "Util.h"
#include "Log.h"
#include "FileSystem.h"

namespace SystemInfo
{
	const int BUFFER_SIZE = 512;

	CPU::CPU()
	{
		Init();
	}

	const std::string& CPU::GetModel() const
	{
		return m_model;
	}

	const std::string& CPU::GetArch() const
	{
		return m_arch;
	}

#ifdef WIN32
	void CPU::Init()
	{
		int len = BUFFER_SIZE;
		char modelBuffer[BUFFER_SIZE];
		char archBuffer[BUFFER_SIZE];
		if (Util::RegReadStr(
			HKEY_LOCAL_MACHINE,
			"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
			"ProcessorNameString",
			modelBuffer,
			&len))
		{
			m_model = modelBuffer;
			Util::Trim(m_model);
		}
		else
		{
			warn("Failed to get CPU model. Couldn't read Windows Registry.");
		}

		if (Util::RegReadStr(
			HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
			"PROCESSOR_ARCHITECTURE",
			archBuffer,
			&len))
		{
			m_arch = archBuffer;
			Util::Trim(m_arch);
		}
		else
		{
			warn("Failed to get CPU arch. Couldn't read Windows Registry.");
		}
	}
#endif	

#ifdef __linux__
#include <fstream>
	void CPU::Init()
	{
		m_arch = GetCPUArch();

		std::ifstream cpuinfo("/proc/cpuinfo");
		if (!cpuinfo.is_open())
		{
			std::string line;
			while (std::getline(cpuinfo, line))
			{
				if (line.find("model name") != std::string::npos ||
					line.find("Processor") != std::string::npos ||
					line.find("cpu model") != std::string::npos ||
					line.find("cpu") != std::string::npos)
				{
					m_model = line.substr(line.find(":") + 1);
					Util::Trim(m_model);
					return;
				}
			}
		}

		std::string cmd = std::string("lscpu | grep \"Model name\"") + Util::NULL_ERR_OUTPUT;
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe)
		{
			warn("Failed to get CPU model. Couldn't read 'lscpu'.");

			return;
		}

		char buffer[BUFFER_SIZE];
		while (!feof(pipe))
		{
			if (fgets(buffer, BUFFER_SIZE, pipe))
			{
				pclose(pipe);
				m_model = std::string(buffer).substr(line.find(":") + 1);
				Util::Trim(m_model);
				return;
			}
		}

		pclose(pipe);

		warn("Failed to get CPU model.");
	}
#endif

#if __BSD__
	void CPU::Init()
	{
		size_t len = BUFFER_SIZE;
		char model[BUFFER_SIZE];
		if (sysctlbyname("hw.model", &model, &len, nullptr, 0) == 0)
		{
			m_model = model;
			Util::Trim(m_model);
		}
		else
		{
			warn("Failed to get CPU model. Couldn't read 'hw.model'.");
		}

		m_arch = GetCPUArch();
	}

#endif

#ifdef __APPLE__
	void CPU::Init()
	{
		size_t len = BUFFER_SIZE;
		char buffer[BUFFER_SIZE];
		if (sysctlbyname("machdep.cpu.brand_string", &buffer, &len, nullptr, 0) == 0)
		{
			m_model = buffer;
			Util::Trim(m_model);
		}
		else
		{
			warn("Failed to get CPU model. Couldn't read 'machdep.cpu.brand_string'.");
		}

		m_arch = GetCPUArch();
	}
#endif

#ifndef WIN32
	std::string CPU::GetCPUArch() const
	{
		std::string cmd = std::string("uname -m") + Util::NULL_ERR_OUTPUT;
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe)
		{
			warn("Failed to get CPU arch. Couldn't read 'uname -m'.");

			return "";
		}

		char buffer[BUFFER_SIZE];
		if (fgets(buffer, BUFFER_SIZE, pipe))
		{
			pclose(pipe);
			Util::Trim(buffer);
			return buffer;
		}

		pclose(pipe);

		warn("Failed to get CPU arch.");

		return "";
	}
#endif
}
