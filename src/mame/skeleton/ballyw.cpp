// license:BSD-3-Clause
/*

Bally Wulff
German Fruit Machines / Gambling Machines

*/


#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/nvram.h"
#include "machine/msm6242.h"
#include "speaker.h"

#include "stellafr.lh"

namespace {

class ballyw_state : public driver_device
{
public:
	ballyw_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
        m_rtc(*this, "rtc"),
		m_digits(*this, "digit%u", 0U)
	{ }

	void ballyw(machine_config &config);

private:
	void mem_map(address_map &map) ATTR_COLD;

	// devices
	required_device<cpu_device> m_maincpu;
    required_device<rtc72421_device> m_rtc;
	output_finder<8> m_digits;
};


void ballyw_state::mem_map(address_map &map)
{
	map(0x000000, 0x0fffff).rom();
}

static INPUT_PORTS_START( ballyw )
	PORT_START("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_GAMBLE_HIGH ) // Left
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_START )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_GAMBLE_BET )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SLOT_STOP1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_GAMBLE_LOW ) // Right
INPUT_PORTS_END


void ballyw_state::ballyw(machine_config &config)
{
	M68000(config, m_maincpu, 16_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &ballyw_state::mem_map);

    RTC72421(config, "rtc", XTAL(32'768)); // internal oscillator
}

ROM_START( gloriasl )
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "123-S4.0_even_GLORIA_SL.ic10", 0x00001, 0x80000, CRC(1be58a11) SHA1(1a1b1f51c9c3f4bd16832a689ee90a5f8faac453) )
	ROM_LOAD16_BYTE( "123-S4.0_odd_GLORIA_SL.ic15", 0x00000, 0x80000, CRC(06d7ee41) SHA1(8bf4b1ce16d9f021381b7dae5069ba8383b150cd) )
ROM_END

ROM_START( sunfun )
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "sunfun_even.ic10", 0x00001, 0x80000, CRC(f03bdbef) SHA1(9dc08b684e03b489953c1867902b2164cd2a4f8d) )
	ROM_LOAD16_BYTE( "sunfun_odd.ic15", 0x00000, 0x80000, CRC(5b52038a) SHA1(9dc08b684e03b489953c1867902b2164cd2a4f8d) )
ROM_END

} // anonymous namespace

GAMEL(2003, gloriasl,        0, ballyw, ballyw, ballyw_state, empty_init, ROT0, "Bally Wulff", "Gloria SL",  MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_stellafr )
GAMEL(2003, sunfun,   gloriasl, ballyw, ballyw, ballyw_state, empty_init, ROT0, "Bally Wulff", "Sun Fun",    MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_stellafr )
