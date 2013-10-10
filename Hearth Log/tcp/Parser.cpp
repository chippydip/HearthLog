// wx #includes must come first to prevent secure function warning from wxcrt.h
#include <wx/log.h>

#include "Parser.h"
#include "Segment.h"
#include "Stream.h"

#include "../util.h"

tcp::Parser::Parser(Callback::Factory callbackFactory)
	: _streams(),
	  _callbackFactory(callbackFactory)
{
}

void tcp::Parser::operator()(int64_t nanotime, std::range<const uint8_t*> data)
{
	tcp::Segment segment(data);
	if (!segment.WasParsed() || segment.IsRst()) {
		// Try to reset/clear the TcpStream
		wxLogVerbose("%s: %s", segment.IsRst() ? "connection reset" : "segment parse error", segment.Endpoints().SrcToDst());
		_streams.erase(segment.Endpoints().SrcToDst());
		_streams.erase(segment.Endpoints().DstToSrc());
		return;
	}

	auto key = segment.Endpoints().SrcToDst();
	auto seq = segment.SeqNum();

	// Get the current stream or reserve space for a new one
	auto prevSize = _streams.size();
	auto &stream = _streams[key];

	if (segment.IsSyn()) {
		// This is a SYN packet, so create a new stream if there wasn't one already
		// or if this starting sequence number doesn't match.
		if (!stream || stream->FirstSeq() != seq) {
			// Get the reverse stream if it already exists
			auto it = _streams.find(segment.Endpoints().DstToSrc());
			auto other = it != _streams.end() ? it->second.get() : nullptr;

			// Create a new stream if there wasn't one or the starting sequence number didn't match
			stream = std::make_unique<Stream>(this, segment.Endpoints(), other, nanotime, seq);
		}
	} else {
		// Not a SYN packet, if this is the first time we've seen this connection
		// report that it will be ignored (map now contains a null Stream for that key).
		if (_streams.size() > prevSize) {
			wxLogVerbose("ignoring %s (no SYN)", key);
		}

		// In any case, stop now if this stream is being ignored (null Stream).
		if (!stream) {
			// Stop ignoring if this is a FIN packet. This isn't strictly needed
			// (if a SYN is seen a new Stream will be created) and it may not always
			// work (if data is seen after the FIN a null will be re-inserted) but
			// most of the time this should work to keep only active connections in
			// this map to save space for long-running programs.
			if (segment.IsFin()) {
				_streams.erase(key);
			}
			return;
		}
	}

	// Pass the data along for reassembly
	auto payload = segment.Payload();
	if (payload.size() > 0) {
		stream->Add(nanotime, seq, payload);
	}

	// Handle final packets
	if (segment.IsFin()) {
		stream->Close(nanotime, seq + payload.size()); // NB: stream may be invalid after this returns (usually calls Remove)
	}
}

void tcp::Parser::Remove(Stream *stream)
{
	_streams.erase(stream->Endpoints().SrcToDst());
}
