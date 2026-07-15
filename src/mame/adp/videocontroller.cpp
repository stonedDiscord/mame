// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina,stonedDiscord

/*

Video Board:
------------
 ____________________________________________________________
 |           ______________  ______________                 |
 |           | t2 i       |  |KM681000ALP7|     74HC573     |
 |           |____________|  |____________|                *|
 |                                              74HC573    *|
 |           ______________  ______________                *|
 |           | t2 ii      |  |KM681000ALP7|               P3|
 |       ||| |____________|  |____________|   |||          *|
 |       ||| ___________                      |||          *|
 |       ||| |         |                      |||          *|
 |       ||| | HD63484 |  74HC04   74HC00     |||         P6|
 |       ||| |         |  74HC74   74HC08     |||  74HC245  |
 |           |         |                                    |
 | 74HC573   |_________|  74HC166  74HC166 74HC166 74HC166  |
 |__________________________________________________________|

Parts:

 HD63484CP8         - Advanced CRT Controller
 KM681000ALP7       - 128K X 8 Bit Low Power CMOS Static RAM

Connectors:

 Two connectors to link with CPU Board
 Two connectors to link with Sound and I/O Board
 P3  - Monitor
 P6  - Lightpen

*/

#include "emu.h"
#include "videocontroller.h"

DEFINE_DEVICE_TYPE(ADP_VIDEOCONTROLLER, adp_videocontroller_device, "adp_videocontroller", "ADP Videocontroller")
DEFINE_DEVICE_TYPE(ADP_VIDEOCONTROLLER_ENCODER, adp_videocontroller_encoder_device, "adp_videocontroller_encoder", "ADP Videocontroller (RGB Encoder)")

adp_videocontroller_device::adp_videocontroller_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	adp_videocontroller_device(mconfig, ADP_VIDEOCONTROLLER, tag, owner, clock)
{
}

adp_videocontroller_device::adp_videocontroller_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, type, tag, owner, clock),
	m_acrtc(*this, "acrtc"),
	m_palette(*this, "palette")
{
}

adp_videocontroller_encoder_device::adp_videocontroller_encoder_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	adp_videocontroller_device(mconfig, ADP_VIDEOCONTROLLER_ENCODER, tag, owner, clock),
	m_ramdac(*this, "ramdac")
{
}

void adp_videocontroller_device::device_add_mconfig(machine_config &config)
{
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500));
	screen.set_size(384, 280);
	screen.set_visarea_full();
	screen.set_screen_update(m_acrtc, FUNC(hd63484_device::update_screen));
	screen.set_palette(m_palette);

	PALETTE(config, m_palette, FUNC(adp_videocontroller_device::palette_init), 0x100);
	HD63484(config, m_acrtc).set_addrmap(0, &adp_videocontroller_device::hd63484_map);
}

void adp_videocontroller_encoder_device::device_add_mconfig(machine_config &config)
{
	adp_videocontroller_device::device_add_mconfig(config);
	palette_device &palette(PALETTE(config.replace(), palette_finder(), FUNC(adp_videocontroller_encoder_device::palette_init), 0x100));
	ramdac_device &ramdac(RAMDAC(config, m_ramdac, palette));
	ramdac.set_addrmap(0, &adp_videocontroller_encoder_device::ramdac_map);
}

adp_videocontroller_device &adp_videocontroller_device::set_high_resolution(bool enabled)
{
	m_high_resolution = enabled;
	screen_device &screen(*subdevice<screen_device>("screen"));
	screen.set_size(enabled ? 640 : 384, enabled ? 480 : 280);
	screen.set_visarea_full();
	return *this;
}

void adp_videocontroller_device::device_start()
{
}

void adp_videocontroller_device::palette_init(palette_device &palette) const
{
	for (int i = 0; i < palette.entries(); i++)
		palette.set_pen_color(i, rgb_t(0x21 * BIT(i, 0) + 0x47 * BIT(i, 3) + 0x97 * BIT(i, 0), 0x21 * BIT(i, 1) + 0x47 * BIT(i, 3) + 0x97 * BIT(i, 1), 0x21 * BIT(i, 2) + 0x47 * BIT(i, 3) + 0x97 * BIT(i, 2)));
}

void adp_videocontroller_device::hd63484_map(address_map &map)
{
	switch (m_layout)
	{
	case memory_layout::STANDARD:
		map(0x00000, 0x1ffff).mirror(0x60000).ram();
		map(0x80000, 0x9ffff).mirror(0x60000).rom().region(m_gfx_region, 0);
		break;
	case memory_layout::EXTENDED_ROM:
		map(0x00000, 0x1ffff).mirror(0x60000).ram();
		map(0x80000, 0xfffff).rom().region(m_gfx_region, 0);
		break;
	case memory_layout::ROM_THEN_RAM:
		map(0x00000, 0x7ffff).rom().region(m_gfx_region, 0);
		map(0x80000, 0xfffff).ram();
		break;
	}
}

void adp_videocontroller_encoder_device::palette_init(palette_device &palette) const
{
	if (high_resolution())
	{
		for (int i = 0; i < palette.entries(); i++)
			palette.set_pen_color(i, rgb_t(pal3bit(i >> 5), pal3bit(i >> 2), pal2bit(i)));
	}
}

void adp_videocontroller_encoder_device::ramdac_map(address_map &map)
{
	map(0x000, 0x3ff).rw(m_ramdac, FUNC(ramdac_device::ramdac_pal_r), FUNC(ramdac_device::ramdac_rgb666_w));
}

u16 adp_videocontroller_device::read(offs_t offset, u16 mem_mask)
{
	if (offset < 2)
		return m_acrtc->read16(offset);
	return 0xffff;
}

void adp_videocontroller_device::write(offs_t offset, u16 data, u16 mem_mask)
{
	if (offset < 2)
		m_acrtc->write16(offset, data);
}

void adp_videocontroller_encoder_device::write(offs_t offset, u16 data, u16 mem_mask)
{
	adp_videocontroller_device::write(offset, data, mem_mask);
	if ((offset >= 4) && ACCESSING_BITS_0_7)
	{
		switch (offset)
		{
		case 4: m_ramdac->index_w(data); break;
		case 5: m_ramdac->pal_w(data); break;
		case 6: m_ramdac->mask_w(data); break;
		}
	}
}
