// wx #includes must come first to prevent secure function warning from wxcrt.h
#include <wx/frame.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/dirdlg.h>

#include "HearthLogApp.h"

#include "Helper.h"
#include "TaskBarIcon.h"
#include "PacketCapture.h"
#include "tcp/Parser.h"
#include "GameLogger.h"

#include "util.h"
#include <fstream>

IMPLEMENT_APP(HearthLogApp)

TaskBarIcon *icon;

std::ofstream fout;

bool HearthLogApp::OnInit()
{
	// Build the path for a log file
	auto file = Helper::GetUserDataDir();
	file.SetFullName("log.txt");

	// Create the containing directory if needed
	if (!file.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
		wxLogError("error creating save directory: %s", file.GetPath());
		return false;
	}

	// Open a file for logging
	fout.open(file.GetFullPath().c_str().AsChar(), std::ofstream::out);

	// Setup the log file
	auto logFile = new wxLogStream(&fout);
	wxLog::SetActiveTarget(logFile);

	// Wrap the log file with a GUI window
	auto logWindow = new wxLogWindow(NULL, _("Log"), false, true);
	logWindow->GetFrame()->SetSize(1024, 300);
	wxLog::SetActiveTarget(logWindow);

	// Start logging
	wxLog::SetVerbose();
	wxLogMessage(_("Hearth Log %s"), Helper::AppVersion());

	// Setup config from file
	wxConfig::Set(new wxFileConfig("", "", "config.ini", "", wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_SUBDIR));

	// Locate the Hearthstone directory
	while (!Helper::GetHearthstoneVersion()) {
		auto dir = wxDirSelector(_("Hearth Log: Please locate your Hearthstone directory (Usually C:\\Program Files (x86)\\Hearthstone)"));
		if (dir.empty()) {
			return false;
		}
		Helper::WriteConfig("HearthstoneDir", dir);
	}

	// Create the GUI bits
	icon = new TaskBarIcon();

	// Setup a packet parsing stack
	PacketCapture::Start("tcp port 3724 or tcp port 1119", 
	//PacketCapture::Start("tcp port 1119", "C:\\Users\\Chip\\Documents\\Network Monitor 3\\Captures\\Hearthstone2.pcap", 
		[]() -> PacketCapture::Callback::Ptr {
			return std::make_unique<tcp::Parser>(
				[](int64_t nanotime, tcp::Stream *stream) -> tcp::Parser::Callback::Ptr {
					return std::make_unique<GameLogger>(nanotime, stream);
				});
		});

	// Try to upload any logs that haven't been uploaded yet
	icon->UploadAll();

	return true;
}

void HearthLogApp::UploadLog(const wxString &filename)
{
	wxCommandEvent evt(HSL_LOG_AVAILABLE_EVENT);
	evt.SetString(filename);

	wxPostEvent(icon, evt);
}
