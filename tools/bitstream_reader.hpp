#pragma once

#include <vector>
#include "../exceptions.hpp"

class BitstreamReader {
private:
    const uint8_t* _data;
    int8_t _bit_position = 7;

public:
    explicit BitstreamReader(const std::vector<uint8_t>& data) : _data(&(data[0]))
    {}

    explicit BitstreamReader(const uint8_t* data) : _data(data)
    {}

    bool next_bit()
    {
        if(_bit_position < 0)
        {
            // First byte was fully read already, reset bit position and go onto next byte
            _bit_position = 7;
            _data++;
        }

        bool first_bit = (*_data >> _bit_position) & 0x01;
        _bit_position -= 1;

        return first_bit;
    }

    uint32_t read_bits(uint8_t amount)
    {
        if(amount > 32)
            throw LandstalkerException("Cannot read more than 32 bits using Bitstream");

        uint32_t value = 0;
        for(uint8_t i=0 ; i<amount ; ++i)
        {
            value <<= 1;
            value |= this->next_bit() ? 1 : 0;
        }

        return value;
    }

    /**
     * Gets compressed variable-width number. Number is in the form 2^Exp + Man
     * Exp is the number of leading zeroes. The following bits make up the
     * mantissa. The same number of bits make up the exponent and mantissa
     */
    uint32_t read_variable_length_number()
    {
        uint16_t exponent = 0;
        while(!this->next_bit())
            exponent += 1;

        uint32_t mantissa = this->read_bits(exponent);
        return (1 << exponent) + mantissa - 1;
    }
};
