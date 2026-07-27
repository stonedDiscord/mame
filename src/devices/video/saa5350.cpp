// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/**********************************************************************

    Philips/Signetics SAA5350 Single-Chip Color CRT Controller

    The SAA5350 (EUROM1) generates a 625-line color videotex display
    from external display RAM. It supports 40 or 80 columns, serial
    or parallel attributes, four ROM character tables, four DRCS
    tables, a scroll map and a programmable color map.

**********************************************************************/

#include "emu.h"
#include "saa5350.h"

#include "screen.h"

DEFINE_DEVICE_TYPE(SAA5350, saa5350_device, "saa5350", "SAA5350 EUROM1 CRT Controller")

ROM_START( saa5350 )
	ROM_REGION( 0x900, "chargen", 0 )
	ROM_LOAD( "saa5350.bin", 0x000, 0x900, BAD_DUMP CRC(ea73a1bb) SHA1(5acedd34225448ad5cd541c7a06917198ee38e7b) ) // Extracted from a Siemens/Commodore Btx decoder ROM
ROM_END


saa5350_device::saa5350_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, SAA5350, tag, owner, clock)
	, device_video_interface(mconfig, *this)
	, m_memory(*this, finder_base::DUMMY_TAG, -1)
	, m_char_rom(*this, "chargen")
	, m_registers{ 0 }
{
}

const tiny_rom_entry *saa5350_device::device_rom_region() const
{
	return ROM_NAME(saa5350);
}

void saa5350_device::device_config_complete()
{
	if (!has_screen())
		return;

	// CLKO is twice the 6 MHz input. One non-interlaced field is 312 lines.
	if (screen().refresh_attoseconds() == 0)
		screen().set_raw(clock() * 2, 768, 198, 678, 312, 6, 306);

	if (!screen().has_screen_update())
		screen().set_screen_update(*this, FUNC(saa5350_device::screen_update));
}

void saa5350_device::device_start()
{
	save_item(NAME(m_registers));
}

void saa5350_device::device_reset()
{
	std::fill(std::begin(m_registers), std::end(m_registers), 0);
}

u8 saa5350_device::read(offs_t offset)
{
	return m_registers[offset & 0x3f];
}

void saa5350_device::write(offs_t offset, u8 data)
{
	m_registers[offset & 0x3f] = data;
}

u16 saa5350_device::character_pattern(u8 code, unsigned line) const
{
	if (code < 0x20 || line >= 12)
		return 0;

	offs_t const offset = (code - 0x20) * 24 + line * 2;
	return (u16(m_char_rom[offset]) << 6) | m_char_rom[offset + 1];
}

u32 saa5350_device::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(rgb_t::black(), cliprect);

	constexpr unsigned columns = 40;
	constexpr unsigned rows = 25;
	constexpr unsigned cell_width = 12;
	constexpr unsigned cell_height = 12;
	offs_t const page_base = m_memory_base;
	int const x_origin = screen.visible_area().min_x;
	int const y_origin = screen.visible_area().min_y;

	for (unsigned row = 0; row < rows; row++)
	{
		for (unsigned line = 0; line < cell_height; line++)
		{
			u32 *const pixels = &bitmap.pix(y_origin + row * cell_height + line);
			for (unsigned column = 0; column < columns; column++)
			{
				u8 const code = m_memory->read_byte(page_base + row * columns + column);
				u16 const pattern = character_pattern(code & 0x7f, line);
				rgb_t const foreground = BIT(code, 7) ? rgb_t(0xff, 0xff, 0x40) : rgb_t::white();

				for (unsigned dot = 0; dot < cell_width; dot++)
					pixels[x_origin + column * cell_width + dot] = BIT(pattern, 11 - dot) ? foreground : rgb_t::black();
			}
		}
	}

	return 0;
}
