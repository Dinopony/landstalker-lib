#pragma once

#include <exception>
#include <string>

class LandstalkerException : public std::exception
{
public:
    LandstalkerException() : _message()
    {}

    LandstalkerException(const std::string& message) : _message(message)
    {}

    const char* what() const noexcept { return _message.c_str(); }

private:
    std::string _message;
};

class WrongVersionException : public LandstalkerException
{
public:
    WrongVersionException(const std::string& message) : LandstalkerException(message)
    {}
};

class JsonParsingException : public LandstalkerException
{
public:
    JsonParsingException(const std::string& message) : LandstalkerException(message)
    {}   
};
