#pragma once

constexpr size_t SYMBOL_COUNT = 108;

class SymbolCount {
private:
    uint8_t _symbol = 0xFF;
    uint32_t _count = 0;

public:
    SymbolCount() = default;

    SymbolCount(uint8_t symbol, uint32_t count) : _symbol(symbol), _count(count)
    {}

    [[nodiscard]] uint8_t symbol() const { return _symbol; }
    [[nodiscard]] uint32_t count() const { return _count; }

    bool operator < (const SymbolCount& b) const
    {
        if(_count > b._count)
            return true;
        else if(_count == b._count)
            return _symbol < b._symbol;
        return false;
    }
};

namespace Symbols 
{
    const char TABLE[] = {
        ' ',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '*', '.', ',', '?', '!', '/', '<', '>', ':', '-', '\'', '\"', '%', '#', '&', '(', ')', '=',
        '\x10', // Upper Left Arrow
        '\x11', // Upper Right Arrow
        '\x12', // Bottom Right Arrow
        '\x13', // Bottom Left Arrow
        '\x02', // (START/EOL)
        '\x18', // {CURSOR} = Blinking arrow
        '\x0A', // {LF} = Line Feed
        '\x19', // Yes/No
        '\x1A', // {DELAY_LONG} = Wait
        '\x1B', // {NUMBER} = Insert numeric value
        '\x1C', // {CHARNAME} = Character name
            '\0', // unused
            '\0', // unused
        '\x03', // {EOL} = Sentence End
        '\x1D', // {ITEM} Object Name
            '\0', // unused
            '\0', // unused
        '\x1E', // {CLR} = Clear Panel
            '\0', // unused
        '\x1F', // {DELAY_SHORT} = Wait
            '\0',
            '\0',
            '\0',
            '\0',
        '\x69', // Line break with hyphen in inventory, no space in textboxes
        '\x6A', // Line break without hyphen in inventory, space in textboxes
        '\x6B', // Line break without hyphen in inventory, no space in textboxes
    };

    inline uint8_t byte_for_symbol(char symbol)
    {
        for(uint8_t i=0 ; i<SYMBOL_COUNT ; ++i)
            if(TABLE[i] == symbol)
                return i;
        return 0xFF;
    }

    inline std::vector<uint8_t> bytes_for_symbols(const std::string& symbol_string)
    {
        std::vector<uint8_t> ret;
        ret.reserve(symbol_string.size());
        for(char c : symbol_string)
            ret.emplace_back(byte_for_symbol(c));
        return ret;
    }

    inline std::string parse_from_bytes(const std::vector<uint8_t>& bytes)
    {
        std::string ret;
        for(uint8_t byte : bytes)
        {
            if(byte == 0x69 || byte == 0x6B)
                continue;
            else if(byte == 0x6A)
                ret += ' ';
            else
                ret += TABLE[byte];
        }

        return ret;
    }
}