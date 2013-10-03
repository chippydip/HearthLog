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
	auto payload = segment.Payload();

	auto key = segment.Endpoints().SrcToDst();
	auto seq = segment.SeqNum();

	// Get the TcpStream for this packet, (re-)creating as needed for SYN packets.
	// If the connection is not found and this isn't a SYN packet, the data is dropped.
	std::unique_ptr<Stream> *streamPtr;
	if (segment.IsSyn()) {
		// Get the current stream or reserve space for a new one
		auto& s = _streams[key];
		if (!s || s->FirstSeq() != seq) {
			// Get the reverse stream if it already exists
			auto it = _streams.find(segment.Endpoints().DstToSrc());
			auto other = it != _streams.end() ? it->second.get() : nullptr;

			// Create a new stream if there wasn't one or the starting sequence number didn't match
			s = std::make_unique<Stream>(this, segment.Endpoints(), other, nanotime, seq);
		}
		streamPtr = &s;
	} else {
		// lookup stream
		auto it = _streams.find(key);
		if (it == _streams.end()) {
			if (payload.size() > 0) {
				wxLogVerbose("%s dropping %d bytes (no SYN)", key, payload.size());
			}
			return;
		}
		streamPtr = &it->second;
	}
	auto &stream = *streamPtr;

	// Pass the data along for reassembly
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
