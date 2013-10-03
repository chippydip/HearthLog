// wx #includes must come first to prevent secure function warning from wxcrt.h
#include <wx/log.h>

#include "Stream.h"

const std::vector<const uint8_t> EMPTY_VECTOR;

tcp::Stream::Stream(Parser *parser, const EndpointPair &endpoints, Stream *other, int64_t nanotime, uint32_t seq)
	: _parser(parser),
	  _endpoints(endpoints),
	  _other(other),
	  _firstSeq(seq),
	  _nextSeq(seq + 1),
	  _cache(),
	  _callback(parser->Factory()(nanotime, this))
{
	// Link other stream
	if (_other) {
		wxCHECK2(!_other->_other, return);

		wxLogVerbose("pairing %s with %s", _endpoints.SrcToDst(), _other->_endpoints.SrcToDst());
		_other->_other = this;
	}
}

tcp::Stream::~Stream()
{
	if (_other) {
		_other->_other = nullptr;
		_other = nullptr;
	}
}

void tcp::Stream::Add(int64_t nanotime, uint32_t seq, std::range<const uint8_t *> data)
{
	wxCHECK2(data.size() > 0, return);

	auto offset = int32_t(seq - _nextSeq);
	if (offset < 0) {
		// Duplicate packet that's already been processed (ignore)
		wxLogVerbose("%s dropping duplicate segment: seq=%d, next=%d, size=%d", _endpoints.SrcToDst(), seq, _nextSeq, data.size());
		return;
	}

	if (seq == _nextSeq) {
		(*_callback)(nanotime, data);
		_nextSeq += data.size();

		// Check cache for additional data
		while (1) {
			auto it = _cache.find(_nextSeq);
			if (it == _cache.end()) {
				return; // wait for more frames
			}

			if (it->second.size() <= 0) {
				Close(nanotime, _nextSeq);
				return;
			}

			auto &v = it->second;
			(*_callback)(nanotime, std::make_range(v.data(), v.data() + v.size()));
			_nextSeq += it->second.size();

			_cache.erase(it);
		}
	}

	auto current = _cache.find(seq);
	if (current != _cache.end()) {
		// There's already data stored there (duplicate packet?)
		wxLogVerbose("%s dropping duplicate segment: seq=%d, size=%d)", _endpoints.SrcToDst(), seq, current->second.size());
		if (current->second.size() != data.size()) {
			wxLogWarning("%s duplicate frames not the same size: seq=%d (%d vs %d)", _endpoints.SrcToDst(), seq, current->second.size(), data.size());
		}
		// TODO: could verify that the data is the same as well
		return;
	}

	// Data out of order so save it for later
	_cache.emplace(seq, std::vector<const uint8_t>(data.begin(), data.end()));
}

void tcp::Stream::Close(int64_t nanotime, uint32_t seq)
{
	if (seq != _nextSeq) {
		// Mark the end of the stream, but wait for missing data
		auto r = _cache.emplace(seq, EMPTY_VECTOR);
		if (!r.second) {
			// Already a frame in the cache there...
			if (r.first->second.empty()) {
				wxLogVerbose("%d duplicate FIN: seq=%d", _endpoints.SrcToDst(), seq);
				return; // just ignore it
			} else {
				wxLogError("%s FIN seq matches existing data: seq=%d, size=%d", _endpoints.SrcToDst(), seq, r.first->second.size());
				// Shouldn't happen, so go ahead and close the stream anyway (below)
			}
		}
	}

	// Close right now
	if (_other) {
		// Unlink if this is the first stream in the pair to FIN
		_other->_other = nullptr;
		_other = nullptr;
	}
	_parser->Remove(this); // NB: this will be invalid once this call returns!
}
