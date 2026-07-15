// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina
/*

adp Gauselmann (Merkur) games from '90 running on similar hardware.
(68k + HD63484 + YM2149)

Skeleton driver by TS

TODO:
 - protection in Fashion Gambler (NVRam based?) Update 2023: outdated note?
 - fstation: Suntris crashes when started it up, culprit is HD63484 paint command
   trying to write (to ROM) with negative XY values;

Supported games :
- Quick Jack      ("COPYRIGHT BY ADP LUEBBECKE GERMANY 1993")
- Skat TV           ("COPYRIGHT BY ADP LUEBBECKE GERMANY 1994")
- Skat TV v. TS3  ("COPYRIGHT BY ADP LUEBBECKE GERMANY 1995")
- Fashion Gambler ("COPYRIGHT BY ADP LUEBBECKE GERMANY 1997")
- Funny Land de Luxe ("Copyright 1992-99 by Stella International Germany")
- Fun Station Spielekoffer 9 Spiele ("COPYRIGHT BY ADP LUEBBECKE GERMANY 2000")


Skat TV (Version TS3)
Three board stack.

There's also (external) JAMMA adapter - 4th board filled with resistors and diodes.

Funny Land de Luxe
------------------

Video board has additional chips:
  - Altera EPM7032 (PLD)
  - SG-615PH (32.0000M oscillator)
  - Bt481 (RAMDAC)



Quick Jack administration/service mode:
- hold down Start and Joker buttons at start
- enter the default CODENUMBER 54321 using the hand buttons
- confirm the CODENUMBER with start

*/

#include "emu.h"
#include "adp_sus.h"
#include "videocontroller.h"
#include "machine/microtch.h"


namespace {

class adp_state : public driver_device
{
public:
	adp_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_microtouch(*this, "microtouch"),
		m_sus(*this, "sus"),
		m_steuereinheit(*this, "steuereinheit"),
		m_video(*this, "videocontroller0"),
		m_in0(*this, "IN0")
	{ }

