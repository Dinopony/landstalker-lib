#include "code.hpp"
#include "../exceptions.hpp"

namespace md {

void Code::add_byte(uint8_t byte)
{
    _bytes.emplace_back(byte);
}

void Code::add_word(uint16_t word)
{
    this->add_byte(static_cast<uint8_t>(word >> 8));
    this->add_byte(static_cast<uint8_t>(word & 0xFF));
}

void Code::add_long(uint32_t longword)
{
    this->add_word(static_cast<uint16_t>(longword >> 16));
    this->add_word(static_cast<uint16_t>(longword & 0xFFFF));
}

void Code::add_bytes(const std::vector<uint8_t>& bytes)
{
    for (uint8_t byte : bytes)
        this->add_byte(byte);
}

void Code::add_opcode(uint16_t opcode)
{
    this->add_word(opcode);
}

Code& Code::bsr(uint16_t offset)
{
    if(offset > 0x00FF)
    {
        this->add_opcode(0x6100);
        this->add_word(offset);
    }
    else
        this->add_opcode(0x6100 + offset);
    return *this;
}

Code& Code::jsr(uint32_t address)
{
    this->add_opcode(0x4EB9);
    this->add_long(address);
    return *this;
}

Code& Code::jmp(const Param& target)
{
    uint16_t opcode = 0x4EC0 + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(target.getAdditionnalData());
    return *this;
}

Code& Code::jmp(uint32_t address)
{
    this->add_opcode(0x4EF9);
    this->add_long(address);
    return *this;
}

Code& Code::cmp(const Param& value, const DataRegister& dx, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0xB000 + (dx.getXn() << 9) + (size_code << 6) + value.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());

    return *this;
}

Code& Code::cmpi(const ImmediateValue& value, const Param& other, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0x0C00 + (size_code << 6) + other.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());
    this->add_bytes(other.getAdditionnalData());

    return *this;
}

Code& Code::cmpa(const Param& value, const AddressRegister& reg)
{
    uint16_t opcode = 0xB1C0 + (reg.getXn() << 9) + value.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());
    return *this;
}

Code& Code::tst(const Param& target, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0x4A00 + (size_code << 6) + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(target.getAdditionnalData());

    return *this;
}

