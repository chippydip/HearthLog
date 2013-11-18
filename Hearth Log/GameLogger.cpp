// wx #includes must come first to prevent secure function warning from wxcrt.h
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>

#include "HearthLogApp.h"
#include "Helper.h"

#include "GameLogger.h"

#include <algorithm>

template <typename T> void swap_clear(T &v) { if (!v.empty()) { T x; v.swap(x); } }

class GameLogger::Log
{
	typedef std::vector<uint8_t> Bytes;
	typedef std::pair<int64_t, Bytes> Message;
	typedef std::vector<Message> MessageList;

public:
	Log(std::string name, int64_t nanotime)
		: _name(std::move(name))
	{
		wxLogVerbose("%lld %s logging", nanotime, _name);
		_messages.emplace_back(nanotime, Bytes());
	}

	~Log()
	{
		if (_messages.size() <= 1) {
			return;
		}

		// Build the file name for storing this game
		auto file = Helper::GetUserDataDir();
		file.SetFullName(wxString::Format("%i.hsl", int(_messages[0].first / int(1e9))));
		file.AppendDir("Logged");
		auto filename = file.GetFullPath();
		wxLogVerbose("saving %d messages to %s", _messages.size() - 1, filename);

		// Create the containing directory if needed
		if (!file.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
			wxLogError("error creating save directory: %s", filename);
			return;
		}

		// This should't happen, but check and log just in case
		if (file.Exists()) {
			wxLogWarning("overwriting existing game: %s", filename);
		}

		// Open the file
		wxFileOutputStream fout(file.GetFullPath());
		if (!fout.Ok()) {
			wxLogError("error opening file: %s", filename);
			return;
		}

		// Zip the data while saving it to save some bandwidth later when the file is uploaded
		wxZlibOutputStream zout(fout, wxZ_BEST_COMPRESSION, wxZLIB_NO_HEADER);

		// Add header info
		// <nanotime>
		// 48 53 4C 48 09 00 00 00 
		// 09 XX XX XX XX XX XX XX XX
		auto version = Helper::GetHearthstoneVersion();
		zout.Write(&_messages[0].first, 8)
			.Write("HSLH\t\0\0\0\t", 9) // HSLH 09000000 09
			.Write(&version, 8);

		// Add the rest of the data
		auto size = 25;
		for (auto i = 1u; i < _messages.size(); i++) {
			auto time = _messages[i].first;
			auto msg = _messages[i].second;

			size += 8 + msg.size();
			zout.Write(&time, 8).Write(msg.data(), msg.size());
		}
		zout.Close();
		wxLogVerbose("saved %d messages from %s (%d bytes, %lld compressed)", _messages.size() - 1, _name, size, fout.GetLength());

		// Notify the app that it can upload the log file
		HearthLogApp::UploadLog(filename);
	}

	void Add(int64_t nanotime, Bytes message)
	{
		if (WasCanceled()) {
			// Cancel was called previously, so don't store messages anymore
			return;
		}

		_messages.emplace_back(nanotime, message);

		auto header = reinterpret_cast<int32_t *>(message.data());
		wxLogVerbose("%lld %s (%d, %d)", nanotime, _name, header[0], header[1]);
	}

	void Cancel()
	{
		swap_clear(_messages); // clear and release memory
	}

	bool WasCanceled()
	{
		return _messages.empty();
	}

private:
	std::string _name;
	MessageList _messages;
};

GameLogger::GameLogger(int64_t nanotime, tcp::Stream *stream)
	: _stream(stream),
	  _header(),
	  _message(),
	  _buffer(_header.data(), _header.data() + _header.size()),
	  _log()
{
	//wxLogVerbose("new stream: %s", stream->Endpoints().SrcToDst());
	if (_stream->Other()) {
		_log = reinterpret_cast<GameLogger*>(_stream->Other()->Callback())->_log;
	} else {
		_log = std::make_shared<Log>(_stream->Endpoints().SrcToDst(), nanotime);
	}
}

GameLogger::~GameLogger()
{
	if (_buffer.begin() != _header.data()) {
		wxLogWarning("%s canceling log (stream closed mid-packet)", _stream->Endpoints().SrcToDst());
		_log->Cancel();
	}
	//wxLogVerbose("stream closed: (%s)", _stream->Endpoints().SrcToDst());
}

void GameLogger::operator()(int64_t nanotime, std::range<const uint8_t *> data) 
{
	if (_log->WasCanceled()) {
		swap_clear(_message);
		return;
	}

	while (!data.empty()) {
		auto toCopy = std::min(_buffer.size(), data.size());

		std::copy(data.begin(), data.begin() + toCopy, _buffer.begin());
		_buffer.pop_front(toCopy);
		data.pop_front(toCopy);

		if (_buffer.empty()) {
			if (_buffer.end() == _header.data() + _header.size()) {
				// Done reading header, parse it
				auto ptr = reinterpret_cast<uint32_t *>(_header.data());
				auto type = ptr[0];
				auto size = ptr[1];

				// Sanity check the values
				if (type > 1000 || size > 8000) {
					wxLogVerbose("%s canceling log (bad header: %d, %d)", _stream->Endpoints().SrcToDst(), type, size);
					_log->Cancel();
					swap_clear(_message);
					_buffer = std::make_range(_header.data(), _header.data() + _header.size());
					return;
				}

				// Allocate space for the message and copy the header to the start
				_message.resize(8 + size);
				std::copy(_header.data(), _header.data() + 8, _message.data());
				_buffer = std::make_range(_message.data() + 8, _message.data() + _message.size());

			} else {
				// Done reading message, add it to the log
				_log->Add(nanotime, std::move(_message));

				// Setup for another header next
				_buffer = std::make_range(_header.data(), _header.data() + _header.size());
			}
		}
	}
	//wxLogVerbose("packet: %d (%s)", data.size(), _stream->Endpoints().SrcToDst());
}
