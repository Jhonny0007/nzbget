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

#include <regex>
#include <boost/version.hpp>

#include "SystemInfo.h"
#include "Options.h"
#include "FileSystem.h"
#include "Log.h"
#include "Json.h"
#include "Xml.h"

namespace SystemInfo
{
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

	const size_t BUFFER_SIZE = 512;

	SystemInfo::SystemInfo()
	{
		InitLibVersions();
	}

	const std::vector<Library>& SystemInfo::GetLibraries() const
	{
		return m_libraries;
	}

	void SystemInfo::InitLibVersions()
	{
		m_libraries.reserve(4);
		m_libraries.push_back({ "LibXML2", LIBXML_DOTTED_VERSION });

#if defined(HAVE_NCURSES_H) || defined(HAVE_NCURSES_NCURSES_H)
		m_libraries.push_back({ "ncurses", NCURSES_VERSION });
#endif

#ifndef DISABLE_GZIP
		m_libraries.push_back({ "Gzip", ZLIB_VERSION });
#endif

#ifdef HAVE_OPENSSL
		m_libraries.push_back({ "OpenSSL", OPENSSL_FULL_VERSION_STR });
#endif

#ifdef HAVE_LIBGNUTLS
		m_libraries.push_back({ "GnuTLS", GNUTLS_VERSION });
#endif

#ifdef BOOST_LIB_VERSION
		m_libraries.push_back({ "Boost", BOOST_LIB_VERSION });
#endif
	}

	const CPU& SystemInfo::GetCPUInfo() const
	{
		return m_cpu;
	}

	const OS& SystemInfo::GetOSInfo() const
	{
		return m_os;
	}

	std::vector<Tool> SystemInfo::GetTools() const
	{
		std::vector<Tool> tools{
			GetPython(),
			GetSevenZip(),
			GetUnrar(),
		};

		return tools;
	}
	std::string SystemInfo::ParseUnpackerVersion(const std::string& line) const
	{
		// e.g. 7-Zip (a) 19.00 (x64) : Copyright (c) 1999-2018 Igor Pavlov : 2019-02-21
		// e.g. UNRAR 5.70 x64 freeware      Copyright (c) 1993-2019 Alexander Roshal
		std::regex pattern(R"([0-9]*\.[0-9]*)"); // float number
		std::smatch match;
		if (std::regex_search(line, match, pattern))
		{
			return match[0].str();
		}

		return std::string("");
	};

	Tool SystemInfo::GetPython() const
	{
		Tool tool;

		tool.name = "Python";
		auto result = Util::FindPython();
		if (!result.has_value())
		{
			return tool;
		}

		std::string cmd = FileSystem::EscapePathForShell(result.get()) + " --version" + Util::NULL_ERR_OUTPUT;
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe)
		{
			return tool;
		}

		char buffer[BUFFER_SIZE];
		if (fgets(buffer, BUFFER_SIZE, pipe) != nullptr)
		{
			// e.g. Python 3.12.3
			std::string version = buffer;
			Util::Trim(version);
			size_t pos = version.find(" ");
			if (pos != std::string::npos)
			{
				tool.version = version.substr(pos + 1);
			}
		}

		pclose(pipe);

		cmd = Util::FIND_CMD + result.get() + Util::NULL_ERR_OUTPUT;
		pipe = popen(cmd.c_str(), "r");
		if (!pipe)
		{
			return tool;
		}

		if (fgets(buffer, BUFFER_SIZE, pipe) != nullptr)
		{
			tool.path = buffer;
			Util::Trim(tool.path);
		}

		pclose(pipe);

