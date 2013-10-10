#pragma once

#include <cstdint>
#include "range.h"
#include <string>
#include <memory>

// From pcap.h
typedef struct pcap_if pcap_if_t;
typedef struct pcap pcap_t;

class PacketCapture
{
public:
	struct Callback
	{
		virtual void operator()(int64_t nanotime, std::range<const uint8_t*> data) = 0;
		virtual ~Callback() { }

		typedef std::unique_ptr<Callback> Ptr;
		typedef Ptr (*Factory)();
	};

	static void Start(const std::string &filter,                           Callback::Factory callbackFactory);
	static void Start(const std::string &filter, pcap_if_t *device,        Callback::Factory callbackFactory);
	static void Start(const std::string &filter, const std::string &file,  Callback::Factory callbackFactory);
	static void Start(const std::string &filter, pcap_t *pcap,             Callback::Factory callbackFactory, std::string deviceName = "");
};
