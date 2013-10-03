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

IMPLEMENT_APP(HearthLogApp)

TaskBarIcon *icon;

bool HearthLogApp::OnInit()
{
	auto log = new wxLogWindow(NULL, _("Log"), false, false);
	log->GetFrame()->SetSize(1024, 300);
	wxLog::SetActiveTarget(log);
	wxLog::SetVerbose();
	wxLogMessage(_("Hearth Log v0.1"));

	wxConfig::Set(new wxFileConfig("", "", "config.ini", "", wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_SUBDIR));

	// Locate the Hearthstone directory
	while (!Helper::GetHearthstoneVersion()) {
		auto dir = wxDirSelector(_("Hearth Log: Please locate your Hearthstone directory (Usually C:\\Program Files (x86)\\Hearthstone)"));
		if (dir.empty()) {
			return false;
		}
		Helper::WriteConfig("HearthstoneDir", dir);
	}

	icon = new TaskBarIcon();

	//gFrame->Show();
	SetTopWindow(log->GetFrame());

	PacketCapture::Start("tcp port 3724", 
	//PacketCapture::Start("tcp port 1119", "C:\\Users\\Chip\\Documents\\Network Monitor 3\\Captures\\Hearthstone2.pcap", 
		[]() -> PacketCapture::Callback::Ptr {
			return std::make_unique<tcp::Parser>(
				[](int64_t nanotime, tcp::Stream *stream) -> tcp::Parser::Callback::Ptr {
					return std::make_unique<GameLogger>(nanotime, stream);
				});
		});

	icon->UploadAll();

	return true;
}

void HearthLogApp::UploadLog(const wxString &filename)
{
	wxCommandEvent evt(HSL_LOG_AVAILABLE_EVENT);
	evt.SetString(filename);

	wxPostEvent(icon, evt);
}
