#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace md
{
enum class Size { BYTE, WORD, LONG };

class Param {
public:
    constexpr Param() = default;

    [[nodiscard]] virtual uint16_t getXn() const = 0;
    [[nodiscard]] virtual uint16_t getM() const = 0;

    [[nodiscard]] uint16_t getMXn() const { return (this->getM() << 3) + this->getXn(); }
    [[nodiscard]] uint16_t getXnM() const { return (this->getXn() << 3) + this->getM(); }

    [[nodiscard]] virtual std::vector<uint8_t> getAdditionnalData() const { return {}; }
};

////////////////////////////////////////////////////////////////////////////

class Register : public Param {
public:
    constexpr explicit Register(uint8_t code) :
            _code(code)
    {}

    [[nodiscard]] uint16_t getXn() const override { return _code & 0x7; }

    [[nodiscard]] virtual bool isDataRegister() const = 0;
    [[nodiscard]] bool isAddressRegister() const { return !isDataRegister(); }

protected:
    uint8_t _code;
};

class AddressRegister : public Register {
public:
    constexpr explicit AddressRegister(uint8_t code) : Register(code) {}

    [[nodiscard]] uint16_t getM() const override { return 0x1; }
    [[nodiscard]] bool isDataRegister() const override { return false; }
};

class DataRegister : public Register {
public:
    constexpr explicit DataRegister(uint8_t code) : Register(code) {}

    [[nodiscard]] uint16_t getM() const override { return 0x0; }
    [[nodiscard]] bool isDataRegister() const override { return true; }
};

////////////////////////////////////////////////////////////////////////////

class DirectAddress : public Param {
public:
    constexpr explicit DirectAddress(uint32_t address, md::Size size) :
            _address        (address),
            _size           (size)
    {}

    [[nodiscard]] uint16_t getM() const override { return 0x7; }
    [[nodiscard]] uint16_t getXn() const override { return (_size == md::Size::LONG) ? 0x1 : 0x0; }

    [[nodiscard]] uint32_t getAddress() const { return _address; }
    [[nodiscard]] md::Size getSize() const { return _size; }

    [[nodiscard]] std::vector<uint8_t> getAdditionnalData() const override
    {
        std::vector<uint8_t> addressBytes;
        if(_size == md::Size::LONG)
        {
            addressBytes.emplace_back((_address >> 24) & 0xFF);
            addressBytes.emplace_back((_address >> 16) & 0xFF);
        }
        addressBytes.emplace_back((_address >> 8) & 0xFF);
        addressBytes.emplace_back(_address & 0xFF);
        return addressBytes;
    }

private:
    uint32_t _address;
    md::Size _size;
};

////////////////////////////////////////////////////////////////////////////

class AddressInRegister : public Param {
public:
    constexpr explicit AddressInRegister(AddressRegister reg, uint16_t offset = 0) :
            _offset(offset),
            _reg(std::move(reg))
    {}

    [[nodiscard]] uint16_t getM() const override { return (_offset > 0) ? 0x5 : _m_without_offset; }
    [[nodiscard]] uint16_t getXn() const override { return _reg.getXn(); }

    [[nodiscard]] std::vector<uint8_t> getAdditionnalData() const override
    {
        if (_offset > 0)
        {
            std::vector<uint8_t> offset_bytes;
            offset_bytes.emplace_back((_offset >> 8) & 0xFF);
            offset_bytes.emplace_back(_offset & 0xFF);
            return offset_bytes;
        }
        else return {};
    }

    void post_increment() { _m_without_offset = 0x3; }
    void pre_decrement() { _m_without_offset = 0x4; }

private:
    AddressRegister _reg;
    uint16_t _offset;
    uint8_t _m_without_offset = 0x2;
};

////////////////////////////////////////////////////////////////////////////

class AddressWithIndex : public Param {
public:
    constexpr AddressWithIndex(const AddressRegister& baseAddrReg, const Register& offsetReg, Size offsetRegSize, uint8_t additionnalOffset = 0) :
            _baseAddrReg(baseAddrReg),
            _offsetReg(offsetReg),
            _offsetRegSize(offsetRegSize),
            _additionnalOffset(additionnalOffset)
    {}

    [[nodiscard]] uint16_t getM() const override { return 0x6; }
    [[nodiscard]] uint16_t getXn() const override { return _baseAddrReg.getXn(); }

    [[nodiscard]] std::vector<uint8_t> getAdditionnalData() const override
    {
        uint8_t msb = (_offsetReg.getMXn() & 0x0F) << 4;
        if (_offsetRegSize == Size::LONG)
            msb |= 0x8;

        uint8_t lsb = _additionnalOffset;

        std::vector<uint8_t> briefExtensionWord;
        briefExtensionWord.emplace_back(msb);
        briefExtensionWord.emplace_back(lsb);
        return briefExtensionWord;
    }

private:
    const AddressRegister& _baseAddrReg;
    const Register& _offsetReg;
    Size _offsetRegSize;
    uint8_t _additionnalOffset;
};

////////////////////////////////////////////////////////////////////////////

class ImmediateValue : public Param {
public:
    explicit constexpr ImmediateValue(uint8_t value) : _size(Size::BYTE), _value(value) {}
    explicit constexpr ImmediateValue(uint16_t value) : _size(Size::WORD), _value(value) {}
    explicit constexpr ImmediateValue(uint32_t value) : _size(Size::LONG), _value(value) {}

    [[nodiscard]] uint16_t getXn() const override { return 0x4; }
    [[nodiscard]] uint16_t getM() const override { return 0x7; }

    [[nodiscard]] std::vector<uint8_t> getAdditionnalData() const override
    {
        std::vector<uint8_t> valueBytes;
        if (_size == Size::LONG)
        {
            valueBytes.emplace_back((_value >> 24) & 0xFF);
            valueBytes.emplace_back((_value >> 16) & 0xFF);
        }

        valueBytes.emplace_back((_value >> 8) & 0xFF);
        valueBytes.emplace_back(_value & 0xFF);

        return valueBytes;
    }

private:
    uint32_t _value;
    Size _size;
};
}

