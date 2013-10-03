#pragma once

#include "tcp/Parser.h"
#include "tcp/Stream.h"

#include <array>
#include <memory>
#include "range.h"
#include <vector>

class GameLogger : public tcp::Parser::Callback
{
public:
	GameLogger(int64_t nanotime, tcp::Stream *stream);
	virtual ~GameLogger();

	virtual void operator()(int64_t nanotime, std::range<const uint8_t *> data);

private:
	tcp::Stream * const _stream;

	std::array<uint8_t, 8> _header;
	std::vector<uint8_t> _message;
	std::range<uint8_t *> _buffer;

	class Log;
	std::shared_ptr<Log> _log;
};
