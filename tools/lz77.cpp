#include "lz77.hpp"

std::vector<uint8_t> decode_lz77(const uint8_t*& it)
{
    std::vector<uint8_t> decompressed_bytes;
    uint8_t cmd;
    uint8_t cmd_remaining_bits = 0;

    while(true)
    {
        if(!cmd_remaining_bits)
        {
            cmd = *it;
            cmd_remaining_bits = 8;
            it += 1;
        }

        uint8_t next_cmd_bit = (cmd & 0x80);
        cmd <<= 1;
        cmd_remaining_bits -= 1;

        if(next_cmd_bit)
        {
            decompressed_bytes.emplace_back(*it);
            it += 1;
        }
        else
        {
            uint8_t byte_1 = *it;
            it += 1;
            uint8_t byte_2 = *it;
            it += 1;

            uint16_t offset = (byte_1 & 0xF0) << 4 | byte_2;
            uint8_t length = 18 - (byte_1 & 0x0F);
            if(offset)
            {
                while(length != 0)
                {
                    uint8_t byte_to_copy = decompressed_bytes[decompressed_bytes.size() - offset];
                    decompressed_bytes.emplace_back(byte_to_copy);
                    length -= 1;
                }
            }
            else break;
        }
    }

    return decompressed_bytes;
}

