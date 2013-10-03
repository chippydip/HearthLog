#pragma once

#include "../PacketCapture.h"

#include <cstdint>
#include "../range.h"
#include <map>
#include <memory>

namespace tcp {

class Stream;

class Parser : public PacketCapture::Callback
{
public:
	struct Callback
	{
		virtual void operator()(int64_t nanotime, std::range<const uint8_t*> data) = 0;
		virtual ~Callback() { }

		typedef std::unique_ptr<Callback> Ptr;
		typedef Ptr (*Factory)(int64_t, Stream*);
	};

	explicit Parser(Callback::Factory callbackFactory);

	virtual void operator()(int64_t nanotime, std::range<const uint8_t*> data);

	Callback::Factory Factory() const { return _callbackFactory; }

	void Remove(Stream *stream);

private:
	std::map<std::string, std::unique_ptr<Stream>> _streams;
	const Callback::Factory _callbackFactory;
};

} // namespace tcp
