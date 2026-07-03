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
DEFINE_DEVICE_TYPE(K1520_K7028, k1520_ats_k7028_device, "k1520_k7028", "K1520 K7028 ATS Board")


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

k1520_abs_k7024_device::k1520_abs_k7024_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock, u16 base_addr) :
	device_t(mconfig, K1520_ABS, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_videoram{ },
	m_chargen(*this, "chargen"),
	m_framecnt(0)
{
	m_base_addr = base_addr;
}

void k1520_abs_k7024_device::device_add_mconfig(machine_config &config)
{
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER, rgb_t::green()));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500));
	screen.set_screen_update(FUNC(k1520_abs_k7024_device::screen_update));
	screen.set_size(640, 300);
	screen.set_visarea(0, 639, 0, 299);
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
	if (offset < m_base_addr || offset > m_base_addr + 0x7ff) // bridge at x11:1 x12:1
		return false;

	data = m_videoram[offset - m_base_addr];
	return true;
}

bool k1520_abs_k7024_device::memory_w(offs_t offset, u8 data)
{
	if (offset < m_base_addr || offset > m_base_addr + 0x7ff) // bridge at x11:1 x12:1
		return false;

	m_videoram[offset - m_base_addr] = data;
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

				u8 chr = m_videoram[x];

				if ((chr & 0x80) && (m_framecnt & 0x08))
					chr = 0x20;

				chr &= 0x7f;

				if (ra < 8)
					gfx = m_chargen[(chr << 3) | ra];
				else
					gfx = m_chargen[(chr << 3) | (ra - 8) | 0x400];

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
	ROM_LOAD( "7024zg1.bin", 0x0000, 0x400, CRC(abf8e894) SHA1(53d7909f84fa929a531260efb30393e6ef39d654))
	ROM_LOAD( "7024zg2.bin", 0x0400, 0x400, CRC(aee4bd8d) SHA1(7d58b86fd0100dd13c70b7a10ae1347b70c1fe7f))
ROM_END

// K1520 K3820 (012-7040) PFS ROM board

k1520_pfs_7040_device::k1520_pfs_7040_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock, u16 base_addr) :
	device_t(mconfig, K1520_PFS, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_rom(*this, "rom")
{
	m_base_addr = base_addr;
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
	if (offset < m_base_addr || offset > m_base_addr + 0x3fff)
		return false;

	data = m_rom[offset - m_base_addr];
	return true;
}

// K1520 K7028 (012-6710) ATS keyboard interface board

k1520_ats_k7028_device::k1520_ats_k7028_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock, u8 base_addr = 0xf0) :
    device_t(mconfig, K1520_K7028, tag, owner, clock),
    device_k1520_card_interface(mconfig, *this),
    m_sio(*this, "sio"),
    m_ctc(*this, "ctc")
{
	m_base_addr = base_addr;
}

void k1520_ats_k7028_device::device_add_mconfig(machine_config &config)
{
    Z80CTC(config, m_ctc, XTAL(4'915'200));
    Z80SIO(config, m_sio, XTAL(4'915'200));
}

void k1520_ats_k7028_device::device_start()
{
}

bool k1520_ats_k7028_device::io_r(offs_t offset, u8 &data)
{
	if ((offset & 0xf0) == m_base_addr)
	{
		if ((offset & 0x0f) == 0x0c)
		{
			data = m_ctc->read(offset & 0x03);
			return true;
		}

		if ((offset & 0x0f) == 0x08)
		{
			data = m_sio->ba_cd_r(offset & 0x03);
			return true;
		}
	}
    return false;
}

bool k1520_ats_k7028_device::io_w(offs_t offset, u8 data)
{
	if ((offset & 0xf0) == m_base_addr)
	{
		if ((offset & 0x0f) == 0x0c)
		{
			m_ctc->write(offset & 0x03, data);
			return true;
		}

		if ((offset & 0x0f) == 0x08)
		{
			m_sio->ba_cd_w(offset & 0x03, data);
			return true;
		}
	}
    return false;
}


k1520_placeholder_card_device::k1520_placeholder_card_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K1520_PLACEHOLDER_CARD, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this)
{
}

void k1520_placeholder_card_device::device_start()
{
}
