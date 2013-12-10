#include <wx/stdpaths.h>
#include <wx/log.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>

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

void logVersion(const wxString &path, std::uint64_t version)
{
	wxLogVerbose("%s v%u.%u.%u.%u", path, 
		(std::uint16_t)(version >> 48),
		(std::uint16_t)(version >> 32),
		(std::uint16_t)(version >> 16),
		(std::uint16_t)(version));
}

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "version.lib")
std::uint64_t Helper::GetHearthstoneVersion()
{
	// Check the config file for the location of Hearthstone.exe (or use the default)
	auto dir = ReadConfig("HearthstoneDir", wxString("C:\\Program Files (x86)\\Hearthstone"));

	// Make sure the .exe exists
	wxFileName file(dir, "Hearthstone.exe");
	auto path = file.GetFullPath();
	if (!file.Exists()) {
		wxLogError("couldn't find %s", path);
		return 0;
	}
	
	// Get the size of the FileVersionInfo object
	DWORD verHandle = NULL;
	DWORD verSize   = GetFileVersionInfoSize(path.t_str(), &verHandle);
	if (!verSize) {
		wxLogError("GetFileVersionInfoSize(%s): %d", path, GetLastError());
		return 0;
	}

	std::uint64_t version = 0;

	// Jump through the hoops to get the file's version info
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
			version = (uint64_t)verInfo->dwFileVersionMS << 32 | verInfo->dwFileVersionLS;
			logVersion(path, version);
			// still need to cleanup verData
		}
	}
	delete[] verData;

	return version;
}

bool Helper::FindHearthstone()
{
	// If Hearthstone.exe isn't in the default location, prompt until the user locates it's directory for us.
	while (!GetHearthstoneVersion()) {
		auto dir = wxDirSelector(_("Hearth Log: Please locate your Hearthstone directory (Usually C:\\Program Files (x86)\\Hearthstone)"));
		if (dir.empty()) {
			return false;
		}

		// Save the location for next time
		Helper::WriteConfig("HearthstoneDir", dir);
	}
	return true;
}
#endif

#ifdef __APPLE__
#include <wx/osx/core/cfstring.h>
#include <wx/tokenzr.h>
#include <CoreFoundation/CFURL.h>
#include <CoreFoundation/CFBundle.h>
std::uint64_t Helper::GetHearthstoneVersion()
{
	// Check the config file for the location of Hearthstone.app (or use the default)
	auto path = ReadConfig("HearthstoneApp", wxString("/Applications/Hearthstone/Hearthstone.app"));

	// Get the game version string from the app's info.plist
	// NB: Using wx wrapper classes for automatic memory management and to ease the wxString<->CFStringRef conversions
	wxCFStringRef cfPath(path);
	wxCFRef<CFURLRef> cfUrl(CFURLCreateWithFileSystemPath(nullptr, cfPath, kCFURLPOSIXPathStyle, false));
	wxCFRef<CFBundleRef> cfBundle(CFBundleCreate(nullptr, cfUrl));
	wxCFStringRef cfVersion((CFStringRef)CFBundleGetValueForInfoDictionaryKey(cfBundle, CFSTR("BlizzardFileVersion")));

	// Transition back to wx-land and then prevent the wrapper from deallocating the string (Get rule)
	auto versionStr = cfVersion.AsString();
	cfVersion.release();

	// Make sure the file version was found
	if (versionStr.empty()) {
		wxLogError("can't find BlizzardFileVersion in %s", path);
		return 0;
	}

	// Tokenize the version string
	wxStringTokenizer tokenizer(versionStr, ".");
	std::uint64_t version = 0;
	for (auto i = 0; i < 4; i++) {
		// Get the next token
		if (!tokenizer.HasMoreTokens()) {
			wxLogError("incomplete BlizzardFileVersion: %s", versionStr);
			return 0;
		}
		auto s = tokenizer.GetNextToken();

		// Parse it an an integer
		long n;
		if (!s.ToLong(&n)) {
			wxLogError("error parsing BlizzardFileVersion: %s", versionStr);
			return 0;
		}

		// Check the range
		if (n < 0 || 65535 < n) {
			wxLogError("BlizzardFileVersion value out of range: %ld", n);
			return 0;
		}

		// Update the packed version number
		version <<= 16;
		version += (std::uint16_t)n;
	}

	// Log the result and return
	logVersion(path, version);
	return version;
}

bool Helper::FindHearthstone()
{
	// If Hearthstone.app isn't in the default location, prompt until the user locates it for us.
	while (!GetHearthstoneVersion()) {
		auto app = wxFileSelector(_("Hearth Log: Please locate your Hearthstone game (Usually /Applications/Hearthstone/Hearthstone.app)"));
		if (app.empty()) {
			return false;
		}

		// Save the location for next time
		Helper::WriteConfig("HearthstoneApp", app);
	}
	return true;
}
#endif