	void skattv(machine_config &config);
	void quickjac(machine_config &config);
	void fashiong(machine_config &config);
	void fstation(machine_config &config);
	void funland(machine_config &config);
	void skattva(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	void adp_common(machine_config &config);

	optional_device<microtouch_device> m_microtouch;
	required_device<adp_sus_device> m_sus;
	required_device<adp_steuereinheit_device> m_steuereinheit;
	required_device<adp_videocontroller_device> m_video;
	required_ioport m_in0;

	/* misc */
	uint8_t m_mux_data;

	/* devices */
	uint16_t input_r();
	void input_w(uint16_t data);
	//INTERRUPT_GEN_MEMBER(adp_int);
};



/***************************************************************************

    68681 DUART <-> Microtouch touch screen controller communication

***************************************************************************/

void adp_state::machine_start()
{
	save_item(NAME(m_mux_data));
}

void adp_state::machine_reset()
{
	m_mux_data = 0;
}

uint16_t adp_state::input_r()
{
	uint16_t data = 0xffff;

	data &= ~(BIT(m_in0->read(), m_mux_data) ? 0x0000 : 0x0004);

	return data;
}

void adp_state::input_w(uint16_t data)
{
	m_mux_data++;
	m_mux_data &= 0x0f;
}



static INPUT_PORTS_START( quickjac )
	PORT_START("PA")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Collect")
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Hand 1")
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Hand 2")
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Hand 3")
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Hand 4")
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Hand 5")
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Joker")
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Low Battery" )
	PORT_DIPSETTING(     0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x00, DEF_STR( On ) )
	PORT_BIT( 0x3e, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static INPUT_PORTS_START( skattv )
	PORT_START("PA")
	PORT_BIT( 0x9f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("videocontroller0:screen", FUNC(screen_device::hblank))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("videocontroller0:screen", FUNC(screen_device::vblank))

	PORT_START("DSW1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_COIN5    )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_COIN6    )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_BILL1    )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN  )

	PORT_START("IN0")
	PORT_DIPNAME( 0x0001,0x0001, "SW0" ) //vblank status?
	PORT_DIPSETTING(     0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_DIPNAME( 0x0004,0x0004, "SW2" ) //another up button
	PORT_DIPSETTING(     0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008,0x0008, "SW3" )
	PORT_DIPSETTING(     0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x0020,0x0020, "SW5" )
	PORT_DIPSETTING(     0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_DIPNAME( 0x0100,0x0100, "SW8" )
	PORT_DIPSETTING(     0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200,0x0200, "SW9" )    //button 2
	PORT_DIPSETTING(     0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_DIPNAME( 0x0800,0x0800, "SW11" )
	PORT_DIPSETTING(     0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000,0x1000, "SW12" )   //button 3
	PORT_DIPSETTING(     0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000,0x2000, "SW13" )
	PORT_DIPSETTING(     0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

static INPUT_PORTS_START( skattva )
	PORT_START("PA")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("videocontroller0:screen", FUNC(screen_device::hblank))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("videocontroller0:screen", FUNC(screen_device::vblank))
	PORT_BIT( 0x9e, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("DSW1")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static INPUT_PORTS_START( fstation )
	PORT_START("PA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("videocontroller0:screen", FUNC(screen_device::hblank))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("videocontroller0:screen", FUNC(screen_device::vblank))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("DSW1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_DIPNAME( 0x0010,0x0010, "SW4" )
	PORT_DIPSETTING(     0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020,0x0020, "SW5" )
	PORT_DIPSETTING(     0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x0080,0x0080, "SW7" )
	PORT_DIPSETTING(     0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100,0x0100, "SW8" )
	PORT_DIPSETTING(     0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200,0x0200, "SW9" )
	PORT_DIPSETTING(     0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400,0x0400, "SW10" )
	PORT_DIPSETTING(     0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800,0x0800, "SW11" )
	PORT_DIPSETTING(     0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000,0x1000, "SW12" )
	PORT_DIPSETTING(     0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000,0x2000, "SW13" )
	PORT_DIPSETTING(     0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(     0x0000, DEF_STR( On ) )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

/*
INTERRUPT_GEN_MEMBER(adp_state::adp_int)
{
    device.execute().set_input_line(1, HOLD_LINE); // ??? All irqs have the same vector, and the mask used is 0 or 7
}
*/

void adp_state::quickjac(machine_config &config)
{
	ADP_SUS_RTC(config, m_sus, 0);
	adp_common(config);
	ADP_VIDEOCONTROLLER(config, m_video, 0);

	m_video->set_gfx_region(":gfx1");
	m_steuereinheit->input_callback().set_ioport("IN0");
	m_steuereinheit->serial_a_tx_callback().set(m_microtouch, FUNC(microtouch_device::rx));
	MICROTOUCH(config, m_microtouch, 9600).stx().set(m_steuereinheit, FUNC(adp_steuereinheit_device::serial_a_rx_w));
}

void adp_state::adp_common(machine_config &config)
{
	ADP_STEUEREINHEIT(config, m_steuereinheit, 0);
	m_steuereinheit->irq_callback().set_inputline("sus:maincpu", M68K_IRQ_4);
	m_steuereinheit->subdevice<ay8910_device>("aysnd")->port_a_read_callback().set_ioport("PA");
	m_steuereinheit->duart_input_callback().set_ioport("DSW1");
}

void adp_state::skattv(machine_config &config)
{
	ADP_SUS_TK(config, m_sus, 0);
	adp_common(config);
	ADP_VIDEOCONTROLLER(config, m_video, 0);

	m_video->set_gfx_region(":gfx1");
	m_steuereinheit->input_callback().set(FUNC(adp_state::input_r));
	m_steuereinheit->output_callback().set(FUNC(adp_state::input_w));
	m_steuereinheit->serial_a_tx_callback().set(m_microtouch, FUNC(microtouch_device::rx));
	MICROTOUCH(config, m_microtouch, 9600).stx().set(m_steuereinheit, FUNC(adp_steuereinheit_device::serial_a_rx_w));
}

void adp_state::skattva(machine_config &config)
{
	ADP_SUS_RTC(config, m_sus, 0);
	adp_common(config);
	ADP_VIDEOCONTROLLER(config, m_video, 0);

	m_sus->set_skattva_nvram_init();
	m_video->set_gfx_region(":gfx1");
	m_steuereinheit->input_callback().set_ioport("IN0");
	m_steuereinheit->serial_a_tx_callback().set(m_microtouch, FUNC(microtouch_device::rx));
	MICROTOUCH(config, m_microtouch, 9600).stx().set(m_steuereinheit, FUNC(adp_steuereinheit_device::serial_a_rx_w));
}

void adp_state::fashiong(machine_config &config)
{
	ADP_SUS_TK(config, m_sus, 0);
	adp_common(config);
	ADP_VIDEOCONTROLLER(config, m_video, 0).set_gfx_region(":gfx1").set_memory_layout(adp_videocontroller_device::memory_layout::EXTENDED_ROM);

	m_steuereinheit->input_callback().set(FUNC(adp_state::input_r));
	m_steuereinheit->output_callback().set(FUNC(adp_state::input_w));
	m_steuereinheit->serial_a_tx_callback().set(m_microtouch, FUNC(microtouch_device::rx));
	MICROTOUCH(config, m_microtouch, 9600).stx().set(m_steuereinheit, FUNC(adp_steuereinheit_device::serial_a_rx_w));
}

void adp_state::funland(machine_config &config)
{
	ADP_SUS_RTC(config, m_sus, 0);
	adp_common(config);
	ADP_VIDEOCONTROLLER_ENCODER(config, m_video, 0).set_gfx_region(":gfx1").set_memory_layout(adp_videocontroller_device::memory_layout::ROM_THEN_RAM);

	m_steuereinheit->input_callback().set_ioport("IN0");
	m_steuereinheit->serial_a_tx_callback().set(m_microtouch, FUNC(microtouch_device::rx));
	MICROTOUCH(config, m_microtouch, 9600).stx().set(m_steuereinheit, FUNC(adp_steuereinheit_device::serial_a_rx_w));
}

void adp_state::fstation(machine_config &config)
{
	ADP_SUS_TK(config, m_sus, 0);
	adp_common(config);
	ADP_VIDEOCONTROLLER_ENCODER(config, m_video, 0);

	m_video->set_gfx_region(":gfx1").set_memory_layout(adp_videocontroller_device::memory_layout::ROM_THEN_RAM).set_high_resolution(true);
	m_steuereinheit->input_callback().set(FUNC(adp_state::input_r));
	m_steuereinheit->output_callback().set(FUNC(adp_state::input_w));
}


ROM_START( quickjac )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "quick_jack_index_a.1.u2", 0x00000, 0x10000, CRC(c2fba6fe) SHA1(f79e5913f9ded1e370cc54dd55860263b9c51d61) )
	ROM_LOAD16_BYTE( "quick_jack_index_a.2.u6", 0x00001, 0x10000, CRC(210cb89b) SHA1(8eac60d40b60e845f9c02fee6c447f125ba5d1ab) )

	ROM_REGION16_BE( 0x40000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "quick_jack_video_inde_a.1.u2", 0x00000, 0x20000, CRC(73c27fc6) SHA1(12429bc0009b7754e08d2b6a5e1cd8251ab66e2d) )
	ROM_LOAD16_BYTE( "quick_jack_video_inde_a.2.u6", 0x00001, 0x20000, CRC(61d55be2) SHA1(bc17dc91fd1ef0f862eb0d7dbbbfa354a8403eb8) )
ROM_END

ROM_START(sbsoli)
	ROM_REGION16_BE( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "skat_bierskat_solitaire_f2_i.u2", 0x00000, 0x20000, CRC(314390cf) SHA1(86c2f4e120235eba379ec54f2afea59e68c94e7e))
	ROM_LOAD16_BYTE( "skat_bierskat_solitaire_f2_ii.u6", 0x00001, 0x20000, CRC(6e7f88cc) SHA1(07e306222cbd94ab7a39be2685941d12c82645fb))

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "skat_bier_solitaire_video_f1_i.u2", 0x00000, 0x80000, CRC(3726a21e) SHA1(63fd2f01ce6103ef9a2c585f1045091dfc4b3408))
	ROM_LOAD16_BYTE( "skat_bier_solitaire_video_f1_ii.u5", 0x00001, 0x80000, CRC(9109774f) SHA1(480a7cd4260dc3481049b108eba749de7eeedfa3))
ROM_END

ROM_START( skattv )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "f2_i.u2", 0x00000, 0x20000, CRC(3cb8b431) SHA1(e7930876b6cd4cba837c3da05d6948ef9167daea) )
	ROM_LOAD16_BYTE( "f2_ii.u6", 0x00001, 0x20000, CRC(0db1d2d5) SHA1(a29b0299352e0b2b713caf02aa7978f2a4b34e37) )

	ROM_REGION16_BE( 0x40000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "f1_i.u2", 0x00000, 0x20000, CRC(4869a889) SHA1(ad9f3fcdfd3630f9ad5b93a9d2738de9fc3514d3) )
	ROM_LOAD16_BYTE( "f1_ii.u5", 0x00001, 0x20000, CRC(17681537) SHA1(133685854b2080aaa3d0cced0287bc454d1f3bfc) )
ROM_END

ROM_START( skattva )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "skat_tv_version_ts3.1.u2", 0x00000, 0x20000, CRC(68f82fe8) SHA1(d5f9cb600531cdd748616d8c042b6a151ebe205a) )
	ROM_LOAD16_BYTE( "skat_tv_version_ts3.2.u6", 0x00001, 0x20000, CRC(4f927832) SHA1(bbe013005fd00dd42d12939eab5c80ec44a54b71) )

	ROM_REGION16_BE( 0x40000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "skat_tv_videoprom_t2.1.u2", 0x00000, 0x20000, CRC(de6f275b) SHA1(0c396fa4d1975c8ccc4967d330b368c0697d2124) )
	ROM_LOAD16_BYTE( "skat_tv_videoprom_t2.2.u5", 0x00001, 0x20000, CRC(af3e60f9) SHA1(c88976ea42cf29a092fdee18377b32ffe91e9f33) )
ROM_END

ROM_START( fashiong )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "fashion_gambler_s6_i.u2", 0x00000, 0x80000, CRC(827a164d) SHA1(dc16380226cabdefbfd893cb50cbfca9e134be40) )
	ROM_LOAD16_BYTE( "fashion_gambler_s6_ii.u6", 0x00001, 0x80000, CRC(5a2466d1) SHA1(c113a2295beed2011c70887a1f2fcdec00b055cb) )

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "fashion_gambler_video_s2_i.u2", 0x00000, 0x80000, CRC(d1ee9133) SHA1(e5fdfa303a3317f8f5fbdc03438ee97415afff4b) )
	ROM_LOAD16_BYTE( "fashion_gambler_video_s2_ii.u5", 0x00001, 0x80000, CRC(07b1e722) SHA1(594cbe9edfea6b04a4e49d1c1594f1c3afeadef5) )

	ROM_REGION( 0x4000, "user1", 0 )
	//nvram - 16 bit
	ROM_LOAD16_BYTE( "m48z08post.bin", 0x0000, 0x2000, CRC(2d317a04) SHA1(c690c0d4b2259231d642ab5a30fcf389ba987b70) )
	ROM_LOAD16_BYTE( "m48z08posz.bin", 0x0001, 0x2000, CRC(7c5a4b78) SHA1(262d0d7f5b24e356ab54eb2450bbaa90e3fb5464) )
ROM_END

ROM_START( fashiong2 )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "fashion_gambler_f3_i.u2", 0x00000, 0x80000, CRC(2939279a) SHA1(75798ea41dd713d294ea341cbcdb73a76d9f63f4) )
	ROM_LOAD16_BYTE( "fashion_gambler_f3_ii.u6", 0x00001, 0x80000, CRC(7d48e9ab) SHA1(603e946b95c53ee75c9ca10751316e723242424f) )

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "fashion_gambler_video_f2_i.u2", 0x00000, 0x80000, CRC(54ea6f10) SHA1(a1284ec34e4e78acba08dc00d5ba47c3457531f8) )
	ROM_LOAD16_BYTE( "fashion_gambler_video_f2_ii.u5", 0x00001, 0x80000, CRC(c292a278) SHA1(9f66531ae9f202d364f47c7ed3551483fc9d27b0) )

	ROM_REGION( 0x4000, "user1", 0 )
	//nvram - 16 bit - taken from parent
	ROM_LOAD16_BYTE( "m48z08post.u5", 0x0000, 0x2000, CRC(2d317a04) SHA1(c690c0d4b2259231d642ab5a30fcf389ba987b70) )
	ROM_LOAD16_BYTE( "m48z08posz.u8", 0x0001, 0x2000, CRC(7c5a4b78) SHA1(262d0d7f5b24e356ab54eb2450bbaa90e3fb5464) )
ROM_END

ROM_START( funlddlx )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "fldl_f6_1.u2", 0x00001, 0x80000, CRC(85c74040) SHA1(24a7d3e6acbaf73ef9817379bef64c38a9ff7896) )
	ROM_LOAD16_BYTE( "fldl_f6_2.u6", 0x00000, 0x80000, CRC(93bf1a4b) SHA1(5b4353feba1e0d4402cd26f4855e3803e6be43b9) )

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "flv_f1_i.u2", 0x00000, 0x80000, CRC(286fccdc) SHA1(dd23deda625e486a7cfe1f3268731d10053a96e9) )
	ROM_LOAD16_BYTE( "flv_f1_ii.u5", 0x00001, 0x80000, CRC(2aa904e6) SHA1(864530b136dd488d619cc95f48e7dce8d93d88e0) )

