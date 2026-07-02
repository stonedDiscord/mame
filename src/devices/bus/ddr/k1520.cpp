// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Robotron K1520 bus

***************************************************************************/

#include "emu.h"
#include "k1520.h"

#include "zre.h"

DEFINE_DEVICE_TYPE(K1520_BUS, k1520_bus_device, "k1520_bus", "Robotron K1520 Bus")
DEFINE_DEVICE_TYPE(K1520_ABS, k1520_abs_k7024_device, "k1520_abs", "K1520 ABS Board")
DEFINE_DEVICE_TYPE(K1520_PFS, k1520_pfs_7040_device, "k8911_pfs", "K1520 PFS Board")
DEFINE_DEVICE_TYPE(K1520_PLACEHOLDER_CARD, k1520_placeholder_card_device, "k1520_placeholder", "K1520 Placeholder Board")


k1520_bus_device::k1520_bus_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K1520_BUS, tag, owner, clock),
	m_cards{ },
	m_out_irq_cb(*this),
	m_out_nmi_cb(*this)
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

	logerror("unmapped memory read from %04X\n", offset);
	return data;
}

void k1520_bus_device::memory_w(offs_t offset, u8 data)
{
	bool handled = false;
	for (device_k1520_card_interface *card : m_cards)
		if (card && card->memory_w(offset, data))
			handled = true;

	if (!handled)
		logerror("unmapped memory write to %04X = %02X\n", offset, data);
}

u8 k1520_bus_device::io_r(offs_t offset)
{
	u8 data = 0xff;

	for (device_k1520_card_interface *card : m_cards)
		if (card && card->io_r(offset, data))
			return data;

	logerror("unmapped io read from %02X\n", offset);
	return data;
}

void k1520_bus_device::io_w(offs_t offset, u8 data)
{
	bool handled = false;
	for (device_k1520_card_interface *card : m_cards)
		if (card && card->io_w(offset, data))
			handled = true;

	if (!handled)
		logerror("unmapped io write to %02X = %02X\n", offset, data);
}

void k1520_bus_device::irq_w(int state)
{
	m_out_irq_cb(state);
}

void k1520_bus_device::nmi_w(int state)
{
	m_out_nmi_cb(state);
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


// K1520 K7024 (012-6820) ABS display board

k1520_abs_k7024_device::k1520_abs_k7024_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K1520_ABS, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_videoram{ },
	m_chargen(*this, "chargen"),
	m_framecnt(0)
{
}

void k1520_abs_k7024_device::device_add_mconfig(machine_config &config)
{
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER, rgb_t::green()));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500));
	screen.set_screen_update(FUNC(k1520_abs_k7024_device::screen_update));
	screen.set_size(640, 250);
	screen.set_visarea(0, 639, 0, 249);
	screen.set_palette("palette");

	PALETTE(config, "palette", palette_device::MONOCHROME);
}

void k1520_abs_k7024_device::device_start()
{
	save_item(NAME(m_videoram));
	save_item(NAME(m_framecnt));
}

bool k1520_abs_k7024_device::memory_r(offs_t offset, u8 &data)
{
	if (offset < 0x1000 || offset > 0x17ff)
		return false;

	data = m_videoram[offset - 0x1000];
	return true;
}

bool k1520_abs_k7024_device::memory_w(offs_t offset, u8 data)
{
	if (offset < 0x1000 || offset > 0x17ff)
		return false;

	m_videoram[offset - 0x1000] = data;
	return true;
}

u32 k1520_abs_k7024_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, rectangle const &cliprect)
{
	u16 sy = 0, ma = 0;

	m_framecnt++;

	for (u8 y = 0; y < 25; y++)
	{
		for (u8 ra = 0; ra < 10; ra++)
		{
			u16 *p = &bitmap.pix(sy++);

			for (u16 x = ma; x < ma + 80; x++)
			{
				u8 gfx = 0;

				if (ra < 9)
				{
					u8 chr = m_videoram[x];

					if ((chr & 0x80) && (m_framecnt & 0x08))
						chr = 0x20;

					chr &= 0x7f;

					gfx = m_chargen[(chr << 4) | ra];
				}

				*p++ = BIT(gfx, 7);
				*p++ = BIT(gfx, 6);
				*p++ = BIT(gfx, 5);
				*p++ = BIT(gfx, 4);
				*p++ = BIT(gfx, 3);
				*p++ = BIT(gfx, 2);
				*p++ = BIT(gfx, 1);
				*p++ = BIT(gfx, 0);
			}
		}
		ma += 80;
	}
	return 0;
}

ROM_START( k1520_abs )
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf) )
ROM_END

// K1520 K3820 (012-7040) PFS ROM board

k1520_pfs_7040_device::k1520_pfs_7040_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K1520_PFS, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_rom(*this, "rom")
{
}

void k1520_pfs_7040_device::set_slot(k1520_bus_device &bus, unsigned slot)
{
	set_bus(bus, slot);
}

void k1520_pfs_7040_device::device_start()
{
}

bool k1520_pfs_7040_device::memory_r(offs_t offset, u8 &data)
{
	if (offset < 0x2000 || offset > 0x7fff)
		return false;

	data = m_rom[offset - 0x2000];
	return true;
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
