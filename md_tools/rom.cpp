#include "rom.hpp"
#include "code.hpp"

#include <iostream>

namespace md {

ROM::ROM(const std::string& input_path) : _was_open(false)
{
    _byte_array.resize(0x200000);

    std::ifstream file(input_path, std::ios::binary);
    if (file.is_open())
    {
        file.read((char*)&(_byte_array[0]), 0x200000);
        file.close();
        _was_open = true;
    }
}

void ROM::set_byte(uint32_t address, uint8_t byte)
{
    if (address >= _byte_array.size())
        return;

    _byte_array[address] = byte;
}

void ROM::set_word(uint32_t address, uint16_t word)
{
    if (address >= _byte_array.size() - 1)
        return;

    this->set_byte(address, (word >> 8));
    this->set_byte(address + 1, (word & 0xFF));
}

void ROM::set_long(uint32_t address, uint32_t longWord)
{
    if (address >= _byte_array.size() - 3)
        return;

    this->set_word(address, (longWord >> 16));
    this->set_word(address + 2, (longWord & 0xFFFF));
}

std::vector<uint8_t> ROM::get_bytes(uint32_t begin, uint32_t end) const
{
    std::vector<uint8_t> output;
    output.reserve(end - begin);

    for (uint32_t addr = begin; addr < end; addr += 0x1)
        output.emplace_back(this->get_byte(addr));

    return output;
}

std::vector<uint16_t> ROM::get_words(uint32_t begin, uint32_t end) const
{
    std::vector<uint16_t> output;
    output.reserve(((end - begin) / 0x2) + 1);

    for (uint32_t addr = begin; addr < end; addr += 0x2)
        output.emplace_back(this->get_word(addr));

    return output;
}

std::vector<uint32_t> ROM::get_longs(uint32_t begin, uint32_t end) const
{
    std::vector<uint32_t> output;
    output.reserve(((end - begin) / 0x4) + 1);

    for (uint32_t addr = begin; addr < end; addr += 0x4)
        output.emplace_back(this->get_long(addr));

    return output;
}

void ROM::set_bytes(uint32_t address, std::vector<uint8_t> bytes)
{
    for (uint32_t i = 0; i < bytes.size(); ++i)
    {
        this->set_byte(address + i, bytes[i]);
    }
}

void ROM::set_bytes(uint32_t address, const unsigned char* bytes, size_t bytes_size)
{
    for (uint32_t i = 0; i < bytes_size; ++i)
    {
        this->set_byte(address + i, bytes[i]);
    }
}

void ROM::set_code(uint32_t address, const Code& code)
{
    this->set_bytes(address, code.get_bytes());
}

uint32_t ROM::inject_bytes(const std::vector<uint8_t>& bytes, const std::string& label)
{
    uint32_t size_to_inject = (uint32_t)bytes.size();
    uint32_t injection_addr = this->reserve_data_block(size_to_inject, label);
    this->set_bytes(injection_addr, bytes);
    return injection_addr;
}

uint32_t ROM::inject_bytes(const unsigned char* bytes, size_t size_to_inject, const std::string& label)
{
    uint32_t injection_addr = this->reserve_data_block(static_cast<uint32_t>(size_to_inject), label);
    this->set_bytes(injection_addr, bytes, size_to_inject);
    return injection_addr;
}

uint32_t ROM::inject_code(const Code& code, const std::string& label)
{
    return this->inject_bytes(code.get_bytes(), label);
}

uint32_t ROM::reserve_data_block(uint32_t byte_count, const std::string& label)
{
    std::pair<uint32_t, uint32_t>* best_pair = nullptr;
    size_t best_pair_size = UINT32_MAX;

    // Find the smallest pair that fits with the required byte count
    for(auto& pair : _empty_chunks)
    {
        if(pair.first >= pair.second)
            continue;

        size_t chunk_size = (pair.second - pair.first);
        if(chunk_size < byte_count)
            continue;

        if(chunk_size < best_pair_size)
        {
            best_pair = &pair;
            best_pair_size = chunk_size;
        }
    }

    // Update the pair to remove reserved space from possible future injection spots
    if(best_pair)
    {
        uint32_t injection_addr = best_pair->first;
        best_pair->first += byte_count;

        // Don't allow an empty chunk to begin with an odd address
        if(best_pair->first % 2 != 0)
            best_pair->first++;

        if (!label.empty())
            this->store_address(label, injection_addr);

        return injection_addr;
    }
    else throw std::out_of_range("Not enough empty room inside the ROM to inject data");
}

void ROM::mark_empty_chunk(uint32_t begin, uint32_t end)
{
    // Don't allow an empty chunk to begin with an odd address
    if(begin % 2 != 0)
        begin++;

    if(begin >= end)
        return;

    for(auto& pair : _empty_chunks)
    {
        if((begin >= pair.first && begin < pair.second) || (end > pair.first && end <= pair.second))
        {
            std::cerr << "There is an overlap between empty chunks" << std::endl;
            return;
        }
    }

    for(uint32_t addr=begin ; addr < end ; ++addr)
        this->set_byte(addr, 0xFF);

    _empty_chunks.emplace_back(std::make_pair(begin, end));
}

uint32_t ROM::remaining_empty_bytes() const
{
    uint32_t count = 0;
    for(auto& pair : _empty_chunks)
        count += (pair.second - pair.first);
    return count;
}

void ROM::extend(size_t new_size)
{
    size_t old_size = _byte_array.size();
    _byte_array.resize(new_size, 0);
    this->mark_empty_chunk(old_size, new_size);

    this->set_long(0x1A4, new_size-1);
}

void ROM::write_to_file(std::ofstream& output_file)
{
    this->update_checksum();
    output_file.write((char*)&(_byte_array[0]), (int32_t)_byte_array.size());
    output_file.close();
}

void ROM::update_checksum()
{
    uint16_t checksum = 0;
    for (uint32_t addr = 0x200; addr < _byte_array.size(); addr += 0x02)
    {
        uint8_t msb = _byte_array[addr];
        uint8_t lsb = _byte_array[addr + 1];
        uint16_t word = (uint16_t)(msb << 8) | lsb;
        checksum += word;
    }

    this->set_word(0x18E, checksum);
}

} // namespace md
