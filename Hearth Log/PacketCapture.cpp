// wx #includes must come first to prevent secure function warning from wxcrt.h
#include <wx/log.h>
#include <wx/translation.h>

#include "PacketCapture.h"

#include <pcap.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <set>

std::set<std::string> deviceNames;
std::mutex mu;

int64_t toNanoTime(timeval ts) {
	const int64_t NSEC_PER_SEC = 1e9;
	const int64_t NSEC_PER_USEC = 1e3;

	return ts.tv_sec * NSEC_PER_SEC + ts.tv_usec * NSEC_PER_USEC;
}

void PacketCapture::Start(const std::string &filter, Callback::Factory callbackFactory)
{
	wxCHECK2(callbackFactory, return);

	// Start thread
	auto thread = std::thread([filter, callbackFactory]() {
		char errbuf[PCAP_ERRBUF_SIZE];

		// Continually scan interfaces to handle waking from sleep other reason 
		// an interface may be added (after possibly becoming invalid earlier).
		while (1) {
			// Get all devices
			pcap_if_t *alldevs;
			if (pcap_findalldevs(&alldevs, errbuf) == -1) {
				wxLogError("pcap_findalldevs: %s", errbuf);
				return;
			}
	
			// Enumerate all devices and start a thread for each one
			// we aren't already listening to.
			{
				std::lock_guard<std::mutex> lock(mu);
				for (auto dev = alldevs; dev != nullptr; dev = dev->next) {
					if (deviceNames.find(dev->name) == deviceNames.end()) {
						deviceNames.insert(dev->name);

						wxLogMessage("listening to %s (%s)", dev->name, dev->description);
						Start(filter, dev, callbackFactory);
					}
				}
			}

			// Done with the device list
			pcap_freealldevs(alldevs);

			// Wait a while until the next loop
			std::this_thread::sleep_for(std::chrono::seconds(60));
		}
	});

	// <thread> will be deleted once it completes
	thread.detach();
}

void PacketCapture::Start(const std::string &filter, pcap_if_t *device, Callback::Factory callbackFactory)
{
	wxCHECK2(device && callbackFactory, return);

	char errbuf[PCAP_ERRBUF_SIZE] = "";

	// Open device
	pcap_t *pcap = pcap_open_live(device->name, 65535, 0, 1000, errbuf);
	if (!pcap) {
		wxLogError("pcap_open_live(%s): %s", device->name, errbuf);
		return;
	} else if (errbuf[0] != '\0') {
		wxLogWarning("pcap_open_live(%s): %s", device->name, errbuf);
	}

	Start(filter, pcap, callbackFactory, device->name);
}

void PacketCapture::Start(const std::string &filter, const std::string &file, Callback::Factory callbackFactory)
{
	wxCHECK2(!file.empty() && callbackFactory, return);

	char errbuf[PCAP_ERRBUF_SIZE];

	// Open the file
	pcap_t *pcap = pcap_open_offline(file.c_str(), errbuf);
	if (!pcap) {
		wxLogError("pcap_open_offline(%s): %s", file, errbuf);
		return;
	}

	Start(filter, pcap, callbackFactory);
}

void PacketCapture::Start(const std::string &filter, pcap_t *pcap, Callback::Factory callbackFactory, std::string deviceName)
{
	wxCHECK2(pcap && callbackFactory, return);

	// Filter
	if (!filter.empty()) {
		bpf_program bpf;
		if (pcap_compile(pcap, &bpf, filter.c_str(), 1, 0) == -1) {
			wxLogError("pcap_compile(%s): %s", filter, pcap_geterr(pcap));
			return;
		}

		if (pcap_setfilter(pcap, &bpf) == -1) {
			wxLogError("pcap_setfilter(%s): %s", filter, pcap_geterr(pcap));
			return;
		}
	}

	// Start thread
	auto thread = std::thread([pcap, callbackFactory, deviceName]() {
		auto handler = [](uint8_t *user, const pcap_pkthdr *header, const uint8_t *packet) {
			if (header->caplen < header->len) {
				wxLogWarning("truncated packet (%d of %d bytes)", header->caplen, header->len);
				// Will likely fail during packet parsing (truncated payload)
			}

			auto&& time = toNanoTime(header->ts);
			auto&& data = std::make_range(packet, packet + header->caplen);

			(*(Callback*)user)(time, data);
		};

		Callback::Ptr callback = callbackFactory();
		// Read packets
		if (pcap_loop(pcap, -1, handler, (uint8_t*)callback.get()) < 0) {
			wxLogError("pcap_loop: %s", pcap_geterr(pcap));
		}

		wxLogWarning("pcap_loop exited");
		pcap_close(pcap);

		// We are no longer listening to this device, but if it becomes 
		// available again the outter loop will start a new thread.
		{
			std::lock_guard<std::mutex> lock(mu);
			deviceNames.erase(deviceName);
		}
	});

	// <thread> will be deleted once it completes
	thread.detach();
}