Code& Code::bra(const std::string& label)
{
    this->add_opcode(0x6000);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::beq(const std::string& label)
{
    this->add_opcode(0x6700);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::bne(const std::string& label)
{
    this->add_opcode(0x6600);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::blt(const std::string& label)
{
    this->add_opcode(0x6D00);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::bgt(const std::string& label)
{
    this->add_opcode(0x6E00);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::bmi(const std::string& label)
{
    this->add_opcode(0x6B00);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::bpl(const std::string& label)
{
    this->add_opcode(0x6A00);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::ble(const std::string& label)
{
    this->add_opcode(0x6F00);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::bge(const std::string& label)
{
    this->add_opcode(0x6C00);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::bcc(const std::string& label)
{
    this->add_opcode(0x6400);
    _pending_branches[static_cast<uint32_t>(_bytes.size())] = label;
    this->resolve_branches();
    return *this;
}

Code& Code::dbra(const DataRegister& dx, const std::string& label)
{
    this->add_opcode(0x51C8 + dx.getXn());

    uint32_t thisAddress = static_cast<uint32_t>(_bytes.size());
    uint32_t labelAddress = _labels.at(label);
    uint16_t offset = static_cast<uint16_t>(labelAddress - thisAddress);
    this->add_word(offset);

    return *this;
}

Code& Code::clr(const Param& param, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0x4200 + (size_code << 6) + param.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(param.getAdditionnalData());
    return *this;
}

Code& Code::move(const Param& from, const Param& to, Size size)
{
    uint16_t moveSizeCode = 0x1;
    if (size == Size::WORD)
        moveSizeCode = 0x3;
    else if (size == Size::LONG)
        moveSizeCode = 0x2;

    uint16_t opcode = (moveSizeCode << 12) + (to.getXnM() << 6) + from.getMXn();
    this->add_opcode(opcode);

    this->add_bytes(from.getAdditionnalData());
    this->add_bytes(to.getAdditionnalData());

    return *this;
}

Code& Code::moveq(uint8_t value, const DataRegister& dx)
{
    uint16_t opcode = 0x7000 + (dx.getXn() << 9) + value;
    this->add_opcode(opcode);
    return *this;
}

Code& Code::addq(uint8_t value, const Register& Rx, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0x5000 + ((uint16_t)(value & 0x7) << 9) + (size_code << 6) + Rx.getMXn();
    this->add_opcode(opcode);
    return *this;
}

Code& Code::movem_to_stack(const std::vector<DataRegister>& data_regs, const std::vector<AddressRegister>& addr_regs)
{
    uint16_t registersToStore = 0x0000;
    for (const DataRegister& r : data_regs)
        registersToStore |= (0x8000 >> r.getXn());
    for (const AddressRegister& r : addr_regs)
        registersToStore |= (0x0080 >> r.getXn());

    this->add_opcode(0x48E7);
    this->add_word(registersToStore); // 0xFFFE = D0-D7 and A0-A6
    return *this;
}

Code& Code::movem_from_stack(const std::vector<DataRegister>& data_regs, const std::vector<AddressRegister>& addr_regs)
{
    uint16_t registersToStore = 0x0000;
    for (const DataRegister& r : data_regs)
        registersToStore |= (0x0001 << r.getXn());
    for (const AddressRegister& r : addr_regs)
        registersToStore |= (0x0100 << r.getXn());

    this->add_opcode(0x4CDF);
    this->add_word(registersToStore); // 0x7FFF = A6-A0 and D7-D0
    return *this;
}

Code& Code::bset(uint8_t bit_id, const Param& target)
{
    this->add_opcode(0x08C0 + target.getMXn());
    this->add_word(static_cast<uint16_t>(bit_id));
    this->add_bytes(target.getAdditionnalData());
    return *this;
}

Code& Code::bset(const DataRegister& dx, const Param& target)
{
    uint16_t opcode = 0x01C0 + (dx.getXn() << 9) + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(target.getAdditionnalData());
    return *this;
}

Code& Code::bclr(uint8_t bit_id, const Param& target)
{
    this->add_opcode(0x0880 + target.getMXn());
    this->add_word(static_cast<uint16_t>(bit_id));
    this->add_bytes(target.getAdditionnalData());
    return *this;
}

Code& Code::bclr(const DataRegister& dx, const Param& target)
{
    uint16_t opcode = 0x0180 + (dx.getXn() << 9) + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(target.getAdditionnalData());
    return *this;
}


Code& Code::btst(uint8_t bit_id, const Param& target)
{
    this->add_opcode(0x0800 + target.getMXn());
    this->add_word(static_cast<uint16_t>(bit_id));
    this->add_bytes(target.getAdditionnalData());
    return *this;
}

Code& Code::btst(const DataRegister& dx, const Param& target)
{
    uint16_t opcode = 0x0100 + (dx.getXn() << 9) + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(target.getAdditionnalData());
    return *this;
}

Code& Code::addi(const ImmediateValue& value, const Param& target, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0x0600 + (size_code << 6) + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());
    this->add_bytes(target.getAdditionnalData());

    return *this;
}

Code& Code::subi(const ImmediateValue& value, const Param& target, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0x0400 + (size_code << 6) + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());
    this->add_bytes(target.getAdditionnalData());

    return *this;
}

Code& Code::mulu(const Param& value, const DataRegister& dx)
{
    uint16_t opcode = 0xC0C0 + (dx.getXn() << 9) + value.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());

    return *this;
}

Code& Code::divu(const Param& value, const DataRegister& dx)
{
    uint16_t opcode = 0x80C0 + (dx.getXn() << 9) + value.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());

    return *this;
}

Code& Code::adda(const Param& value, const AddressRegister& ax)
{
    uint16_t opcode = 0xD1C0 + (ax.getXn() << 9) + value.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());
    return *this;
}

Code& Code::lea(uint32_t value, const Register& ax)
{
    uint16_t opcode = 0x41F9 + (ax.getXn() << 9);
    this->add_opcode(opcode);
    this->add_long(value);
    return *this;
}

Code& Code::and_to_dx(const Param& from, const DataRegister& to, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0xC000 + (to.getXn() << 9) + (size_code << 6) + from.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(from.getAdditionnalData());

    return *this;
}

Code& Code::andi(const ImmediateValue& value, const Param& target, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0x0200 + (size_code << 6) + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());
    this->add_bytes(target.getAdditionnalData());

    return *this;
}

Code& Code::ori(const ImmediateValue& value, const Param& target, Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t opcode = 0x0000 + (size_code << 6) + target.getMXn();
    this->add_opcode(opcode);
    this->add_bytes(value.getAdditionnalData());
    this->add_bytes(target.getAdditionnalData());

    return *this;
}

Code& Code::ori_to_ccr(uint8_t value)
{
    this->add_opcode(0x003C);
    this->add_byte(0x00);
    this->add_byte(value);
    return *this;
}

Code& Code::lsx(const DataRegister& bitcount_reg, const DataRegister& reg, bool direction_left, md::Size size)
{
    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t direction_mask = (direction_left) ? 1 : 0;

    uint16_t opcode = 0xE028 + (bitcount_reg.getXn() << 9) + (direction_mask << 8) + (size_code << 6) + reg.getXn();
    this->add_opcode(opcode);
    return *this;
}

Code& Code::lsx(uint8_t bitcount, const DataRegister& reg, bool direction_left, md::Size size)
{
    if(bitcount > 8 || bitcount == 0)
        throw std::exception(); // Impossible

    if(bitcount == 8)
        bitcount = 0;

    uint16_t size_code = 0x0;
    if (size == Size::WORD)
        size_code = 0x1;
    else if (size == Size::LONG)
        size_code = 0x2;

    uint16_t direction_mask = (direction_left) ? 1 : 0;

    uint16_t opcode = 0xE008 + (bitcount << 9) + (direction_mask << 8) + (size_code << 6) + reg.getXn();
    this->add_opcode(opcode);
    return *this;
}

Code& Code::extw(const DataRegister& reg)
{
    this->add_opcode(0x4880 + reg.getXn());
    return *this;
}

Code& Code::extl(const DataRegister& reg)
{
    this->add_opcode(0x48C0 + reg.getXn());
    return *this;
}

Code& Code::rts()
{
    this->add_word(0x4E75);
    return *this;
}

Code& Code::nop(uint16_t amount)
{
    this->add_word(0x4E71);
    if (--amount > 0)
        this->nop(amount);
    return *this;
}

Code& Code::trap(uint8_t trap_id, const std::vector<uint8_t>& additionnal_bytes)
{
    uint16_t opcode = 0x4E40 + static_cast<uint16_t>(trap_id);
    this->add_opcode(opcode);
    this->add_bytes(additionnal_bytes);
    return *this;
}

void Code::label(const std::string& label)
{
    _labels[label] = static_cast<uint32_t>(_bytes.size());
    this->resolve_branches();
}

const std::vector<uint8_t>& Code::get_bytes() const
{
    if (!_pending_branches.empty())
        throw LandstalkerException(std::string("Pending branch is unresolved on injected code : ") + _pending_branches.begin()->second);
    return _bytes;
}

void Code::resolve_branches()
{
    std::vector<uint32_t> branches_to_clean;

    for (auto& [branch_address, label] : _pending_branches)
    {
        if (_labels.count(label))
        {
            uint32_t label_address = _labels[label];
            int32_t offset = (int32_t)(label_address - branch_address);
            if(offset > 0x7FFF || offset < -0x7FFF)
            {
                throw LandstalkerException("Offset for branch at byte " + std::to_string(branch_address) +
                                           " is too big (cannot be expressed as word)");
            }

            int16_t truncated_offset = (int16_t)(label_address - branch_address);
            uint16_t truncated_offset_as_word = static_cast<uint16_t>(truncated_offset);
            if (truncated_offset > 0x7F || truncated_offset < -0x7F)
            {
                // Branch offset is more than one byte long, we need to add an optional displacement
                _bytes.insert(_bytes.begin() + branch_address, static_cast<uint8_t>(truncated_offset_as_word >> 8));
                _bytes.insert(_bytes.begin() + branch_address + 1, static_cast<uint8_t>(truncated_offset_as_word & 0x00FF));
            }
            else
            {
                // Branch offset is less than one byte long, we only need to increment opcode by the offset value
                _bytes[static_cast<size_t>(branch_address) - 1] += truncated_offset_as_word;
            }

            branches_to_clean.emplace_back(branch_address);
        }
    }

    for (uint32_t addr : branches_to_clean)
        _pending_branches.erase(addr);
}

} // namespace md
