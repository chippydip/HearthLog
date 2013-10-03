#pragma once

#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>

namespace tcp {

class Endpoint
{
public:
	Endpoint() { } // = default;
	Endpoint(std::string ip, uint16_t port) : _ip(std::move(ip)), _port(port) { }

	const std::string &Ip() const { return _ip; }
	uint16_t Port() const { return _port; }

private:
	std::string _ip;
	uint16_t _port;

	friend std::ostream &operator<<(std::ostream &out, const Endpoint &endpoint)
	{ return out << '[' << endpoint._ip << "]:" << endpoint._port; }
};

class EndpointPair
{
public:
	EndpointPair() : _src(), _dst() { } // = default;
	EndpointPair(Endpoint src, Endpoint dst) : _src(std::move(src)), _dst(std::move(dst)) { }

	const Endpoint &Src() const { return _src; }
	const Endpoint &Dst() const { return _dst; }

	std::string SrcToDst(const std::string &sep = "->") const { return ToString(_src, sep, _dst); }
	std::string DstToSrc(const std::string &sep = "->") const { return ToString(_dst, sep, _src); }

private:
	Endpoint _src;
	Endpoint _dst;

	static std::string ToString(const Endpoint &first, const std::string &sep, const Endpoint &second)
	{
		std::ostringstream oss;
		oss << first << sep << second;
		return oss.str();
	}
};

} // namespace tcp
