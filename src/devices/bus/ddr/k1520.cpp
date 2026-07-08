// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Robotron K1520 bus

***************************************************************************/

#include "emu.h"
#include "k1520.h"

#include "zre.h"

#include "machine/keyboard.ipp"

DEFINE_DEVICE_TYPE(K1520_BUS, k1520_bus_device, "k1520_bus", "Robotron K1520 Bus")
DEFINE_DEVICE_TYPE(K1520_ABS, k1520_abs_k7024_device, "k1520_abs", "K1520 ABS Board")
DEFINE_DEVICE_TYPE(K1520_PFS, k1520_pfs_7040_device, "k8911_pfs", "K8911 PFS Board")
DEFINE_DEVICE_TYPE(K1520_PLACEHOLDER_CARD, k1520_placeholder_card_device, "k1520_placeholder", "K1520 Placeholder Board")
DEFINE_DEVICE_TYPE(K1520_ATS, k1520_ats_k7028_device, "k7028_ats", "K7028 ATS Board")


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
	m_gfxdecode(*this, "gfxdecode"),
	m_framecnt(0)
{
	m_base_addr = base_addr;
}

static const gfx_layout k7024_charlayout =
{
	8, 10,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 0x400*8, 0x401*8 },
	8*8
};

static GFXDECODE_START( gfx_k7024 )
	GFXDECODE_ENTRY( "chargen", 0, k7024_charlayout, 0, 1 )
GFXDECODE_END

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
	GFXDECODE(config, m_gfxdecode, "palette", gfx_k7024);
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
	gfx_element *const gfx = m_gfxdecode->gfx(0);
	u32 const rowbytes = gfx->rowbytes();
	u16 sy = 0, ma = 0;

	m_framecnt++;

	for (u8 y = 0; y < 25; y++)
	{
		for (u8 ra = 0; ra < 10; ra++)
		{
			u16 *p = &bitmap.pix(sy++);

			for (u16 x = ma; x < ma + 80; x++)
			{
				u8 chr = m_videoram[x];

				if ((chr & 0x80) && (m_framecnt & 0x08))
					chr = 0x20;

				chr &= 0x7f;

				u8 const *const data = gfx->get_data(chr);
				u32 const row = ra * rowbytes;

				*p++ = data[row + 0];
				*p++ = data[row + 1];
				*p++ = data[row + 2];
				*p++ = data[row + 3];
				*p++ = data[row + 4];
				*p++ = data[row + 5];
				*p++ = data[row + 6];
				*p++ = data[row + 7];
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

bool k1520_pfs_7040_device::memory_w(offs_t offset, u8 data)
{
	return offset >= m_base_addr && offset <= m_base_addr + 0x3fff;
}

// K1520 K7028 (012-6710) ATS keyboard interface board

k1520_ats_k7028_device::k1520_ats_k7028_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock, u8 base_addr = 0xe0) :
    device_t(mconfig, K1520_ATS, tag, owner, clock),
    device_k1520_card_interface(mconfig, *this),
	m_sio(*this, "sio"),
	m_ctc(*this, "ctc"),
	m_keyboard(*this, "keyboard"),
	m_keyboard_status_pending(false),
	m_keyboard_status(0),
	m_sio_loopback_data{ },
	m_sio_loopback_pending{ },
	m_printer_loopback_data{ },
	m_printer_loopback_head{ },
	m_printer_loopback_tail{ },
	m_printer_loopback_count{ }
{
	m_base_addr = base_addr;
}

namespace {

INPUT_PORTS_START( k7028_ats )
	PORT_INCLUDE(generic_keyboard)

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("K8911 Raw 1f") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(k1520_ats_k7028_device::special_key), 0x1f)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("K8911 Raw 8f") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6)) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(k1520_ats_k7028_device::special_key), 0x8f)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("K8911 Raw 91") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7)) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(k1520_ats_k7028_device::special_key), 0x91)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("K8911 Raw 95") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8)) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(k1520_ats_k7028_device::special_key), 0x95)
INPUT_PORTS_END

} // anonymous namespace

ioport_constructor k1520_ats_k7028_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(k7028_ats);
}

void k1520_ats_k7028_device::device_add_mconfig(machine_config &config)
{
    Z80CTC(config, m_ctc, XTAL(4'915'200));
	m_ctc->intr_callback().set(FUNC(k1520_ats_k7028_device::irq_w));
	m_ctc->zc_callback<0>().set(m_sio, FUNC(z80sio_device::rxca_w));
	m_ctc->zc_callback<0>().append(m_sio, FUNC(z80sio_device::txca_w));

    Z80SIO(config, m_sio, XTAL(4'915'200));
	m_sio->out_int_callback().set(FUNC(k1520_ats_k7028_device::irq_w));

	GENERIC_KEYBOARD(config, m_keyboard);
	m_keyboard->set_keyboard_callback(FUNC(k1520_ats_k7028_device::keyboard_put));
}

