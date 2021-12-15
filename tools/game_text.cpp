#include "game_text.hpp"
#include <sstream>

GameText::GameText(const std::string& text, uint8_t lines_in_textbox) :
        _lines_in_textbox      (lines_in_textbox)
{
    this->text(text);
}

GameText::GameText(const std::string& text, const std::string& name, uint8_t lines_in_textbox) :
        GameText(text, lines_in_textbox)
{
    std::string full_name = name + ": ";
    for(char c : full_name)
        _current_line_length += chararcter_width(c);
}

void GameText::text(const std::string& text)
{
    _initial_text = text;
    _output_text = "";

    for (size_t i = 0; i < text.size(); ++i)
        this->add_character(text, i);

    _output_text += "\x03"; // EOL
}

void GameText::add_character(const std::string& text, size_t i)
{
    char character = text[i];

    // If we start a new word, check if we can finish it on the same line
    if (!_output_text.empty() && character != ' ' && (*_output_text.rbegin()) == ' ')
    {
        uint16_t wordWidth = 0;
        char currentWordChar = character;
        while (currentWordChar != '\n' && currentWordChar != ' ' && currentWordChar != '\0')
        {
            wordWidth += chararcter_width(currentWordChar);
            currentWordChar = text[++i];
        }

        // Word is too big to fit on the line, skip a line
        if (_current_line_length + wordWidth >= 0x105)
            this->add_character('\n');
    }

    this->add_character(character);
}

void GameText::add_character(char character)
{
    if (character == '\n')
    {
        _current_line_length = 0;
        if (*_output_text.rbegin() == ' ')
            _output_text = _output_text.substr(0, _output_text.size() - 1);

        if (++_current_line_count == _lines_in_textbox)
        {
            _output_text += "\x1E";
            _current_line_count = 0;
        }
        _output_text += "\x0A";

        return;
    } 
    
    if (_current_line_length >= 0x105)
    {
        this->add_character('\n');

        uint8_t previousByte = *_output_text.rbegin();
        bool needsHyphen = (previousByte >= 'A' && previousByte <= 'z');
        if (needsHyphen)
            this->add_character('-');
    }

    _output_text += character;
    _current_line_length += chararcter_width(character);
}

uint8_t GameText::chararcter_width(char character)
{
    switch (character)
    {
    case ' ':    return 8;
    case '0':    return 8;
        //        case '1':    return 8;
    case '2':    return 8;
        //        case '3':    return 8;
        //        case '4':    return 8;
        //        case '5':    return 8;
        //        case '6':    return 8;
        //        case '7':    return 8;
        //        case '8':    return 8;
        //        case '9':    return 8;
    case 'A':    return 8;
    case 'B':    return 8;
        //        case 'C':    return 8;
    case 'D':    return 8;
    case 'E':    return 8;
    case 'F':    return 8;
        //        case 'G':    return 8;
        //        case 'H':    return 8;
    case 'I':    return 5;
        //        case 'J':    return 8;
        //        case 'K':    return 8;
        //        case 'L':    return 8;
        //        case 'M':    return 8;
        //        case 'N':    return 8;
    case 'O':    return 8;
        //        case 'P':    return 8;
        //        case 'Q':    return 8;
        //        case 'R':    return 8;
        //        case 'S':    return 8;
    case 'T':    return 9;
        //        case 'U':    return 8;
        //        case 'V':    return 8;
        //        case 'W':    return 8;
        //        case 'X':    return 8;
    case 'Y':    return 8;
        //        case 'Z':    return 8;
    case 'a':    return 9;
    case 'b':    return 8;
    case 'c':    return 8;
    case 'd':    return 8;
    case 'e':    return 8;
    case 'f':    return 7;
    case 'g':    return 8;
    case 'h':    return 8;
    case 'i':    return 5;
    case 'j':    return 6;
    case 'k':    return 8;
    case 'l':    return 6;
    case 'm':    return 10;
    case 'n':    return 8;
    case 'o':    return 8;
    case 'p':    return 8;
        //        case 'q':    return 8;
    case 'r':    return 8;
    case 's':    return 8;
    case 't':    return 8;
    case 'u':    return 8;
    case 'v':    return 9;
    case 'w':    return 11;
    case 'x':    return 8;
    case 'y':    return 8;
        //        case 'z':    return 8;
    case '*':    return 9;
    case '.':    return 5;
    case ',':    return 5;
    case '?':    return 8;
    case '!':    return 5;
        //        case '/':    return 8;
        //        case '<':    return 8;
        //        case '>':    return 8;
    case ':':    return 4;
        //        case '-':    return 8;
    case '\'':    return 4;
        //        case '"':    return 8;
        //        case '%':    return 8;
        //        case '#':    return 8;
        //        case '&':    return 8;
        //        case '(':    return 8;
        //        case ')':    return 8;
        //        case '=':    return 8;
    default:    return 8;
    }
}
