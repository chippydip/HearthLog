// wx #includes must come first to prevent secure function warning from wxcrt.h
#include <wx/log.h>

#include "Segment.h"

#include "pcap_tcp.h"

tcp::Segment::Segment(std::range<const uint8_t *> frame)
{
	//-------------------------------------------------------------------------
	// Ethernet
	auto ether = reinterpret_cast<const ether_header *>(frame.begin());
	ptrdiff_t offset = ETHER_HDRLEN;

	if (offset > frame.size()) {
		wxLogError("truncated Ethernet header (%d bytes)", frame.size());
		return;
	}

	//-------------------------------------------------------------------------
	// IP
	std::string ipSrc, ipDst;
	u_int8_t ipPayloadType;
	ptrdiff_t ipPayloadLen;

	switch (ntohs(ether->ether_type)) {
	case ETHERTYPE_IP: { // IPv4
			// Check minimum header size before reading the actual length
			auto ip4HeaderLen = 20; // default (min) size
			if (offset + ip4HeaderLen > frame.size()) {
				wxLogError("truncated IPv4 header (%d bytes)", frame.size());
				return;
			}

			auto ipv4 = reinterpret_cast<const ip *>(frame.begin() + offset);

			// Parse out the info we care about
			ip4HeaderLen = IP_HL(ipv4) * 4;

			ipPayloadType = ipv4->ip_p;
			ipPayloadLen = ntohs(ipv4->ip_len) - ip4HeaderLen;

			ipSrc = inet_ntoa(ipv4->ip_src);
			ipDst = inet_ntoa(ipv4->ip_dst);

			// Check actual packet size
			offset += ip4HeaderLen;
			if (offset > frame.size()) {
				wxLogError("truncated IPv4 header (%d bytes)", frame.size());
				return;
			}
		}
		break;

	case ETHERTYPE_IPV6: {
			// TODO
			wxLogError("NYI: IPv6");
			return;
		}
		break;

	default:
		wxLogError("expected IP packet (ether_type: 0x%04x)", ntohs(ether->ether_type));
		return;
	}

	//-------------------------------------------------------------------------
	// TCP
	if (ipPayloadType != IPPROTO_TCP) {
		wxLogError("expected TCP packet (ip_proto: %d)", ipPayloadType);
		return;
	}

	// Check minimum header size before reading the actual length
	auto tcpHeaderLen = 20; // default (min) size
	if (offset + tcpHeaderLen > frame.size()) {
		wxLogError("truncated TCP header (%d bytes)", frame.size());
		return;
	}

	auto tcp = reinterpret_cast<const tcphdr *>(frame.begin() + offset);

	// Parse out the info we care about
	tcpHeaderLen = TH_OFF(tcp) * 4;

	_endpoints = EndpointPair(
		Endpoint(std::move(ipSrc), ntohs(tcp->th_sport)),
		Endpoint(std::move(ipDst), ntohs(tcp->th_dport))
	);

	_seq = ntohl(tcp->th_seq);
	_flags = tcp->th_flags;

	// Check actual packet size
	offset += tcpHeaderLen;
	if (offset > frame.size()) {
		wxLogError("truncated TCP header (%d bytes)", frame.size());
		return;
	}

	//-------------------------------------------------------------------------
	// Payload
	auto payloadLen = ipPayloadLen - tcpHeaderLen;

	if (offset + payloadLen > frame.size()) {
		wxLogError("truncated TCP payload (%d bytes)", frame.size());
		return;
	}

	_payload = frame.slice(offset, offset + payloadLen);

	_ok = true;
}
