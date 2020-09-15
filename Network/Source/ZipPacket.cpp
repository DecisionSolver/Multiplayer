#include "ZipPacket.hpp"

namespace swl
{
    //const void* ZipPacket::onSend(uint32_t& size)
    //{
    //    char* outData = new char[getSize()+2];
    //    size = LZ4_compress_default((const char*)getData(), outData, getSize(), getSize()+2);
    //    return outData;
    //}

    //void ZipPacket::onReceive(const void* _data, const uint32_t& size)
    //{
    //    char* outData = new char[size*2];
    //    uint32_t outSize = LZ4_decompress_safe((const char*)_data, outData, size, size*2);
    //    if (outSize > 0)
    //        append(outData, outSize);
    //}
}