	ROM_REGION( 0x40000, "nvram", 0 )
	//nvram - 16 bit - taken from parent
	ROM_LOAD16_BYTE( "v62c5181024ll.u3", 0x0000, 0x20000, CRC(66e00617) SHA1(74abbf8fae63f88f9dcbe9c72ff3d2f2fbf9cd87) )
	ROM_LOAD16_BYTE( "v62c5181024ll.u7", 0x0001, 0x20000, CRC(89705c86) SHA1(e5b57ab26a5034349ee61b8821d1ae64e2dd45f4) )
ROM_END

ROM_START( funlddlx2 )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "funny_land_dlx_i_w2.u2", 0x00000, 0x80000, CRC(d51abc1d) SHA1(e9c30efc36cf754fe8aa369c83ead6a8f4b300f4) )
	ROM_LOAD16_BYTE( "funny_land_dlx_ii_w2.u6", 0x00001, 0x80000, CRC(44691005) SHA1(faf88d6e5e67a4f789f5535a1f2eb2eb93d0f9fd) )

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "flv_f1_i.u2", 0x00000, 0x80000, CRC(286fccdc) SHA1(dd23deda625e486a7cfe1f3268731d10053a96e9) )
	ROM_LOAD16_BYTE( "flv_f1_ii.u5", 0x00001, 0x80000, CRC(2aa904e6) SHA1(864530b136dd488d619cc95f48e7dce8d93d88e0) )

	ROM_REGION( 0x40000, "nvram", 0 )
	//nvram - 16 bit - taken from parent
	ROM_LOAD16_BYTE( "v62c5181024ll.u3", 0x0000, 0x20000, CRC(66e00617) SHA1(74abbf8fae63f88f9dcbe9c72ff3d2f2fbf9cd87) )
	ROM_LOAD16_BYTE( "v62c5181024ll.u7", 0x0001, 0x20000, CRC(89705c86) SHA1(e5b57ab26a5034349ee61b8821d1ae64e2dd45f4) )
