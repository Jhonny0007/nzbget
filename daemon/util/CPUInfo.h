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

#ifndef CPU_INFO_H
#define CPU_INFO_H

#include <string>

namespace SystemInfo
{
	class CPUInfo final
	{
	public:
		CPUInfo();
		const std::string& GetModel() const;
		const std::string& GetArch() const;

	private:
		void Init();

#ifndef WIN32
		std::string GetCPUArch() const;
#endif

		std::string m_model;
		std::string m_arch;
	};
}

#endif
