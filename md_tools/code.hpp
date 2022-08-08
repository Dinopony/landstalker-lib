#pragma once

#include <vector>
#include <map>
#include <string>
#include "types.hpp"

namespace md
{
    class Code {
    private:
        std::vector<uint8_t> _bytes;
        std::map<uint32_t, std::string> _pending_branches;  // key = address of branch instruction, value = label name
        std::map<std::string, uint32_t> _labels;

    public:
        Code() = default;

        void add_byte(uint8_t byte);
        void add_word(uint16_t word);
        void add_long(uint32_t longword);
        void add_bytes(const std::vector<uint8_t>& bytes);
        void add_opcode(uint16_t opcode);

        Code& bsr(uint16_t offset);
        Code& jsr(uint32_t address);
        Code& jmp(const Param& target);
        Code& jmp(uint32_t address);

        Code& cmp(const Param& value, const DataRegister& dx, Size size);
        Code& cmpb(const Param& value, const DataRegister& dx) { return this->cmp(value, dx, Size::BYTE); }
        Code& cmpw(const Param& value, const DataRegister& dx) { return this->cmp(value, dx, Size::WORD); }
        Code& cmpl(const Param& value, const DataRegister& dx) { return this->cmp(value, dx, Size::LONG); }

        Code& cmpi(const ImmediateValue& value, const Param& other, Size size);
        Code& cmpib(uint8_t value, const Param& other) { return this->cmpi(ImmediateValue(value), other, Size::BYTE); }
        Code& cmpiw(uint16_t value, const Param& other) { return this->cmpi(ImmediateValue(value), other, Size::WORD); }
        Code& cmpil(uint32_t value, const Param& other) { return this->cmpi(ImmediateValue(value), other, Size::LONG); }
        
        Code& cmpa(const Param& value, const AddressRegister& reg);

        Code& tst(const Param& target, Size size);
        Code& tstb(const Param& target) { return this->tst(target, Size::BYTE); }
        Code& tstw(const Param& target) { return this->tst(target, Size::WORD); }
        Code& tstl(const Param& target) { return this->tst(target, Size::LONG); }

        Code& bra(const std::string& label);
        Code& beq(const std::string& label);
        Code& bne(const std::string& label);
        Code& blt(const std::string& label);
        Code& bgt(const std::string& label);
        Code& bmi(const std::string& label);
        Code& bpl(const std::string& label);
        Code& ble(const std::string& label);
        Code& bge(const std::string& label);
        Code& bcc(const std::string& label);
        Code& dbra(const DataRegister& dx, const std::string& label);

        Code& clr(const Param& param, Size size);
        Code& clrb(const Param& param) { return this->clr(param, Size::BYTE); }
        Code& clrw(const Param& param) { return this->clr(param, Size::WORD); }
        Code& clrl(const Param& param) { return this->clr(param, Size::LONG); }

        Code& move(const Param& from, const Param& to, Size size);
        Code& moveb(uint8_t from, const Param& to) { return this->move(ImmediateValue(from), to, Size::BYTE); }
        Code& movew(uint16_t from, const Param& to) { return this->move(ImmediateValue(from), to, Size::WORD); }
        Code& movel(uint32_t from, const Param& to) { return this->move(ImmediateValue(from), to, Size::LONG); }
        Code& moveb(const Param& from, const Param& to) { return this->move(from, to, Size::BYTE); }
        Code& movew(const Param& from, const Param& to) { return this->move(from, to, Size::WORD); }
        Code& movel(const Param& from, const Param& to) { return this->move(from, to, Size::LONG); }

        Code& moveq(uint8_t value, const DataRegister& dx);

        Code& addq(uint8_t value, const Register& Rx, Size size);
        Code& addqb(uint8_t value, const Register& Rx) { return this->addq(value, Rx, Size::BYTE); }
        Code& addqw(uint8_t value, const Register& Rx) { return this->addq(value, Rx, Size::WORD); }
        Code& addql(uint8_t value, const Register& Rx) { return this->addq(value, Rx, Size::LONG); }