ROM_END

ROM_START( funlddlx4 )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "fldl_w4_i.u2", 0x00000, 0x80000, CRC(dc64234e) SHA1(4bdcb6b54095307939118cc479aa89db66e02757) )
	ROM_LOAD16_BYTE( "fldl_w4_ii.u6", 0x00001, 0x80000, CRC(fde4caa0) SHA1(0db9e8c16c86d005b2f0957f0a42a947b24890a9) )

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "flv_f1_i.u2", 0x00000, 0x80000, CRC(286fccdc) SHA1(dd23deda625e486a7cfe1f3268731d10053a96e9) )
	ROM_LOAD16_BYTE( "flv_f1_ii.u5", 0x00001, 0x80000, CRC(2aa904e6) SHA1(864530b136dd488d619cc95f48e7dce8d93d88e0) )

	ROM_REGION( 0x40000, "nvram", 0 )
	//nvram - 16 bit - generated by running 0x188dc
	ROM_LOAD16_BYTE( "v62c5181024ll.u3", 0x0000, 0x20000, CRC(66e00617) SHA1(74abbf8fae63f88f9dcbe9c72ff3d2f2fbf9cd87) )
	ROM_LOAD16_BYTE( "v62c5181024ll.u7", 0x0001, 0x20000, CRC(89705c86) SHA1(e5b57ab26a5034349ee61b8821d1ae64e2dd45f4) )
