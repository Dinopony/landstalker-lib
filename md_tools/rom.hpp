﻿#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>

#ifdef DEBUG
#include <set>
#endif

namespace md {

    class Code;

    class ROM
    {
    private:
        bool _was_open;
        std::vector<uint8_t> _byte_array;
        std::vector<uint8_t> _original_byte_array;
        std::map<std::string, uint32_t> _stored_addresses;
        std::vector<std::pair<uint32_t, uint32_t>> _empty_chunks;

    public:
        explicit ROM(const std::string& input_path);

        [[nodiscard]] bool is_valid() const { return _was_open; }

        [[nodiscard]] uint8_t get_byte(uint32_t address) const { return _byte_array[address]; }
        [[nodiscard]] uint16_t get_word(uint32_t address) const { return (this->get_byte(address) << 8) + this->get_byte(address+1); }
        [[nodiscard]] uint32_t get_long(uint32_t address) const { return (static_cast<uint32_t>(this->get_word(address)) << 16) + static_cast<uint32_t>(this->get_word(address+2)); }

        [[nodiscard]] std::vector<uint8_t> get_bytes(uint32_t begin, uint32_t end) const;
        [[nodiscard]] std::vector<uint16_t> get_words(uint32_t begin, uint32_t end) const;
        [[nodiscard]] std::vector<uint32_t> get_longs(uint32_t begin, uint32_t end) const;

        void set_byte(uint32_t address, uint8_t byte);
        void set_word(uint32_t address, uint16_t word);
        void set_long(uint32_t address, uint32_t long_word);
        void set_bytes(uint32_t address, std::vector<uint8_t> bytes);
        void set_bytes(uint32_t address, const unsigned char* bytes, size_t bytes_size);
        void set_code(uint32_t address, const Code& code);

        [[nodiscard]] uint32_t inject_bytes(const std::vector<uint8_t>& bytes, const std::string& label = "");
        [[nodiscard]] uint32_t inject_bytes(const unsigned char* bytes, size_t size_to_inject, const std::string& label = "");
        [[nodiscard]] uint32_t inject_code(const Code& code, const std::string& label = "");
        [[nodiscard]] uint32_t reserve_data_block(uint32_t byte_count, const std::string& label = "");

        [[nodiscard]] const uint8_t* iterator_at(uint32_t addr) const { return reinterpret_cast<const uint8_t*>(&(_byte_array[addr])); }

        void store_address(const std::string& name, uint32_t address) { _stored_addresses[name] = address; }
        uint32_t stored_address(const std::string& name) { return _stored_addresses.at(name); }

        void mark_empty_chunk(uint32_t begin, uint32_t end);
        [[nodiscard]] uint32_t remaining_empty_bytes() const;

        void extend(size_t new_size);

        void write_to_file(std::ofstream& output_file);
    private:
        void update_checksum();
    };

}