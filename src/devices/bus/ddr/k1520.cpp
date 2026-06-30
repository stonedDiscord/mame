// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Robotron K1520 bus

***************************************************************************/

#include "emu.h"
#include "k1520.h"


DEFINE_DEVICE_TYPE(K1520_BUS, k1520_bus_device, "k1520_bus", "Robotron K1520 Bus")
DEFINE_DEVICE_TYPE(K1520_ZRE, k1520_zre_7100_device, "k1520_zre", "K1520 K2521 ZRE Board")
DEFINE_DEVICE_TYPE(K1520_ZRE_8786, k1520_zre_8786_device, "k1520_zre_8786", "K1520 045-8786 ZRE Board")
DEFINE_DEVICE_TYPE(K1520_ABS, k1520_abs_6820_device, "k1520_abs", "K1520 K7024 ABS Board")
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

k1520_zre_7100_device::k1520_zre_7100_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	k1520_zre_7100_device(mconfig, K1520_ZRE, tag, owner, clock)
{
}

k1520_zre_7100_device::k1520_zre_7100_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_maincpu(*this, "maincpu"),
	m_ctc(*this, "ctc"),
	m_sio(*this, "sio"),
	m_rom(*this, "rom"),
	m_ram{ }
{
}

void k1520_zre_7100_device::device_add_mconfig(machine_config &config)
{
	Z80(config, m_maincpu, XTAL(9'830'400) / 4);
	m_maincpu->set_addrmap(AS_PROGRAM, &k1520_zre_7100_device::mem_map);
	m_maincpu->set_addrmap(AS_IO, &k1520_zre_7100_device::io_map);

	z80ctc_device &ctc(Z80CTC(config, m_ctc, XTAL(9'830'400) / 4));
	ctc.set_clk<0>(XTAL(9'830'400) / 4);
	ctc.set_clk<1>(XTAL(9'830'400) / 4);
	ctc.set_clk<2>(XTAL(9'830'400) / 4);
	ctc.set_clk<3>(XTAL(9'830'400) / 4);
	ctc.zc_callback<0>().set(m_sio, FUNC(z80sio_device::rxtxcb_w));

	Z80SIO(config, m_sio, XTAL(9'830'400) / 4);
}

void k1520_zre_7100_device::device_start()
{
	save_item(NAME(m_ram));
}

void k1520_zre_7100_device::mem_map(address_map &map)
{
	map(0x0000, 0xffff).rw(FUNC(k1520_zre_7100_device::bus_memory_r), FUNC(k1520_zre_7100_device::bus_memory_w));
}

void k1520_zre_7100_device::io_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0xff).rw(FUNC(k1520_zre_7100_device::bus_io_r), FUNC(k1520_zre_7100_device::bus_io_w));
	map(0x50, 0x53).rw(m_sio, FUNC(z80sio_device::ba_cd_r), FUNC(z80sio_device::ba_cd_w));
	map(0x80, 0x83).rw(m_ctc, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
}

u8 k1520_zre_7100_device::bus_memory_r(offs_t offset)
{
	return m_bus->memory_r(offset);
}

void k1520_zre_7100_device::bus_memory_w(offs_t offset, u8 data)
{
	m_bus->memory_w(offset, data);
}

u8 k1520_zre_7100_device::bus_io_r(offs_t offset)
{
	return m_bus->io_r(offset);
}

void k1520_zre_7100_device::bus_io_w(offs_t offset, u8 data)
{
	m_bus->io_w(offset, data);
}

bool k1520_zre_7100_device::memory_r(offs_t offset, u8 &data)
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

bool k1520_zre_7100_device::memory_w(offs_t offset, u8 data)
{
	if (offset < 0x0c00 || offset > 0x0fff)
		return false;

	m_ram[offset & 0x3ff] = data;
	return true;
}


k1520_zre_8786_device::k1520_zre_8786_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	k1520_zre_7100_device(mconfig, K1520_ZRE_8786, tag, owner, clock),
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

	return k1520_zre_7100_device::memory_r(offset, data);
}


// K1520 K7024 (012-6820) ABS display board

k1520_abs_6820_device::k1520_abs_6820_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K1520_ABS, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_videoram{ },
	m_chargen(*this, "chargen"),
	m_framecnt(0)
{
}

void k1520_abs_6820_device::device_add_mconfig(machine_config &config)
{
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER, rgb_t::green()));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500));
	screen.set_screen_update(FUNC(k1520_abs_6820_device::screen_update));
	screen.set_size(640, 250);
	screen.set_visarea(0, 639, 0, 249);
	screen.set_palette("palette");

	PALETTE(config, "palette", palette_device::MONOCHROME);
}

void k1520_abs_6820_device::device_start()
{
	save_item(NAME(m_videoram));
	save_item(NAME(m_framecnt));
}

bool k1520_abs_6820_device::memory_r(offs_t offset, u8 &data)
{
	if (offset < 0x1000 || offset > 0x17ff)
		return false;

	data = m_videoram[offset - 0x1000];
	return true;
}

bool k1520_abs_6820_device::memory_w(offs_t offset, u8 data)
{
	if (offset < 0x1000 || offset > 0x17ff)
		return false;

	m_videoram[offset - 0x1000] = data;
	return true;
}

u32 k1520_abs_6820_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, rectangle const &cliprect)
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


// K7024 (012-6820) ABS / K7028 (012-6710) ATS placeholder board

k1520_placeholder_card_device::k1520_placeholder_card_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K1520_PLACEHOLDER_CARD, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this)
{
}

void k1520_placeholder_card_device::device_start()
{
}
