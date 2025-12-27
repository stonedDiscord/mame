// license:BSD-3-Clause
/*

Bally Wulff Eurotec
German Fruit Machines / Gambling Machines

The predecessor is the Technik 2000 system.

The CPU board has the CPU, RTC, RAM and ROM

CPU: Motorola MC68EC000
RAM: Samsung K6T4016C3C 256Kx16 bit Low Power CMOS Static RAM
RTC: Epson RTC72421

It sits on a main board that has the OKI sound and a PLCC84 labeled

ATMEL/TEMIC MBZ
ULC 1240 V2.0
5/98
Bally Wulff
YYWW

which seems to be an Ultimate Logic Conversion from an FPGA

older boards are labelled
002.600.002       002.600.11B2

newer with OKI sound
0B01.0600.1100B4

TODO:
there's a watchdog on the main board

*/


#include "emu.h"

#include "cpu/m68000/m68000.h"
#include "machine/nvram.h"
//#include "machine/ds2430a.h" //DS1985 die under an epoxy blob
#include "machine/msm6242.h"
#include "video/roc10937.h"
#include "sound/okim6376.h"

#include "speaker.h"

#include "proconn.lh"

namespace {

class ballyw_state : public driver_device
{
public:
	ballyw_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_rtc(*this, "rtc"),
		m_vfd(*this, "vfd")
	{ }

	void b2(machine_config &config);
	void b4(machine_config &config);

private:
	virtual void machine_start() override;
	void mem_map(address_map &map) ATTR_COLD;

	void vfd_w(uint8_t data);
	INTERRUPT_GEN_MEMBER(watchdog_interrupt);
	
	void watchdog_clear(int);
	emu_timer *m_watchdog_clear_timer = nullptr;
	
	// devices
	required_device<cpu_device> m_maincpu;
	required_device<rtc72421_device> m_rtc;
	optional_device<rocvfd_device> m_vfd;
};

void ballyw_state::vfd_w(uint8_t data)
{
	m_vfd->write_char(data);
}

void ballyw_state::mem_map(address_map &map)
{
	map(0x000000, 0x07ffff).rom();
	map(0x080000, 0x0fffff).ram();
	map(0x100000, 0x1fffff).ram();
	map(0x1006ac, 0x1006ac).w(FUNC(ballyw_state::vfd_w));
	map(0x800000, 0x8007ff).rw("rtc", FUNC(rtc72421_device::read), FUNC(rtc72421_device::write));
	//map(0x800000, 0x8007ff).rom().region("ident",0); //?
	map(0x900000, 0x9002ff).ram(); //ulc?
	map(0x90f000, 0x90ffff).ram();
}

void ballyw_state::machine_start()
{
	m_watchdog_clear_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(ballyw_state::watchdog_clear), this));
}

static INPUT_PORTS_START( ballyw )
	PORT_START("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_GAMBLE_HIGH ) // Left
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_START )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_GAMBLE_BET )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SLOT_STOP1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_GAMBLE_LOW ) // Right
INPUT_PORTS_END


void ballyw_state::b2(machine_config &config)
{
	M68000(config, m_maincpu, 16_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &ballyw_state::mem_map);

	RTC72421(config, "rtc", XTAL(32'768)); // internal oscillator

	MSC1937(config, m_vfd);

	SPEAKER(config, "mono").front_center();

	config.set_default_layout(layout_proconn);
	
	// Set up watchdog timer to generate periodic interrupts
	// This prevents the firmware from getting stuck in busy-wait loops
	m_maincpu->set_periodic_int(FUNC(ballyw_state::watchdog_interrupt), attotime::from_hz(100));
}

INTERRUPT_GEN_MEMBER(ballyw_state::watchdog_interrupt)
{
	device.execute().set_input_line(M68K_IRQ_1, ASSERT_LINE);
	
	m_watchdog_clear_timer->adjust(attotime::from_usec(10));
}

void ballyw_state::watchdog_clear(int)
{
	m_maincpu->set_input_line(M68K_IRQ_1, CLEAR_LINE);
}

void ballyw_state::b4(machine_config &config)
{
	b2(config);

	OKIM6376(config, "snd", 4_MHz_XTAL).add_route(ALL_OUTPUTS, "mono", 1.0);
}

ROM_START( gbsky )
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "101-s6.0_even_sky.ic10", 0x00001, 0x80000, CRC(8bf3fd6d) SHA1(c95a3651e025e9d2c99c708e643fe3a2982a39ad) )
	ROM_LOAD16_BYTE( "101-s6.0_odd_sky.ic15", 0x00000, 0x80000, CRC(0ce4f9d7) SHA1(d4cea08466cf86de7c27ffdfead456f796e4a0af) )
ROM_END

ROM_START( gloriasl )
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "123-s4.0_even_gloria_sl.ic10", 0x00001, 0x80000, CRC(1be58a11) SHA1(1a1b1f51c9c3f4bd16832a689ee90a5f8faac453) )
	ROM_LOAD16_BYTE( "123-s4.0_odd_gloria_sl.ic15", 0x00000, 0x80000, CRC(06d7ee41) SHA1(8bf4b1ce16d9f021381b7dae5069ba8383b150cd) )

	ROM_REGION( 0x100000, "snd", 0 )
	ROM_LOAD( "123-sound_1.4_gloria_sl.ic13", 0x00000, 0x100000, CRC(71cfbd7e) SHA1(efe2e92cfb7de4b6145aa3462fda010282b31090) )

	ROM_REGION( 0x800, "ident", 0 )
	ROM_LOAD( "127401373.p15", 0x000, 0x800, CRC(215275b6) SHA1(297dd49d42122e2f5f131d610d6a00b42beee677) )
ROM_END

ROM_START( harlekin )
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD( "harlekin_patched.bin", 0x00000, 0x80000, BAD_DUMP CRC(d4b96450) SHA1(c3477ad9fd64e39b5402995cbc757d3ad0dffed6) )

	ROM_REGION( 0x100000, "snd", 0 )
	ROM_LOAD( "sound_harlekin.ic13", 0x00000, 0x100000, NO_DUMP )
ROM_END

ROM_START( sunfun )
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "sunfun_even.ic10", 0x00001, 0x80000, CRC(5b52038a) SHA1(9dc08b684e03b489953c1867902b2164cd2a4f8d) )
	ROM_LOAD16_BYTE( "sunfun_odd.ic15", 0x00000, 0x80000, CRC(8dfdf298) SHA1(986b7e8d118f5edeaa1c930c3e7de77d3922a088) )

	ROM_REGION( 0x100000, "snd", 0 )
	ROM_LOAD( "sound_sunfun.ic13", 0x00000, 0x100000, NO_DUMP )
ROM_END

} // anonymous namespace

GAMEL(1999, harlekin, 0, b2, ballyw, ballyw_state, empty_init, ROT0, "Bally Wulff", "Harlekin",   MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_proconn )
GAMEL(2002, gbsky,    0, b2, ballyw, ballyw_state, empty_init, ROT0, "Bally Wulff", "Sky",        MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_proconn )
GAMEL(2003, gloriasl, 0, b4, ballyw, ballyw_state, empty_init, ROT0, "Bally Wulff", "Gloria SL",  MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_proconn )
GAMEL(2003, sunfun,   0, b4, ballyw, ballyw_state, empty_init, ROT0, "Bally Wulff", "Sun Fun",    MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_proconn )
