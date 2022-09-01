#pragma once

#include "../game_patch.hpp"

/**
 * Add a soft reset function when A, B and C are pressed along Start to open game menu
 */
class PatchExtendROM : public GamePatch
{
public:
    void clear_space_in_rom(md::ROM& rom) override
    {
        // Injecting code in here may seem inappropriate, but it is an exception for this patch.
        // This code absolutely needs to be injected in the ROM space before extension, since its nature is to be
        // called while bank-switching.
        add_bankswitching_when_checking_sram(rom);
        add_bankswitching_when_erasing_save(rom);
        add_bankswitching_when_writing_save(rom);
        add_bankswitching_when_writing_save_checksum(rom);
        add_bankswitching_when_checking_save_checksum(rom);
        add_bankswitching_when_loading_save(rom);
        add_bankswitching_when_copying_save(rom);

        // Once the functions are injected, we free a HUUUUGE amount of space by doubling the ROM's effective size
        rom.extend(0x400000);
    }

private:
    static constexpr uint8_t BANK_SRAM = 0x1;
    static constexpr uint8_t BANK_ROM = 0x0;

    static void add_bankswitching_when_checking_sram(md::ROM& rom)
    {
        md::Code func;
        func.moveb(BANK_SRAM, addr_(0xA130F1)); // switch to SRAM
        {
            func.label("loop");
            {
                func.moveb(addr_postinc_(reg_A1), reg_D0);
                func.cmpb(addr_(reg_A0), reg_D0);
                func.beq("valid_magic_word");
                {
                    func.jsr(0x1520); // SetSRAMMagicWord
                    func.bra("ret");
                }
                func.label("valid_magic_word");
                func.addqw(2, reg_A0);
            }
            func.dbra(reg_D7, "loop");
        }
        func.label("ret");
        func.moveb(BANK_ROM, addr_(0xA130F1)); // switch to ROM
        func.rts();

        uint32_t func_check_magic_word = rom.inject_code(func);

        rom.set_code(0x14FE, md::Code().jsr(func_check_magic_word).nop(3));
    }

    static void add_bankswitching_when_erasing_save(md::ROM& rom)
    {
        md::Code func;
        func.moveb(BANK_SRAM, addr_(0xA130F1)); // switch to SRAM
        {
            func.label("loop");
            {
                func.clrb(addr_(reg_A0));
                func.addqw(2, reg_A0);
            }
            func.dbra(reg_D7, "loop");
        }
        func.moveb(BANK_ROM, addr_(0xA130F1)); // switch to ROM
        func.rts();

        uint32_t func_erase_save = rom.inject_code(func);

        rom.set_code(0x1558, md::Code().jsr(func_erase_save).nop());
    }

    static void add_bankswitching_when_writing_save(md::ROM& rom)
    {
        md::Code func;
        func.moveb(BANK_SRAM, addr_(0xA130F1)); // switch to SRAM
        {
            func.label("loop");
            {
                func.moveb(addr_postinc_(reg_A2), addr_(reg_A0));
                func.addqw(0x2, reg_A0);
            }
            func.dbra(reg_D7, "loop");
        }
        func.moveb(BANK_ROM, addr_(0xA130F1)); // switch to ROM
        func.rts();

        uint32_t func_write_sram = rom.inject_code(func);

        rom.set_code(0x15AE, md::Code().jsr(func_write_sram).nop());
    }

    static void add_bankswitching_when_writing_save_checksum(md::ROM& rom)
    {
        md::Code proc;
        proc.moveb(BANK_SRAM, addr_(0xA130F1)); // switch to SRAM
        proc.moveb(reg_D1, addr_(reg_A0));
        proc.moveb(BANK_ROM, addr_(0xA130F1)); // switch to ROM
        proc.movem_from_stack({ reg_D0, reg_D1, reg_D7 }, { reg_A0, reg_A1, reg_A2 });
        proc.rts();

        uint32_t proc_write_checksum = rom.inject_code(proc);

        rom.set_code(0x15BA, md::Code().jmp(proc_write_checksum));
    }

    static void add_bankswitching_when_checking_save_checksum(md::ROM& rom)
    {
        md::Code func;
        func.moveb(BANK_SRAM, addr_(0xA130F1)); // switch to SRAM
        {
            func.label("loop");
            {
                func.addb(addr_(reg_A0), reg_D1);
                func.addqw(2, reg_A0);
            }
            func.dbra(reg_D7, "loop");
            func.cmpb(addr_(reg_A0), reg_D1);
        }
        func.moveb(BANK_ROM, addr_(0xA130F1)); // switch to ROM
        func.rts();

        uint32_t func_check_save_checkum = rom.inject_code(func);

        rom.set_code(0x1574, md::Code().jsr(func_check_save_checkum).nop(2));
    }

    static void add_bankswitching_when_loading_save(md::ROM& rom)
    {
        md::Code func;
        func.moveb(BANK_SRAM, addr_(0xA130F1)); // switch to SRAM
        {
            func.label("loop");
            {
                func.moveb(addr_(reg_A0), addr_postinc_(reg_A2));
                func.addqw(2, reg_A0);
            }
            func.dbra(reg_D7, "loop");
        }
        func.moveb(BANK_ROM, addr_(0xA130F1)); // switch to ROM
        func.rts();

        uint32_t func_load_save = rom.inject_code(func);

        rom.set_code(0x15DA, md::Code().jsr(func_load_save).nop());
    }

    static void add_bankswitching_when_copying_save(md::ROM& rom)
    {
        md::Code func;
        func.moveb(BANK_SRAM, addr_(0xA130F1)); // switch to SRAM
        {
            func.label("loop");
            {
                func.moveb(addr_(reg_A1), addr_(reg_A0));
                func.addqw(2, reg_A0);
                func.addqw(2, reg_A1);
            }
            func.dbra(reg_D7, "loop");
        }
        func.moveb(BANK_ROM, addr_(0xA130F1)); // switch to ROM
        func.rts();

        uint32_t func_copy_save = rom.inject_code(func);

        rom.set_code(0x15F8, md::Code().jsr(func_copy_save).nop(2));
    }
};
