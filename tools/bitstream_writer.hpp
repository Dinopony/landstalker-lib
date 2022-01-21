#pragma once

#include <vector>
#include "../exceptions.hpp"

class BitstreamWriter {
private:
    std::vector<uint8_t> _bytes;
    int8_t _bit_position = 8;

public:
    BitstreamWriter() = default;

    [[nodiscard]] const std::vector<uint8_t>& bytes() const { return _bytes; }

    void add_bit(bool bit)
    {
        if(_bit_position == 8)
        {
            _bytes.emplace_back(0);
            _bit_position = 0;
        }

        uint8_t& last_byte = _bytes[_bytes.size()-1];
        uint8_t base_value = (bit) ? 1 : 0;
        last_byte |= (base_value << (7 - _bit_position));
        _bit_position += 1;
    }

    void add_bits(const std::vector<bool>& bits)
    {
        for(bool bit : bits)
            this->add_bit(bit);
    }

    void add_number(uint32_t value, int num_bits)
    {
        for(int bit = num_bits-1 ; bit >= 0 ; --bit)
            this->add_bit(value & (1 << bit));
    }

    void add_variable_length_number(uint32_t number)
    {
        uint16_t exponent = 0;
        uint32_t number_copy = number + 1;
        while(number_copy > 1) {
            exponent += 1;
            number_copy >>= 1;
        }
        uint32_t mantissa = number + 1 - (1 << exponent);

        for(uint16_t i=0 ; i<exponent ; ++i)
            this->add_bit(false);
        this->add_bit(true);
        this->add_number(mantissa, exponent);
    }
};