ROM_END

ROM_START( fstation7 )
	ROM_REGION16_BE( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE("spielekoffer_7_sp_f1_i.u2", 0x00000, 0x80000, CRC(bbf4bbd9) SHA1(80e785cb04213f8cc2f580b523e20b4825ba45e5))
	ROM_LOAD16_BYTE("spielekoffer_7_sp_f1_ii.u6", 0x00001, 0x80000, CRC(cd8ab9e3) SHA1(cb9206d0367f00bec278cee0a4115594ba715fcd))

	ROM_REGION16_BE( 0x100000, "gfx1", 0)
	ROM_LOAD16_BYTE("spielekoffer_7_sp_video_f1_i.u2", 0x00000, 0x80000, CRC(dcddb25a) SHA1(7c54bd7a368fd57e3eb995a26462b3d2d589b0db))
	ROM_LOAD16_BYTE("spielekoffer_7_sp_video_f1_ii.u5", 0x00001, 0x80000, CRC(400f9b8f) SHA1(4c4a9f46016eee805653b5fae65680225ac71436))
ROM_END

ROM_START( fstation8 )
	ROM_REGION16_BE( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE("spielekoffer_8_sp_f1_i.u2", 0x00000, 0x80000, CRC(f9c792ab) SHA1(30ab7352cce22340be87ddae80e4b3c2f69ea778))
	ROM_LOAD16_BYTE("spielekoffer_8_sp_f1_ii.u5", 0x00001, 0x80000, CRC(0cb7b719) SHA1(e87bc67da903d9514dd97a6abf2d4e2171e15dbd))

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "spielekoffer_8_sp_video_i.u2", 0x00000, 0x80000, NO_DUMP )
	ROM_LOAD16_BYTE( "spielekoffer_8_sp_video_ii.u5", 0x00001, 0x80000, NO_DUMP )
ROM_END

ROM_START( fstation )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "spielekoffer_9_sp_fun_station_f1.i", 0x00000, 0x80000, CRC(4572efbd) SHA1(e0a91d32ab4096767cafb743523d038f5e0d3238) )
	ROM_LOAD16_BYTE( "spielekoffer_9_sp_fun_station_f1.ii", 0x00001, 0x80000, CRC(a972184d) SHA1(1849e71e696039f07b7b67c4172c7999e81664c3) )

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "spielekoffer_video_9_sp_f1.i", 0x00000, 0x80000, CRC(b6eb971e) SHA1(14e3272c66a82db0f77123974eea28f308209b1b) )
	ROM_LOAD16_BYTE( "spielekoffer_video_9_sp_f1.ii", 0x00001, 0x80000, CRC(64138dcb) SHA1(1b629915cba32f8f6164ae5075c175b522b4a323) )
