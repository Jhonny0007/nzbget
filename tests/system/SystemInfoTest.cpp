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

#include <boost/test/unit_test.hpp>

#include "SystemInfo.h"
#include "FileSystem.h"
#include "Options.h"
#include "Log.h"
#include "DiskState.h"

Log* g_Log;
Options* g_Options;
DiskState* g_DiskState;

BOOST_AUTO_TEST_CASE(SystemInfoTest)
{
	BOOST_CHECK(0 == 0);
	Options::CmdOptList cmdOpts;
	cmdOpts.push_back("SevenZipCmd=7z");
	cmdOpts.push_back("UnrarCmd=unrar");
	Options options(&cmdOpts, nullptr);
	g_Options = &options;

	auto sysInfo = std::make_unique<SystemInfo::SystemInfo>();

	std::string jsonStrResult = SystemInfo::ToJsonStr(*sysInfo);
	std::string xmlStrResult = SystemInfo::ToXmlStr(*sysInfo);

	//BOOST_TEST_MESSAGE(jsonStrResult);
	BOOST_TEST_MESSAGE(xmlStrResult);

	std::string jsonStrExpected = "{\"OS\":{\"Name\":\"" + sysInfo->GetOSInfo().GetName() +
		"\",\"Version\":\"" + sysInfo->GetOSInfo().GetVersion() +
		"\"},\"CPU\":{\"Model\":\"" + sysInfo->GetCPUInfo().GetModel() +
		"\",\"Arch\":\"" + sysInfo->GetCPUInfo().GetArch() +
		"\"},\"Network\":{\"PublicIP\":\"" + sysInfo->GetNetworkInfo().publicIP +
		"\",\"PrivateIP\":\"" + sysInfo->GetNetworkInfo().privateIP +
		"\"},\"Tools\":[{\"Name\":\"" + sysInfo->GetTools()[0].name +
		"\",\"Version\":\"" + sysInfo->GetTools()[0].version +
		"\",\"Path\":\"" + "C:\\\\Users\\\\asus\\\\AppData\\\\Local\\\\Programs\\\\Python\\\\Python312\\\\python.exe" +
		"\"},{\"Name\":\"" + sysInfo->GetTools()[1].name +
		"\",\"Version\":\"" + sysInfo->GetTools()[1].version +
		"\",\"Path\":\"" + sysInfo->GetTools()[1].path +
		"\"},{\"Name\":\"" + sysInfo->GetTools()[2].name +
		"\",\"Version\":\"" + sysInfo->GetTools()[2].version +
		"\",\"Path\":\"" + sysInfo->GetTools()[2].path +
		"\"}],\"Libraries\":[{\"Name\":\"" + sysInfo->GetLibraries()[0].name +
		"\",\"Version\":\"" + sysInfo->GetLibraries()[0].version +
		"\"},{\"Name\":\"" + sysInfo->GetLibraries()[1].name +
		"\",\"Version\":\"" + sysInfo->GetLibraries()[1].version +
		"\"},{\"Name\":\"" + sysInfo->GetLibraries()[2].name +
		"\",\"Version\":\"" + sysInfo->GetLibraries()[2].version +
		"\"},{\"Name\":\"" + sysInfo->GetLibraries()[3].name +
		"\",\"Version\":\"" + sysInfo->GetLibraries()[3].version +
		"\"}]}";
	std::string xmlStrExpected = "<value><struct><OS><member><name>Name</name><value><string>" + sysInfo->GetOSInfo().GetName() +
		"</string></value></member>" +
		"<member><name>Version</name><value><string>" + sysInfo->GetOSInfo().GetVersion() +
		"</string></value></member></OS>" +
		"<CPU><member><name>Model</name><value><string>" + sysInfo->GetCPUInfo().GetModel() +
		"</string></value></member>" +
		"<member><name>Arch</name><value><string>" + sysInfo->GetCPUInfo().GetArch() +
		"</string></value></member></CPU>" +
		"<Network><member><name>PublicIP</name><value><string>" + sysInfo->GetNetworkInfo().publicIP +
		"</string></value></member>"
		"<member><name>PrivateIP</name><value><string>" + sysInfo->GetNetworkInfo().privateIP +
		"</string></value></member></Network>" +
		"<Tools><member><name>Name</name><value><string>" + sysInfo->GetTools()[0].name +
		"</string></value></member>" +
		"<member><name>Version</name><value><string>" + sysInfo->GetTools()[0].version +
		"</string></value></member>" +
		"<member><name>Path</name><value><string>" + sysInfo->GetTools()[0].path +
		"</string></value></member>" +
		"<member><name>Name</name><value><string>" + sysInfo->GetTools()[1].name +
		"</string></value></member>" +
		"<member><name>Version</name><value><string/>" + sysInfo->GetTools()[1].version +
		"</string></value></member>" +
		"<member><name>Path</name><value><string>" + sysInfo->GetTools()[1].path +
		"</string></value></member>" +
		"<member><name>Name</name><value><string>" + sysInfo->GetTools()[2].name +
		"</string></value></member>" +
		"<member><name>Version</name><value><string/>" + sysInfo->GetTools()[2].version +
		"</string></value></member>" +
		"<member><name>Path</name><value><string>" + sysInfo->GetTools()[2].path +
		"</string></value></member></Tools>" +
		"<Libraries><member><name>Name</name><value><string>" + sysInfo->GetLibraries()[0].name +
		"</string></value></member>" +
		"<member><name>Version</name><value><string/>" + sysInfo->GetLibraries()[0].version +
		"</string></value></member>" +
		"<member><name>Name</name><value><string>" + sysInfo->GetLibraries()[1].name +
		"</string></value></member>" +
		"<member><name>Version</name><value><string>" + sysInfo->GetLibraries()[1].version +
		"</string></value></member>" +
		"<member><name>Name</name><value><string>" + sysInfo->GetLibraries()[2].name +
		"</string></value></member>" +
		"<member><name>Version</name><value><string>" + sysInfo->GetLibraries()[2].version +
		"</string></value></member>" +
		"<member><name>Name</name><value><string>" + sysInfo->GetLibraries()[3].name +
		"</string></value></member>" +
		"<member><name>Version</name><value><string>" + sysInfo->GetLibraries()[3].version +
		"</string></value></member></Libraries></struct></value>";
	BOOST_TEST_MESSAGE("-----------------------");
	//BOOST_TEST_MESSAGE(jsonStrExpected);
	BOOST_TEST_MESSAGE(xmlStrExpected);

	//BOOST_CHECK(jsonStrResult == jsonStrExpected);
	BOOST_CHECK(xmlStrResult == xmlStrExpected);
}
