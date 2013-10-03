#pragma once

#include "Endpoint.h"

#include <cstdint>
#include "../range.h"

namespace tcp {

class Segment
{
public:
	Segment(std::range<const uint8_t *> frame);

	const EndpointPair &Endpoints() const { return _endpoints; }

	const Endpoint &Src() const { return _endpoints.Src(); }
	const Endpoint &Dst() const { return _endpoints.Dst(); }

	uint32_t SeqNum() const { return _seq; }

	bool IsSyn() const { return (_flags & TH_SYN) != 0; }
	bool IsFin() const { return (_flags & TH_FIN) != 0; }
	bool IsRst() const { return (_flags & TH_RST) != 0; }

	bool WasParsed() const { return _ok; }

	std::range<const uint8_t *> Payload() const { return _payload; }

private:
	EndpointPair _endpoints;
	uint32_t _seq;
	uint8_t _flags;
	bool _ok;
	std::range<const uint8_t *> _payload;

	// From pcap_tcp.h so we can inline the flag checking functions
	enum {
		TH_FIN = 0x01,
		TH_SYN = 0x02,
		TH_RST = 0x04,
		TH_ACK = 0x10,
	};
};

} // namespace tcp