#define reg_A0 md::AddressRegister(0)
#define reg_A1 md::AddressRegister(1)
#define reg_A2 md::AddressRegister(2)
#define reg_A3 md::AddressRegister(3)
#define reg_A4 md::AddressRegister(4)
#define reg_A5 md::AddressRegister(5)
#define reg_A6 md::AddressRegister(6)
#define reg_A7 md::AddressRegister(7)

#define reg_D0 md::DataRegister(0)
#define reg_D1 md::DataRegister(1)
#define reg_D2 md::DataRegister(2)
#define reg_D3 md::DataRegister(3)
#define reg_D4 md::DataRegister(4)
#define reg_D5 md::DataRegister(5)
#define reg_D6 md::DataRegister(6)
#define reg_D7 md::DataRegister(7)

#define reg_D0_D7 reg_D0,reg_D1,reg_D2,reg_D3,reg_D4,reg_D5,reg_D6,reg_D7
#define reg_A0_A6 reg_A0,reg_A1,reg_A2,reg_A3,reg_A4,reg_A5,reg_A6

template<typename T>
inline constexpr md::ImmediateValue bval_(T value)
{
    return md::ImmediateValue(static_cast<uint8_t>(value));
}

template<typename T>
inline constexpr md::ImmediateValue wval_(T value)
{
    return md::ImmediateValue(static_cast<uint16_t>(value));
}

template<typename T>
inline constexpr md::ImmediateValue lval_(T value)
{
    return md::ImmediateValue(static_cast<uint32_t>(value));
}

template<typename T = void>
inline constexpr md::DirectAddress addr_(uint32_t address, md::Size size = md::Size::LONG)
{
    return md::DirectAddress(address, size);
}

template<typename T = void>
inline constexpr md::DirectAddress addrw_(uint32_t address)
{
    return addr_(address, md::Size::WORD);
}

template<typename T = void>
inline constexpr md::AddressInRegister addr_(const md::AddressRegister& addressRegister)
{
    return md::AddressInRegister(addressRegister);
}

template<typename T = void>
inline constexpr md::AddressInRegister addr_postinc_(const md::AddressRegister& addressRegister)
{
    md::AddressInRegister ret(addressRegister);
    ret.post_increment();
    return ret;
}

template<typename T = void>
inline constexpr md::AddressInRegister addr_predec_(const md::AddressRegister& addressRegister)
{
    md::AddressInRegister ret(addressRegister);
    ret.pre_decrement();
    return ret;
}

template<typename T>
inline constexpr md::AddressInRegister addr_(const md::AddressRegister& addressRegister, T offset)
{
    return md::AddressInRegister(addressRegister, offset);
}

template<typename T = void>
inline constexpr md::AddressWithIndex addr_(  const md::AddressRegister& addressRegister,
                                              const md::DataRegister& offsetDataRegister,
                                              uint8_t additionnalOffset = 0,
                                              md::Size dataRegisterSize = md::Size::LONG)
{
    return md::AddressWithIndex(addressRegister, offsetDataRegister, dataRegisterSize, additionnalOffset);
}

template<typename T = void>
inline constexpr md::AddressWithIndex addrw_(  const md::AddressRegister& addressRegister,
                                               const md::DataRegister& offsetDataRegister,
                                               uint8_t additionnalOffset = 0)
{
    return addr_(addressRegister, offsetDataRegister, additionnalOffset, md::Size::WORD);
}