		return tool;
	}

	Tool SystemInfo::GetUnrar() const
	{
		Tool tool;

		tool.name = "UnRAR";
		tool.path = GetUnpackerPath(g_Options->GetUnrarCmd());
		tool.version = GetUnpackerVersion(tool.path, "UNRAR");

		return tool;
	}

	Tool SystemInfo::GetSevenZip() const
	{
		Tool tool;

		tool.name = "7-Zip";
		tool.path = GetUnpackerPath(g_Options->GetSevenZipCmd());
		tool.version = GetUnpackerVersion(tool.path, tool.name.c_str());

		return tool;
	}

	std::string SystemInfo::GetUnpackerPath(const char* unpackerCmd) const
	{
		if (Util::EmptyStr(unpackerCmd))
		{
			return "";
		}

		std::string path = FileSystem::ExtractFilePath(unpackerCmd);

		auto result = FileSystem::GetFileRealPath(path);
		if (result.has_value() && FileSystem::FileExists(result.get().c_str()))
		{
			return result.get();
		}

		return "";
	}

	std::string SystemInfo::GetUnpackerVersion(const std::string& path, const char* marker) const
	{
		if (path.empty())
		{
			return "";
		}

		std::string cmd = FileSystem::EscapePathForShell(path) + Util::NULL_ERR_OUTPUT;
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe)
		{
			return "";
		}

		std::string version;
		char buffer[BUFFER_SIZE];
		while (!feof(pipe))
		{
			if (fgets(buffer, BUFFER_SIZE, pipe) != nullptr)
			{
				if (strstr(buffer, marker))
				{
					version = ParseUnpackerVersion(buffer);
					break;
				}
			}
		}

		pclose(pipe);

		return version;
	}

	Network SystemInfo::GetNetworkInfo() const
	{
		return GetNetwork();
	}

	std::string ToJsonStr(const SystemInfo& sysInfo)
	{
		Json::JsonObject json;
		Json::JsonObject osJson;
		Json::JsonObject networkJson;
		Json::JsonObject cpuJson;
		Json::JsonArray toolsJson;
		Json::JsonArray librariesJson;

		const auto& os = sysInfo.GetOSInfo();
		const auto& network = sysInfo.GetNetworkInfo();
		const auto& cpu = sysInfo.GetCPUInfo();
		const auto& tools = sysInfo.GetTools();
		const auto& libraries = sysInfo.GetLibraries();

		osJson["Name"] = os.GetName();
		osJson["Version"] = os.GetVersion();
		networkJson["PublicIP"] = network.publicIP;
		networkJson["PrivateIP"] = network.privateIP;
		cpuJson["Model"] = cpu.GetModel();
		cpuJson["Arch"] = cpu.GetArch();

		for (const auto& tool : tools)
		{
			Json::JsonObject toolJson;
			toolJson["Name"] = tool.name;
			toolJson["Version"] = tool.version;
			toolJson["Path"] = tool.path;
			toolsJson.push_back(std::move(toolJson));
		}

		for (const auto& library : libraries)
		{
			Json::JsonObject libraryJson;
			libraryJson["Name"] = library.name;
			libraryJson["Version"] = library.version;
			librariesJson.push_back(std::move(libraryJson));
		}

		json["OS"] = std::move(osJson);
		json["CPU"] = std::move(cpuJson);
		json["Network"] = std::move(networkJson);
		json["Tools"] = std::move(toolsJson);
		json["Libraries"] = std::move(librariesJson);

		return Json::Serialize(json);
	}

	std::string ToXmlStr(const SystemInfo& sysInfo)
	{
		xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST "value");
		xmlNodePtr structNode = xmlNewNode(nullptr, BAD_CAST "struct");
		xmlNodePtr osNode = xmlNewNode(nullptr, BAD_CAST "OS");
		xmlNodePtr networkNode = xmlNewNode(nullptr, BAD_CAST "Network");
		xmlNodePtr cpuNode = xmlNewNode(nullptr, BAD_CAST "CPU");
		xmlNodePtr toolsNode = xmlNewNode(nullptr, BAD_CAST "Tools");
		xmlNodePtr librariesNode = xmlNewNode(nullptr, BAD_CAST "Libraries");

		const auto& os = sysInfo.GetOSInfo();
		const auto& network = sysInfo.GetNetworkInfo();
		const auto& cpu = sysInfo.GetCPUInfo();
		const auto& tools = sysInfo.GetTools();
		const auto& libraries = sysInfo.GetLibraries();

		Xml::AddNewNode(osNode, "Name", "string", os.GetName().c_str());
		Xml::AddNewNode(osNode, "Version", "string", os.GetVersion().c_str());
		Xml::AddNewNode(networkNode, "PublicIP", "string", network.publicIP.c_str());
		Xml::AddNewNode(networkNode, "PrivateIP", "string", network.privateIP.c_str());
		Xml::AddNewNode(cpuNode, "Model", "string", cpu.GetModel().c_str());
		Xml::AddNewNode(cpuNode, "Arch", "string", cpu.GetArch().c_str());

		for (const auto& tool : tools)
		{
			Xml::AddNewNode(toolsNode, "Name", "string", tool.name.c_str());
			Xml::AddNewNode(toolsNode, "Version", "string", tool.version.c_str());
			Xml::AddNewNode(toolsNode, "Path", "string", tool.path.c_str());
		}

		for (const auto& library : libraries)
		{
			Xml::AddNewNode(librariesNode, "Name", "string", library.name.c_str());
			Xml::AddNewNode(librariesNode, "Version", "string", library.version.c_str());
		}

		xmlAddChild(structNode, osNode);
		xmlAddChild(structNode, networkNode);
		xmlAddChild(structNode, cpuNode);
		xmlAddChild(structNode, toolsNode);
		xmlAddChild(structNode, librariesNode);
		xmlAddChild(rootNode, structNode);

		std::string result = Xml::Serialize(rootNode);

		xmlFreeNode(rootNode);

		return result;
	}

}