        Code& movem_to_stack(const std::vector<DataRegister>& data_regs, const std::vector<AddressRegister>& addr_regs);
        Code& movem_from_stack(const std::vector<DataRegister>& data_regs, const std::vector<AddressRegister>& addr_regs);

        Code& bset(uint8_t bit_id, const Param& target);
        Code& bset(const DataRegister& dx, const Param& target);
        Code& bclr(uint8_t bit_id, const Param& target);
        Code& bclr(const DataRegister& dx, const Param& target);
        Code& btst(uint8_t bit_id, const Param& target);
        Code& btst(const DataRegister& dx, const Param& target);

        Code& addi(const ImmediateValue& value, const Param& target, Size size);
        Code& addib(uint8_t value, const Param& target) { return this->addi(ImmediateValue(value), target, Size::BYTE); }
        Code& addiw(uint16_t value, const Param& target) { return this->addi(ImmediateValue(value), target, Size::WORD); }
        Code& addil(uint32_t value, const Param& target) { return this->addi(ImmediateValue(value), target, Size::LONG); }

        Code& subi(const ImmediateValue& value, const Param& target, Size size);
        Code& subib(uint8_t value, const Param& target) { return this->subi(ImmediateValue(value), target, Size::BYTE); }
        Code& subiw(uint16_t value, const Param& target) { return this->subi(ImmediateValue(value), target, Size::WORD); }
        Code& subil(uint32_t value, const Param& target) { return this->subi(ImmediateValue(value), target, Size::LONG); }

        Code& mulu(const Param& value, const DataRegister& dx);
        Code& divu(const Param& value, const DataRegister& dx);

        Code& adda(const Param& value, const AddressRegister& ax);
        Code& adda(uint32_t value, const AddressRegister& ax) { return this->adda(ImmediateValue(value), ax); }

        Code& lea(uint32_t value, const Register& ax);

        Code& and_to_dx(const Param& from, const DataRegister& to, Size size);
        Code& andb(const Param& from, const DataRegister& to) { return this->and_to_dx(from, to, Size::BYTE); }
        Code& andw(const Param& from, const DataRegister& to) { return this->and_to_dx(from, to, Size::WORD); }
        Code& andl(const Param& from, const DataRegister& to) { return this->and_to_dx(from, to, Size::LONG); }

        Code& andi(const ImmediateValue& value, const Param& target, Size size);
        Code& andib(uint8_t value, const Param& target) { return this->andi(ImmediateValue(value), target, Size::BYTE); }
        Code& andiw(uint16_t value, const Param& target) { return this->andi(ImmediateValue(value), target, Size::WORD); }
        Code& andil(uint32_t value, const Param& target) { return this->andi(ImmediateValue(value), target, Size::LONG); }

        Code& ori(const ImmediateValue& value, const Param& target, Size size);
        Code& orib(uint8_t value, const Param& target) { return this->ori(ImmediateValue(value), target, Size::BYTE); }
        Code& oriw(uint16_t value, const Param& target) { return this->ori(ImmediateValue(value), target, Size::WORD); }
        Code& oril(uint32_t value, const Param& target) { return this->ori(ImmediateValue(value), target, Size::LONG); }

        Code& ori_to_ccr(uint8_t value);

        Code& extw(const DataRegister& reg);
        Code& extl(const DataRegister& reg);

        Code& rts();
        Code& nop(uint16_t amount = 1);
        Code& trap(uint8_t trap_id, const std::vector<uint8_t>& additionnal_bytes = {});

        void label(const std::string& label);
        [[nodiscard]] uint32_t size() const { return (uint32_t)_bytes.size(); }
        [[nodiscard]] const std::vector<uint8_t>& get_bytes() const;

    private:
        void resolve_branches();
    };
}
