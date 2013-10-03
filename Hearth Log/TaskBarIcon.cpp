// wx #includes must come first to prevent secure function warning from wxcrt.h
#include <wx/file.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include <wx/textdlg.h>
#include <wx/aboutdlg.h>
#include <wx/filename.h>
#include <wx/frame.h>
#include <wx/dir.h>

#include "icons/favicon-16x16-8.xpm"
#include "icons/favicon-32x32-8.xpm"
#include "icons/favicon-64x64-8.xpm"

#include "TaskBarIcon.h"
#include "Helper.h"

wxDEFINE_EVENT(HSL_LOG_AVAILABLE_EVENT, wxCommandEvent);

enum
{
	ID_Quit = 1,
	ID_About,
	ID_Log,
	ID_UploadKey,
};

BEGIN_EVENT_TABLE(TaskBarIcon, wxTaskBarIcon)
	EVT_MENU(ID_Quit, TaskBarIcon::OnQuit)
	EVT_MENU(ID_About, TaskBarIcon::OnAbout)
	EVT_MENU(ID_Log, TaskBarIcon::OnLog)
	EVT_MENU(ID_UploadKey, TaskBarIcon::OnUploadKey)
	EVT_COMMAND(wxID_ANY, HSL_LOG_AVAILABLE_EVENT, TaskBarIcon::OnLogSaved)
END_EVENT_TABLE()

TaskBarIcon::TaskBarIcon()
	: wxTaskBarIcon()
{
	SetIcon(wxIcon(favicon_16_16_8), _("Hearth Log"));

	_frame = ((wxLogWindow*)wxLog::GetActiveTarget())->GetFrame();
	_frame->SetIcon(wxIcon(favicon_32_32_8));
}

wxMenu *TaskBarIcon::CreatePopupMenu()
{
	auto menu = new wxMenu;

	menu->Append(ID_About, _("&About..."));
	menu->Append(ID_UploadKey, _("Upload &Key..."));
	menu->AppendSeparator();
	menu->Append(ID_Log, _("Show &Log..."));
	menu->AppendSeparator();
	menu->Append(ID_Quit, _("E&xit"));

	return menu;
}

void TaskBarIcon::OnQuit(wxCommandEvent& event)
{
	Destroy();
}

void TaskBarIcon::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo about;
	about.SetName("Hearth Log");
	about.SetVersion("v0.1");
	about.SetDescription(_("Hearthstone game logger."));
	about.SetCopyright("(C) 2013");
	about.SetWebSite("http://hearthlog.com");
	about.SetIcon(wxIcon(favicon_32_32_8));

	wxAboutBox(about);
}

void TaskBarIcon::OnLog(wxCommandEvent& event)
{
	wxLogWindow *log = (wxLogWindow*)wxLog::GetActiveTarget();
	
	log->Show(false);
	log->Show();
}

void TaskBarIcon::OnUploadKey(wxCommandEvent& event)
{
	auto key = Helper::ReadConfig("UploadKey", wxString());
	key = wxGetTextFromUser("", _("Upload Key"), key);
	if (!key.empty()) {
		Helper::WriteConfig("UploadKey", key);
	}

	UploadAll();
}

void TaskBarIcon::UploadAll()
{
	auto path = Helper::GetUserDataDir();
	path.AppendDir("Logged");
	if (!path.DirExists())
		return;

	wxDir dir(path.GetFullPath());
	if (!dir.IsOpened())
		return;

	wxCommandEvent evt(HSL_LOG_AVAILABLE_EVENT);
	wxString filename;
	auto cont = dir.GetFirst(&filename, "*.hsl", wxDIR_FILES);
	while (cont) {
		path.SetFullName(filename);
		evt.SetString(path.GetFullPath());
		wxPostEvent(this, evt);

		cont = dir.GetNext(&filename);
	}
}

void TaskBarIcon::OnLogSaved(wxCommandEvent &event)
{
	auto key = Helper::ReadConfig("UploadKey", wxString());
	if (key.empty()) {
		wxLogWarning("can't upload logs without an upload key");
		return; // Try later if an upload key is set
	}

	wxFileName filename(event.GetString());
	wxString src(filename.GetFullPath());

	// Open the file
	wxFile file(src);
	if (!file.IsOpened()) {
		wxLogError("couldn't open file: %s", src);
		return;
	}
	wxFileOffset size(file.Length());

	// Read contents into a memory buffer
	wxMemoryBuffer buffer(size);
	buffer.SetDataLen(size);
	if (file.Read(buffer.GetData(), buffer.GetDataLen()) != size) {
		wxLogError("didn't read the whole file: %s", src);
		file.Close();
		return;
	}
	file.Close();

	wxHTTP http;
	http.SetPostBuffer("application/hearthlog+deflate", buffer);
	http.SetTimeout(10); // 10 seconds of timeout instead of 10 minutes ...

	// Check config for development purposes
	auto host = "www.hearthlog.com";
	auto port = Helper::ReadConfig("localhost", 0L);
	if (port) {
		host = "localhost";
	} else {
		port = 80;
	}
 
	// this will wait until the user connects to the internet. It is important in case of dialup (or ADSL) connections
	while (!http.Connect(host, port))  // only the server, no pages here yet ...
		wxSleep(5);
 
	auto httpStream = http.GetInputStream("/upload?key=" + key);
 
	wxLogVerbose("upload: %d %s %d", http.GetResponse(), (httpStream ? "OK" : "FAILURE"), http.GetError());
 
	if (http.GetError() != wxPROTO_NOERR)
	{
		wxLogWarning("Unable to connect! %d", http.GetError());

		wxDELETE(httpStream);
		http.Close();
		return;
	}

	wxString res;
	wxStringOutputStream out_stream(&res);
	httpStream->Read(out_stream);
 
	wxLogVerbose(res);
 
	wxDELETE(httpStream);
	http.Close();

	// Move to the uploaded dir
	filename.RemoveLastDir();
	filename.AppendDir("Uploaded");
	auto dst = filename.GetFullPath();

	// Create the containing directory if needed
	if (!filename.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
		wxLogError("error creating save directory: %s", dst);
		return;
	}

	// This should't happen, but check and log just in case
	if (filename.Exists()) {
		wxLogWarning("overwriting existing game: %s", dst);
	}
	wxRenameFile(src, dst);
}
