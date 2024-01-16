// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

Odyssey (c) 1972 Magnavox

TODO: everything

***************************************************************************/

#include "emu.h"

#include "machine/netlist.h"

#include "video/fixfreq.h"

#include "netlist/devices/net_lib.h"

#include "nl_modyssey.h"

#include "screen.h"

#include <cmath>


/*
 * Videos:
 *
 * https://www.youtube.com/watch?v=5f5Ol57HEkg
 * https://www.youtube.com/watch?v=Lc8swrtGer4
 *
 *
 */


namespace {

static const int NS_PER_CLOCK  = static_cast<int>((double) netlist::config::INTERNAL_RES::value / (double) 7159000 + 0.5);
static const int MASTER_CLOCK  = static_cast<int>((double) netlist::config::INTERNAL_RES::value / (double) NS_PER_CLOCK + 0.5);

#define V_TOTAL    (0x105+1)       // 262
#define H_TOTAL    (0x1C5+1)       // 454

enum input_changed_enum
{
	IC_PADDLE1,
	IC_PADDLE2,
	IC_COIN,
	IC_SWITCH,
	IC_VR1,
	IC_VR2
};

class ttl_mono_state : public driver_device
{
public:
	ttl_mono_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_video(*this, "fixfreq")
	{
	}

	// devices
	required_device<netlist_mame_device> m_maincpu;
	required_device<fixedfreq_device> m_video;

private:

};

class modyssey_state : public ttl_mono_state
{
public:
	modyssey_state(const machine_config &mconfig, device_type type, const char *tag)
		: ttl_mono_state(mconfig, type, tag)
	{
	}

	DECLARE_INPUT_CHANGED_MEMBER(input_changed);

	void modyssey(machine_config &config);

private:

};

static INPUT_PORTS_START( modyssey )
	PORT_START( "PADDLE0" ) /* fake input port for player 1 paddle */
	//PORT_BIT( 0x01, 0x10, IPT_PADDLE ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p1hor")
	//PORT_BIT( 0x01, 0x20, IPT_PADDLE ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p1ver")
	//PORT_BIT( 0x01, 0x30, IPT_PADDLE ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p1eng")
	//PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )     NETLIST_LOGIC_PORT_CHANGED("maincpu", "SW4_1")

	PORT_START( "PADDLE1" ) /* fake input port for player 2 paddle */
	//PORT_BIT( 0x01, 0x10, IPT_PADDLE ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p2hor")
	//PORT_BIT( 0x01, 0x20, IPT_PADDLE ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p2ver")
	//PORT_BIT( 0x01, 0x30, IPT_PADDLE ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p2eng")
	//PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )     NETLIST_LOGIC_PORT_CHANGED("maincpu", "SW3_1")

INPUT_PORTS_END

void modyssey_state::modyssey(machine_config &config)
{
	/* basic machine hardware */
	NETLIST_CPU(config, "maincpu", netlist::config::DEFAULT_CLOCK()).set_source(NETLIST_NAME(modyssey));

	//NETLIST_ANALOG_INPUT(config, "maincpu:p1hor", "RV4_61.R").set_mult_offset(1.0 / 100.0 * RES_K(50), RES_K(56) );
	//NETLIST_ANALOG_INPUT(config, "maincpu:p1ver", "RV4_8.R").set_mult_offset(1.0 / 100.0 * RES_K(50), RES_K(56) );

	//NETLIST_LOGIC_INPUT(config, "maincpu:p1reset", "SW4_1.POS", 0);
	//NETLIST_LOGIC_INPUT(config, "maincpu:p2reset", "SW3_1.POS", 0);

	NETLIST_ANALOG_OUTPUT(config, "maincpu:vid0", 0).set_params("videomix", "fixfreq", FUNC(fixedfreq_device::update_composite_monochrome));

	/* video hardware */
	SCREEN(config, "screen", SCREEN_TYPE_RASTER);
	//SCREEN(config, "screen", SCREEN_TYPE_VECTOR);
	FIXFREQ(config, m_video)
		.set_monitor_clock(MASTER_CLOCK)
		.set_horz_params(H_TOTAL-66,H_TOTAL-40,H_TOTAL-8,H_TOTAL)
		.set_vert_params(V_TOTAL-22,V_TOTAL-10,V_TOTAL-8,V_TOTAL)
		.set_fieldcount(1)
		.set_threshold(0.11)
		.set_gain(2.37)
		.set_horz_scale(4)
		.set_screen("screen");
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( modyssey ) /* dummy to satisfy game entry*/
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
ROM_END

} // anonymous namespace

GAME(  1972, modyssey,      0, modyssey,     modyssey,      modyssey_state,     empty_init, ROT0,  "Magnavox", "Odyssey", MACHINE_NOT_WORKING|MACHINE_NO_SOUND_HW)