ROM_END

ROM_START( kkornf4 )
	ROM_REGION16_BE( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "kimme_und_korn_f4_i.u2", 0x00000, 0x20000, CRC(eb9aca01) SHA1(d38dbe7387824a18c7ba8b70691b7313bd604e28) )
	ROM_LOAD16_BYTE( "kimme_und_korn_f4_ii.u6", 0x00001, 0x20000, CRC(84940a33) SHA1(efe167d07199d3200915fd355e30946a09c2ed23) )

	ROM_REGION16_BE( 0x100000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "kimme_und_korn_video_i.u2", 0x00000, 0x80000, NO_DUMP )
	ROM_LOAD16_BYTE( "kimme_und_korn_video_ii.u5", 0x00001, 0x80000, NO_DUMP )

	ROM_REGION( 0x4000, "nvram", 0 )
	ROM_LOAD16_BYTE( "mk48t08_i.u5", 0x0000, 0x2000, CRC(a99a0c21) SHA1(d88452bd00e2dba3568f3ad0b73be4c6048a7654) )
	ROM_LOAD16_BYTE( "mk48t08_ii.u8", 0x0001, 0x2000, CRC(0b5053de) SHA1(ff3d55038322d825d96262ce3f787586bd45fd33) )
ROM_END

ROM_START(trumpfas)
	ROM_REGION16_BE( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "trumpf_as_dm_f2_i.u2", 0x00000, 0x20000, CRC(542b1517) SHA1(fcddb31b4b429c8d67161037d356861413567bb8))
	ROM_LOAD16_BYTE( "trumpf_as_dm_f2_ii.u6", 0x00001, 0x20000, CRC(d39bbd88) SHA1(64f47fd0076845ed3f9f3e84aca3504c110ad8ad))

	ROM_REGION16_BE( 0x40000, "gfx1", 0 )
	ROM_LOAD16_BYTE( "trumpf_as_video_i.u2", 0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "trumpf_as_video_ii.u5", 0x00001, 0x20000, NO_DUMP )
