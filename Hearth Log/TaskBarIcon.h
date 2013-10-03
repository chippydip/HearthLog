#pragma once

#include <wx/event.h>
#include <wx/taskbar.h>

wxDECLARE_EVENT(HSL_LOG_AVAILABLE_EVENT, wxCommandEvent);

class wxFrame;

class TaskBarIcon : public wxTaskBarIcon
{
public:
	TaskBarIcon();

	void OnQuit(wxCommandEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnLog(wxCommandEvent &event);
	void OnUploadKey(wxCommandEvent &event);

	void OnLogSaved(wxCommandEvent &event);

	void UploadAll();

protected:
	virtual wxMenu *CreatePopupMenu();

private:
	wxFrame *_frame;

	DECLARE_EVENT_TABLE();
};
