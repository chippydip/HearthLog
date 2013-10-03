#pragma once

#include "Endpoint.h"
#include "Parser.h"

#include <cstdint>
#include "../range.h"
#include <map>
#include <memory>
#include <ostream>
#include <string>

namespace tcp {

class Parser;

class Stream
{
public:
	Stream(Parser *parser, const EndpointPair &endpoints, Stream *other, int64_t nanotime, uint32_t seq);
	~Stream();

	const EndpointPair &Endpoints() const { return _endpoints; }

	const Endpoint &Src() const { return _endpoints.Src(); }
	const Endpoint &Dst() const { return _endpoints.Dst(); }

	uint32_t FirstSeq() const { return _firstSeq; }

	void Add(int64_t nanotime, uint32_t seq, std::range<const uint8_t *> data);
	void Close(int64_t nanotime, uint32_t seq);

	Stream * const Other() { return _other; }
	Parser::Callback * const Callback() { return _callback.get(); }

private:
	Parser *const _parser;
	const EndpointPair _endpoints;
	Stream *_other;
	const uint32_t _firstSeq;
	uint32_t _nextSeq;
	std::map<uint32_t, const std::vector<const uint8_t>> _cache;

	// This should come last so its constructor is called last and destructor is called first
	const Parser::Callback::Ptr _callback;
};

} // namespace tcp