void k1520_ats_k7028_device::device_start()
{
	save_item(NAME(m_keyboard_status_pending));
	save_item(NAME(m_keyboard_status));
	save_item(NAME(m_sio_loopback_data));
	save_item(NAME(m_sio_loopback_pending));
	save_item(NAME(m_printer_loopback_data));
	save_item(NAME(m_printer_loopback_head));
	save_item(NAME(m_printer_loopback_tail));
	save_item(NAME(m_printer_loopback_count));
}

void k1520_ats_k7028_device::device_reset()
{
	m_keyboard_status_pending = false;
	m_keyboard_status = 0xa0;
	m_sio_loopback_data = { 0, 0 };
	m_sio_loopback_pending = { false, false };
	m_printer_loopback_head = { 0, 0 };
	m_printer_loopback_tail = { 0, 0 };
	m_printer_loopback_count = { 0, 0 };
}

void k1520_ats_k7028_device::irq_w(int state)
{
	if (m_bus)
		m_bus->irq_w(state);
}

void k1520_ats_k7028_device::keyboard_put(u8 data)
{
	m_keyboard_status_pending = true;
	m_keyboard_status = data;
}

INPUT_CHANGED_MEMBER(k1520_ats_k7028_device::special_key)
{
	if (newval)
		keyboard_put(u8(param));
}

bool k1520_ats_k7028_device::io_r(offs_t offset, u8 &data)
{
	if ((offset & 0xe0) == m_base_addr)
	{
		if ((offset & 0x1c) == 0x00)
		{
			switch (offset & 0x07)
			{
			case 0:
				if (m_keyboard_status_pending)
				{
					m_keyboard_status_pending = false;
					data = m_keyboard_status;
				}
				else
				{
					data = 0xff;
				}
				return true;

			case 1:
				data = m_keyboard_status_pending ? 0xf7 : 0xff;
				return true;

			case 3:
				data = 0xff;
				return true;

			case 4:
				data = 0xff;
				return true;
			}
		}

		if ((offset & 0x1c) == 0x10)
		{
			unsigned const channel = BIT(offset, 1);
			if ((offset & 0x01) == 0)
			{
				if (m_printer_loopback_count[channel] != 0)
				{
					data = m_printer_loopback_data[channel][m_printer_loopback_head[channel]];
					m_printer_loopback_head[channel] = (m_printer_loopback_head[channel] + 1) & 0x0f;
					m_printer_loopback_count[channel]--;
				}
				else
				{
					data = 0xff;
				}
				return true;
			}
			else
			{
				data = 0x04 | (m_printer_loopback_count[channel] != 0 ? 0x01 : 0x00);
				return true;
			}
		}

		if ((offset & 0x1c) == 0x14)
		{
			if ((offset & 0x01) == 0)
			{
				unsigned const channel = BIT(offset, 1);
				if (m_sio_loopback_pending[channel])
				{
					m_sio_loopback_pending[channel] = false;
					data = m_sio_loopback_data[channel];
					return true;
				}
			}

			data = m_sio->ba_cd_r(offset & 0x03);
			return true;
		}
		if ((offset & 0x18) == 0x18)
		{
			data = m_ctc->read(offset & 0x03);
			return true;
		}
	}
    return false;
}

bool k1520_ats_k7028_device::io_w(offs_t offset, u8 data)
{
	if ((offset & 0xe0) == m_base_addr)
	{
		if ((offset & 0x1c) == 0x00)
		{
			switch (offset & 0x07)
			{
			case 2:
				if (data == 0xfb)
				{
					m_keyboard_status_pending = true;
					m_keyboard_status = 0xa0;
				}
				else if (data == 0x00)
				{
					m_keyboard_status_pending = false;
				}
				return true;

			case 4:
				return true;
			}
		}

		if ((offset & 0x1c) == 0x10)
		{
			unsigned const channel = BIT(offset, 1);
			if ((offset & 0x01) == 0)
			{
				if (m_printer_loopback_count[channel] < m_printer_loopback_data[channel].size())
				{
					m_printer_loopback_data[channel][m_printer_loopback_tail[channel]] = data;
					m_printer_loopback_tail[channel] = (m_printer_loopback_tail[channel] + 1) & 0x0f;
					m_printer_loopback_count[channel]++;
				}
			}
			else
			{
				m_printer_loopback_head[channel] = 0;
				m_printer_loopback_tail[channel] = 0;
				m_printer_loopback_count[channel] = 0;
			}
			return true;
		}

		if ((offset & 0x1c) == 0x14)
		{
			unsigned const channel = BIT(offset, 1);
			if ((offset & 0x01) == 0)
			{
				m_sio_loopback_data[channel] = data;
				m_sio_loopback_pending[channel] = true;
			}
			else
			{
				m_sio_loopback_pending[channel] = false;
			}

			m_sio->ba_cd_w(offset & 0x03, data);
			return true;
		}
		if ((offset & 0x18) == 0x18)
		{
			m_ctc->write(offset & 0x03, data);
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
