#include "bitstream_reader.hpp"

template<>
std::string unpack_from<std::string>(BitstreamReader& bitpack)
{
    std::string str;
    char character;
    while(true) {
        character = bitpack.unpack<char>();
        if(character == '\0')
            return str;

        str.push_back(character);
    }
}

template<>
bool unpack_from<bool>(BitstreamReader& bitpack)
{
    return bitpack.next_bit();
}