ROM_END

} // Anonymous namespace


GAME( 1993, quickjac,  0,        quickjac, quickjac, adp_state, empty_init, ROT0, "ADP",     "Quick Jack",                        0 )
GAME( 1993, kkornf4,   0,        skattv,   skattv,   adp_state, empty_init, ROT0, "ADP",     "Kimme und Korn (F4)",               MACHINE_NOT_WORKING ) // lightgun missing
GAME( 1994, skattv,    0,        skattv,   skattv,   adp_state, empty_init, ROT0, "ADP",     "Skat TV",                           0 )
GAME( 1994, trumpfas,  skattv,   skattv,   skattv,   adp_state, empty_init, ROT0, "ADP",     "Trumpf As",                         MACHINE_NOT_WORKING ) // throws FOUL error on startup
GAME( 1995, skattva,   skattv,   skattva,  skattva,  adp_state, empty_init, ROT0, "ADP",     "Skat TV (version TS3)",             0 )
GAME( 1997, fashiong,  0,        fashiong, skattv,   adp_state, empty_init, ROT0, "ADP",     "Fashion Gambler (set 1)",           0 )
GAME( 1997, fashiong2, fashiong, fashiong, skattv,   adp_state, empty_init, ROT0, "ADP",     "Fashion Gambler (set 2)",           0 )
GAME( 1998, sbsoli,    0,        fashiong, skattv,   adp_state, empty_init, ROT0, "ADP",     "Skat Bierskat Solitaire (F2)",      0 )
GAME( 1999, funlddlx,  funlddlx4,funland,  skattv,   adp_state, empty_init, ROT0, "Stella",  "Funny Land de Luxe",                MACHINE_NOT_WORKING )
GAME( 2000, fstation7, fstation, fstation, fstation, adp_state, empty_init, ROT0, "ADP",     "Fun Station Spielekoffer 7 Spiele", MACHINE_NOT_WORKING ) // suntris crashes when executing HD63484 paint commands
GAME( 2000, fstation8, fstation, fstation, fstation, adp_state, empty_init, ROT0, "ADP",     "Fun Station Spielekoffer 8 Spiele", MACHINE_NOT_WORKING ) // suntris crashes when executing HD63484 paint commands
GAME( 2000, fstation,  0,        fstation, fstation, adp_state, empty_init, ROT0, "ADP",     "Fun Station Spielekoffer 9 Spiele", MACHINE_NOT_WORKING ) // suntris crashes when executing HD63484 paint commands
GAME( 2001, funlddlx2, funlddlx4,funland,  skattv,   adp_state, empty_init, ROT0, "Stella",  "Funny Land de Luxe (W2 set)",       MACHINE_NOT_WORKING )
GAME( 2001, funlddlx4, 0,        funland,  skattv,   adp_state, empty_init, ROT0, "Stella",  "Funny Land de Luxe (W4 set)",       MACHINE_NOT_WORKING )
