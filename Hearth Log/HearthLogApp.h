#pragma once

#include <wx/app.h>

class HearthLogApp : public wxApp
{
public:
	static void UploadLog(const wxString &filename);

	virtual bool OnInit();
};
