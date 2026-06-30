// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Robotron K1520 bus

***************************************************************************/

#include "emu.h"
#include "k1520.h"


DEFINE_DEVICE_TYPE(K1520_BUS, k1520_bus_device, "k1520_bus", "Robotron K1520 Bus")
DEFINE_DEVICE_TYPE(K1520_ZRE, k1520_zre_device, "k1520_zre", "K1520 K2521 ZRE Board")
DEFINE_DEVICE_TYPE(K1520_ZRE_8786, k1520_zre_8786_device, "k1520_zre_8786", "K1520 045-8786 ZRE Board")
DEFINE_DEVICE_TYPE(K1520_PLACEHOLDER_CARD, k1520_placeholder_card_device, "k1520_placeholder", "K1520 Placeholder Board")


k1520_bus_device::k1520_bus_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K1520_BUS, tag, owner, clock),
	m_cards{ }
{
}

void k1520_bus_device::device_start()
{
}

void k1520_bus_device::add_card(unsigned slot, device_k1520_card_interface &card)
{
	if (slot == 0 || slot >= m_cards.size())
		fatalerror("K1520 slot %u out of range\n", slot);

	m_cards[slot] = &card;
}

u8 k1520_bus_device::memory_r(offs_t offset)
{
	u8 data = 0xff;

	for (device_k1520_card_interface *card : m_cards)
		if (card && card->memory_r(offset, data))
			return data;

	return data;
}

void k1520_bus_device::memory_w(offs_t offset, u8 data)
{
	for (device_k1520_card_interface *card : m_cards)
		if (card)
			card->memory_w(offset, data);
}

u8 k1520_bus_device::io_r(offs_t offset)
{
	u8 data = 0xff;

	for (device_k1520_card_interface *card : m_cards)
		if (card && card->io_r(offset, data))
			return data;

	return data;
}

void k1520_bus_device::io_w(offs_t offset, u8 data)
{
	for (device_k1520_card_interface *card : m_cards)
		if (card)
			card->io_w(offset, data);
}


device_k1520_card_interface::device_k1520_card_interface(machine_config const &mconfig, device_t &device) :
	device_interface(device, "k1520card"),
	m_bus(nullptr),
	m_slot(0)
{
}

device_k1520_card_interface::~device_k1520_card_interface()
{
}

void device_k1520_card_interface::set_bus(k1520_bus_device &bus, unsigned slot)
{
	m_bus = &bus;
	m_slot = slot;
	m_bus->add_card(slot, *this);
}


// K1520 K2521 (012-7100) / 045-8786 ZRE CPU board

k1520_zre_device::k1520_zre_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	k1520_zre_device(mconfig, K1520_ZRE, tag, owner, clock)
{
}

k1520_zre_device::k1520_zre_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_maincpu(*this, "maincpu"),
	m_rom(*this, "rom"),
	m_ram{ }
{
}

void k1520_zre_device::device_add_mconfig(machine_config &config)
{
	Z80(config, m_maincpu, XTAL(9'830'400) / 4);
	m_maincpu->set_addrmap(AS_PROGRAM, &k1520_zre_device::mem_map);
	m_maincpu->set_addrmap(AS_IO, &k1520_zre_device::io_map);
}

void k1520_zre_device::device_start()
{
	save_item(NAME(m_ram));
}

void k1520_zre_device::mem_map(address_map &map)
{
	map(0x0000, 0xffff).rw(FUNC(k1520_zre_device::bus_memory_r), FUNC(k1520_zre_device::bus_memory_w));
}

void k1520_zre_device::io_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0xff).rw(FUNC(k1520_zre_device::bus_io_r), FUNC(k1520_zre_device::bus_io_w));
}

u8 k1520_zre_device::bus_memory_r(offs_t offset)
{
	return m_bus->memory_r(offset);
}

void k1520_zre_device::bus_memory_w(offs_t offset, u8 data)
{
	m_bus->memory_w(offset, data);
}

u8 k1520_zre_device::bus_io_r(offs_t offset)
{
	return m_bus->io_r(offset);
}

void k1520_zre_device::bus_io_w(offs_t offset, u8 data)
{
	m_bus->io_w(offset, data);
}

bool k1520_zre_device::memory_r(offs_t offset, u8 &data)
{
	if (offset <= 0x0bff)
	{
		data = m_rom[offset];
		return true;
	}

	if (offset >= 0x0c00 && offset <= 0x0fff)
	{
		data = m_ram[offset & 0x3ff];
		return true;
	}

	return false;
}

bool k1520_zre_device::memory_w(offs_t offset, u8 data)
{
	if (offset < 0x0c00 || offset > 0x0fff)
		return false;

	m_ram[offset & 0x3ff] = data;
	return true;
}


k1520_zre_8786_device::k1520_zre_8786_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	k1520_zre_device(mconfig, K1520_ZRE_8786, tag, owner, clock),
	m_rom(*this, "rom")
{
}

bool k1520_zre_8786_device::memory_r(offs_t offset, u8 &data)
{
	if (offset <= 0x4fff)
	{
		data = m_rom[offset];
		return true;
	}

	return k1520_zre_device::memory_r(offset, data);
}


// K7024 (012-6820) ABS / K7028 (012-6710) ATS placeholder board

k1520_placeholder_card_device::k1520_placeholder_card_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K1520_PLACEHOLDER_CARD, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this)
{
}

void k1520_placeholder_card_device::device_start()
{
}
