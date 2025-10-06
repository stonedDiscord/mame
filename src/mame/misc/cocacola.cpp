// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/*
 *
 *********************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "machine/6850acia.h"
#include "machine/mc146818.h"
#include "video/hd44780.h"
#include "emupal.h"
#include "screen.h"

namespace {

class cocacola_state : public driver_device
{
public:
	cocacola_state(const machine_config &mconfig, device_type type, const char * tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_pia(*this, "pia")
		, m_acia0(*this, "acia0")
		, m_acia1(*this, "acia1")
		, m_rtc(*this, "rtc")
		, m_lcd(*this, "lcd")

	{ }

	void cocacola(machine_config &config);

protected:
	HD44780_PIXEL_UPDATE(pixel_update);

	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

	required_device<m6802_cpu_device> m_maincpu;
	required_device<pia6821_device> m_pia;
	required_device<acia6850_device> m_acia0;
	required_device<acia6850_device> m_acia1;
	required_device<mc146818_device> m_rtc;

	required_device<hd44780_device> m_lcd;

	void cocacola_map(address_map &map);
};

HD44780_PIXEL_UPDATE(cocacola_state::pixel_update)
{
	if (x < 5 && y < 8 && line < 2 && pos < 8)
		bitmap.pix(y, (line * 8 + pos) * 6 + x) = state;
}

void cocacola_state::machine_reset()
{
	m_maincpu->reset();
}

void cocacola_state::machine_start()
{

}

void cocacola_state::cocacola_map(address_map &map)
{
	map(0x0000, 0x07ff).ram();
	map(0x4000, 0x47ff).ram();
	map(0x6000, 0x67ff).ram();
	map(0x8000, 0xffff).rom().region("maincpu", 0);
}

//===================

static INPUT_PORTS_START( cocacola )
	PORT_START("INA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_BUTTON7 )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x00, "Light barrier" )  PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Setup" )    PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Test" )    PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Empty lamps" )    PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Clear" )    PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x10, DEF_STR( On ) )
INPUT_PORTS_END

void cocacola_state::cocacola(machine_config &config)
{
	m6802_cpu_device &maincpu(M6802(config, m_maincpu, XTAL(4'000'000))); // TODO: verify clock
	maincpu.set_ram_enable(false);
	maincpu.set_addrmap(AS_PROGRAM, &cocacola_state::cocacola_map);

	PIA6821(config, m_pia);

	ACIA6850(config, m_acia0, 0);
	ACIA6850(config, m_acia1, 0);

	MC146818(config, m_rtc, 32.768_kHz_XTAL);

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_LCD));
	screen.set_refresh_hz(50);
	screen.set_screen_update(m_lcd, FUNC(hd44780_device::screen_update));
	screen.set_size(16*6, 16);
	screen.set_visarea(0, 16*6-1, 0, 16-1);
	screen.set_palette("palette");

	hd44780_device &hd44780(HD44780(config, m_lcd, 270'000));
	hd44780.set_lcd_size(2, 16);
	hd44780.set_pixel_update_cb(FUNC(cocacola_state::pixel_update));

	PALETTE(config, "palette").set_entries(2);
}

ROM_START( cocacola )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD( "hofer_cckaz_6.0.u9", 0x00, 0x8000, CRC(80931f30) SHA1(a01261df38c2996b55bdfe973b78f4ccaec0a2f7) )
ROM_END

} // anonymous namespace


//    YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT    CLASS          INIT        COMPANY               FULLNAME           FLAGS
COMP( 1990, cocacola,    0,      0, cocacola, cocacola,cocacola_state,empty_init, u8"Höfer",           "Coca-Cola",        MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
