#pragma once

#include <string>
#include <vector>
#include <cstdint>

class GameText
{
private:
    std::string _initial_text;
    std::string _output_text;
    uint16_t _current_line_length = 0;
    uint8_t _current_line_count = 0;
    uint8_t _lines_in_textbox = 3;

public:
    GameText() = default;
    explicit GameText(const std::string& text, uint8_t lines_in_textbox = 3);
    GameText(const std::string& text, const std::string& name, uint8_t lines_in_textbox = 3);

    [[nodiscard]] const std::string& text() const { return _initial_text; }
    void text(const std::string& text);

    void add_character(const std::string& text, size_t i);
    void add_character(char character);

    [[nodiscard]] const std::string& getOutput() const { return _output_text; }

    static uint8_t character_width(char character);
};