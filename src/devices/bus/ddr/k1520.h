// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Robotron K1520 bus

    Signal  Pin Pin Signalname
    5P      C29 A29 5P
    12P     C28 A28 12P
    /BAI    C27 A27 /BAO
    /HALT   C26 A26 /M1
    /RDY    C25 A25 /RFSH
    /IORQ   C24 A24 /WAIT
    /INT    C23 A23 /NMI
    00      C22 A22 /IODI
    00      C21 A21 TAKT
    /BUSRQ  C20 A20 /RESET
    AB1     C19 A19 AB0
    AB3     C18 A18 AB2
    AB5     C17 A17 AB4
    AB7     C16 A16 AB6
    5N      C15 A15 5N
    AB9     C14 A14 AB8
    AB11    C13 A13 AB10
    AB13    C12 A12 AB12
    AB15    C11 A11 AB14
    /IEI    C10 A10 /IEO
    /MEMDI  C9   A9 /MREQ
    /RD     C8   A8 /WR
    DB0     C7   A7 DB1
    DB2     C6   A6 DB3
    DB4     C5   A5 DB5
    DB6     C4   A4 DB7
    5PG     C3   A3 5PG
    00      C2   A2 00
    00      C1   A1 00

***************************************************************************/

#ifndef MAME_BUS_DDR_K1520_H
#define MAME_BUS_DDR_K1520_H

#pragma once

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/keyboard.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "drawgfx.h"
#include "emupal.h"
#include "screen.h"

#include <array>


class device_k1520_card_interface;

class k1520_bus_device : public device_t
{
public:
	k1520_bus_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock = 0);

	auto irq() { return m_out_irq_cb.bind(); }
	auto nmi() { return m_out_nmi_cb.bind(); }

	void add_card(unsigned slot, device_k1520_card_interface &card);

	u8 memory_r(offs_t offset);
	void memory_w(offs_t offset, u8 data);
	u8 io_r(offs_t offset);
	void io_w(offs_t offset, u8 data);

	void irq_w(int state);
	void nmi_w(int state);

protected:
	virtual void device_start() override ATTR_COLD;

private:
	std::array<device_k1520_card_interface *, 12> m_cards;

	devcb_write_line m_out_irq_cb;
	devcb_write_line m_out_nmi_cb;
};

DECLARE_DEVICE_TYPE(K1520_BUS, k1520_bus_device)


class device_k1520_card_interface : public device_interface
{
public:
	virtual ~device_k1520_card_interface();

	void set_bus(k1520_bus_device &bus, unsigned slot);
	void set_slot(k1520_bus_device &bus, unsigned slot) { set_bus(bus, slot); }

	virtual bool memory_r(offs_t offset, u8 &data) { return false; }
	virtual bool memory_w(offs_t offset, u8 data) { return false; }
	virtual bool io_r(offs_t offset, u8 &data) { return false; }
	virtual bool io_w(offs_t offset, u8 data) { return false; }

protected:
	device_k1520_card_interface(machine_config const &mconfig, device_t &device);

	k1520_bus_device *m_bus;
	unsigned m_slot;
};


class k1520_abs_k7024_device : public device_t, public device_k1520_card_interface
{
public:
	k1520_abs_k7024_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock, u16 base_addr = 0x1000);

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual bool memory_r(offs_t offset, u8 &data) override;
	virtual bool memory_w(offs_t offset, u8 data) override;

private:
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, rectangle const &cliprect);

	std::array<u8, 0x800> m_videoram;
	required_device<gfxdecode_device> m_gfxdecode;
	u16 m_base_addr;
	u8 m_framecnt;
};



DECLARE_DEVICE_TYPE(K1520_ABS, k1520_abs_k7024_device)

class k1520_ats_k7028_device : public device_t, public device_k1520_card_interface
{
public:
    k1520_ats_k7028_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock, u8 base_addr);

	INPUT_CHANGED_MEMBER(special_key);

protected:
    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
    virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
    virtual bool io_r(offs_t offset, u8 &data) override;
    virtual bool io_w(offs_t offset, u8 data) override;

private:
	void irq_w(int state);
	void keyboard_put(u8 data);

    required_device<z80sio_device> m_sio;
    required_device<z80ctc_device> m_ctc;
	required_device<generic_keyboard_device> m_keyboard;
	u8 m_base_addr;
	bool m_keyboard_status_pending;
	u8 m_keyboard_status;
	std::array<u8, 2> m_sio_loopback_data;
	std::array<bool, 2> m_sio_loopback_pending;
	std::array<std::array<u8, 16>, 2> m_printer_loopback_data;
	std::array<u8, 2> m_printer_loopback_head;
	std::array<u8, 2> m_printer_loopback_tail;
	std::array<u8, 2> m_printer_loopback_count;
};


DECLARE_DEVICE_TYPE(K1520_ATS, k1520_ats_k7028_device)

class k1520_pfs_7040_device : public device_t, public device_k1520_card_interface
{
public:
	k1520_pfs_7040_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock, u16 base_addr = 0x2000);

	void set_slot(k1520_bus_device &bus, unsigned slot);

protected:
	virtual void device_start() override ATTR_COLD;
	virtual bool memory_r(offs_t offset, u8 &data) override;
	virtual bool memory_w(offs_t offset, u8 data) override;

private:
	required_region_ptr<u8> m_rom;
	u16 m_base_addr;
};


DECLARE_DEVICE_TYPE(K1520_PFS, k1520_pfs_7040_device)

class k1520_placeholder_card_device : public device_t, public device_k1520_card_interface
{
public:
	k1520_placeholder_card_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

protected:
	virtual void device_start() override ATTR_COLD;
};

DECLARE_DEVICE_TYPE(K1520_PLACEHOLDER_CARD, k1520_placeholder_card_device)


#endif // MAME_BUS_DDR_K1520_H
