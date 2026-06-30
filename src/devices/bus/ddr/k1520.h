// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Robotron K1520 bus

***************************************************************************/

#ifndef MAME_BUS_DDR_K1520_H
#define MAME_BUS_DDR_K1520_H

#pragma once

#include "cpu/z80/z80.h"


class device_k1520_card_interface;

class k1520_bus_device : public device_t
{
public:
	k1520_bus_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock = 0);

	void add_card(unsigned slot, device_k1520_card_interface &card);

	u8 memory_r(offs_t offset);
	void memory_w(offs_t offset, u8 data);
	u8 io_r(offs_t offset);
	void io_w(offs_t offset, u8 data);

protected:
	virtual void device_start() override ATTR_COLD;

private:
	std::array<device_k1520_card_interface *, 12> m_cards;
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


class k1520_zre_device : public device_t, public device_k1520_card_interface
{
public:
	k1520_zre_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

protected:
	k1520_zre_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, u32 clock);

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual bool memory_r(offs_t offset, u8 &data) override;
	virtual bool memory_w(offs_t offset, u8 data) override;

private:
	void mem_map(address_map &map) ATTR_COLD;
	void io_map(address_map &map) ATTR_COLD;
	u8 bus_memory_r(offs_t offset);
	void bus_memory_w(offs_t offset, u8 data);
	u8 bus_io_r(offs_t offset);
	void bus_io_w(offs_t offset, u8 data);

	required_device<z80_device> m_maincpu;
	required_region_ptr<u8> m_rom;
	std::array<u8, 0x400> m_ram;
};


class k1520_zre_8786_device : public k1520_zre_device
{
public:
	k1520_zre_8786_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

protected:
	virtual bool memory_r(offs_t offset, u8 &data) override;

private:
	required_region_ptr<u8> m_rom;
};


class k1520_placeholder_card_device : public device_t, public device_k1520_card_interface
{
public:
	k1520_placeholder_card_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

protected:
	virtual void device_start() override ATTR_COLD;
};


DECLARE_DEVICE_TYPE(K1520_ZRE, k1520_zre_device)
DECLARE_DEVICE_TYPE(K1520_ZRE_8786, k1520_zre_8786_device)
DECLARE_DEVICE_TYPE(K1520_PLACEHOLDER_CARD, k1520_placeholder_card_device)


#endif // MAME_BUS_DDR_K1520_H
