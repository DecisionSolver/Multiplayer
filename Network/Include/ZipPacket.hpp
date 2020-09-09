#pragma once
#include "Packet.hpp"
#include "LZ4/lz4.h"

namespace swl
{
	class ZipPacket : public Packet
	{
	protected:
		const void* onSend(uint32_t& size) override;
		void onReceive(const void* _data, const uint32_t& size) override;
	};
}