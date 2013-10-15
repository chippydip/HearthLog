#include <wx/stdpaths.h>
#include <wx/log.h>

#include "Helper.h"

const std::string &Helper::AppVersion()
{
	static const std::string version = "v0.2.1";
	return version;
}

wxFileName Helper::GetUserDataDir()
{
	return wxFileName(wxStandardPaths::Get().GetUserDataDir(), "");
}

#ifdef WIN32
#include <windows.h>
#pragma comment(lib, "version.lib")
std::uint64_t Helper::GetHearthstoneVersion()
{
	auto dir = ReadConfig("HearthstoneDir", wxString("C:\\Program Files (x86)\\Hearthstone"));
	wxFileName file(dir, "Hearthstone.exe");
	auto path = file.GetFullPath();
	if (!file.Exists()) {
		wxLogError("couldn't find %s", path);
		return 0;
	}
	
	DWORD verHandle = NULL;
	DWORD verSize   = GetFileVersionInfoSize(path.t_str(), &verHandle);
	if (!verSize) {
		wxLogError("GetFileVersionInfoSize(%s): %d", path, GetLastError());
		return 0;
	}

	LPSTR  verData  = new char[verSize];
	UINT   size     = 0;
	LPBYTE lpBuffer = NULL;
	if (!GetFileVersionInfo(path.t_str(), verHandle, verSize, verData)) {
		wxLogError("GetFileVersionInfo(%s): %d", path, GetLastError());
	} else if (!VerQueryValue(verData, L"\\", (LPVOID*)&lpBuffer, &size)) {
		wxLogError("VerQueryValue(%s): %d", path, GetLastError());
	} else if (!size) {
		wxLogError("no version info", path, GetLastError());
	} else {
		VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
		if (verInfo->dwSignature != 0xfeef04bd) {
			wxLogError("bad version signature (%s)", path);
		} else {
			auto version = (uint64_t)verInfo->dwFileVersionMS << 32 | verInfo->dwFileVersionLS;
			wxLogVerbose("%s v%u.%u.%u.%u", path, 
				(uint16_t)(version >> 48),
				(uint16_t)(version >> 32),
				(uint16_t)(version >> 16),
				(uint16_t)(version));
			return version;
		}
	}
	delete[] verData;

	return 0;
}
#endif
