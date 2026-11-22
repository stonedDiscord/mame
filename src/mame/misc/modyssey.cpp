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

enum input_changed_enum
{
	IC_PADDLE0H,
	IC_PADDLE0V,
	IC_PADDLE0E,
	IC_PADDLE1H,
	IC_PADDLE1V,
	IC_PADDLE1E,
	IC_PADDLE0R,
	IC_PADDLE1R,
	IC_POT_3,
	IC_POT_4
};

class nl_mono_state : public driver_device
{
public:
	nl_mono_state(const machine_config &mconfig, device_type type, const char *tag)
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

class modyssey_card_device : public device_t
{
public:
	modyssey_card_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	void set_netlist(void (*netlist)(netlist::nlparse_t &)) { m_netlist = netlist; }
	void connect(netlist::nlparse_t &setup) const { m_netlist(setup); }

protected:
	virtual void device_start() override { }

private:
	void (*m_netlist)(netlist::nlparse_t &) = nullptr;
};

DEFINE_DEVICE_TYPE_PRIVATE(ODYSSEY_CARD, device_t, modyssey_card_device, "odyssey_card", "Odyssey Game Card")

modyssey_card_device::modyssey_card_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, ODYSSEY_CARD, tag, owner, clock)
{
}

class modyssey_card_slot_device : public device_t, public device_single_card_slot_interface<modyssey_card_device>
{
public:
	modyssey_card_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	virtual void device_start() override { }
};

DEFINE_DEVICE_TYPE_PRIVATE(ODYSSEY_CARD_SLOT, modyssey_card_slot_device, modyssey_card_slot_device, "odyssey_card_slot", "Odyssey Game Card Slot")

modyssey_card_slot_device::modyssey_card_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, ODYSSEY_CARD_SLOT, tag, owner, clock)
	, device_single_card_slot_interface<modyssey_card_device>(mconfig, *this)
{
	auto const add_card = [this](const char *name, void (*netlist)(netlist::nlparse_t &))
	{
		option_add(name, ODYSSEY_CARD);
		set_option_machine_config(name, [netlist](device_t *device)
		{
			downcast<modyssey_card_device &>(*device).set_netlist(netlist);
		});
	};

	add_card("card1", NETLIST_NAME(modyssey_card1));
	add_card("card2", NETLIST_NAME(modyssey_card2));
	add_card("card3", NETLIST_NAME(modyssey_card3));
	add_card("card4", NETLIST_NAME(modyssey_card4));
	add_card("card5", NETLIST_NAME(modyssey_card5));
	add_card("card6", NETLIST_NAME(modyssey_card6));
	add_card("card7", NETLIST_NAME(modyssey_card7));
	add_card("card8", NETLIST_NAME(modyssey_card8));
	add_card("card9", NETLIST_NAME(modyssey_card9));
	add_card("card10", NETLIST_NAME(modyssey_card10));
	add_card("card11", NETLIST_NAME(modyssey_card11));
	add_card("card12", NETLIST_NAME(modyssey_card12));
	set_default_option("card1");
}

class modyssey_state : public nl_mono_state
{
public:
	modyssey_state(const machine_config &mconfig, device_type type, const char *tag)
		: nl_mono_state(mconfig, type, tag)
		, m_card_slot(*this, "gamecard")
	{
	}

	DECLARE_INPUT_CHANGED_MEMBER(reset_changed);

	void modyssey(machine_config &config);

private:
	void setup_netlist(netlist::nlparse_t &setup);

	required_device<modyssey_card_slot_device> m_card_slot;
};

static INPUT_PORTS_START( modyssey )
	PORT_START( "PADDLE0H" ) /* fake input port for player 1 paddle */
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_H ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p1hor")
	PORT_START( "PADDLE0V" )
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_V ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p1ver")
	PORT_START( "PADDLE0E" )
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0)   NETLIST_ANALOG_PORT_CHANGED("maincpu", "p1eng")
	PORT_START( "PADDLE0R" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )     PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(modyssey_state::reset_changed), 1)

	PORT_START( "PADDLE1H" ) /* fake input port for player 2 paddle */
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_H ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0) PORT_PLAYER(2) NETLIST_ANALOG_PORT_CHANGED("maincpu", "p2hor")
	PORT_START( "PADDLE1V" )
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_V ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0) PORT_PLAYER(2)  NETLIST_ANALOG_PORT_CHANGED("maincpu", "p2ver")
	PORT_START( "PADDLE1E" )
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(2) PORT_KEYDELTA(100) PORT_CENTERDELTA(0) PORT_PLAYER(2)  NETLIST_ANALOG_PORT_CHANGED("maincpu", "p2eng")
	PORT_START( "PADDLE1R" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )     PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(modyssey_state::reset_changed), 2)

	PORT_START("POT_3")
	PORT_ADJUSTER( 50, "Pot: Ball Speed" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "ballspeed")
	PORT_START("POT_4")
	PORT_ADJUSTER( 50, "Pot: Ball Height" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "ballheight")
	PORT_START("POT_6")
	PORT_ADJUSTER( 50, "Pot: Ball Width" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "ballwidth")
	PORT_START("POT_12")
	PORT_ADJUSTER( 50, "Pot: Wall Center Adjustment" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "wallcentadj")
	PORT_START("POT_17")
	PORT_ADJUSTER( 50, "Pot: Wall Width" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "wallwidth")
	PORT_START("POT_26")
	PORT_ADJUSTER( 50, "Pot: Player No.1 Width" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "p1width")
	PORT_START("POT_28")
	PORT_ADJUSTER( 50, "Pot: Player No.1 Height" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "p1height")
	PORT_START("POT_31")
	PORT_ADJUSTER( 50, "Pot: Player No.2 Width" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "p2width")
	PORT_START("POT_32")
	PORT_ADJUSTER( 50, "Pot: Player No.2 Height" )  NETLIST_ANALOG_PORT_CHANGED("maincpu", "p2height")

INPUT_PORTS_END

INPUT_CHANGED_MEMBER(modyssey_state::reset_changed)
{
	const char *const reset = param == 1 ? "maincpu:p1reset" : "maincpu:p2reset";
	const char *const orange = param == 1 ? "maincpu:p1reset_orange" : "maincpu:p2reset_orange";
	const char *const yellow = param == 1 ? "maincpu:p1reset_yellow" : "maincpu:p2reset_yellow";
	subdevice<netlist_mame_logic_input_device>(reset)->write_line(newval);
	subdevice<netlist_mame_logic_input_device>(orange)->write_line(newval);
	subdevice<netlist_mame_logic_input_device>(yellow)->write_line(newval);
}

void modyssey_state::setup_netlist(netlist::nlparse_t &setup)
{
	NETLIST_NAME(modyssey)(setup);
	if (const modyssey_card_device *const card = m_card_slot->get_card_device())
		card->connect(setup);
	else
		PARAM(V5.IN, 0)
}

void modyssey_state::modyssey(machine_config &config)
{
	/* basic machine hardware */
	ODYSSEY_CARD_SLOT(config, m_card_slot, 0);
	NETLIST_CPU(config, "maincpu", netlist::config::DEFAULT_CLOCK()).set_source(this, &modyssey_state::setup_netlist);

	NETLIST_ANALOG_INPUT(config, "maincpu:p1hor", "RV4_61.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:p1ver", "RV4_8.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:p1eng", "RV4_62.DIAL");

	NETLIST_ANALOG_INPUT(config, "maincpu:p2hor", "RV3_61.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:p2ver", "RV3_8.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:p2eng", "RV3_62.DIAL");

	NETLIST_LOGIC_INPUT(config, "maincpu:p1reset", "SW4_1.POS", 0);
	NETLIST_LOGIC_INPUT(config, "maincpu:p1reset_orange", "SW4_2.POS", 0);
	NETLIST_LOGIC_INPUT(config, "maincpu:p1reset_yellow", "SW4_3.POS", 0);
	NETLIST_LOGIC_INPUT(config, "maincpu:p2reset", "SW3_1.POS", 0);
	NETLIST_LOGIC_INPUT(config, "maincpu:p2reset_orange", "SW3_2.POS", 0);
	NETLIST_LOGIC_INPUT(config, "maincpu:p2reset_yellow", "SW3_3.POS", 0);

	NETLIST_ANALOG_INPUT(config, "maincpu:ballspeed", "RV3.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:ballheight", "RV4.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:ballwidth", "RV6.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:wallcentadj", "RV12.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:wallwidth", "RV17.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:p1width", "RV26.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:p1height", "RV28.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:p2width", "RV31.DIAL");
	NETLIST_ANALOG_INPUT(config, "maincpu:p2height", "RV32.DIAL");

	NETLIST_ANALOG_OUTPUT(config, "maincpu:vid0", 0).set_params("videomix", "fixfreq", FUNC(fixedfreq_device::update_composite_monochrome));

	/* video hardware */
	SCREEN(config, "screen");
	FIXFREQ(config, m_video)
		.set_mode_ntsc704()
		.set_fieldcount(1)
		.set_vert_params(240, 244, 249, 262)
		.set_threshold(1.2)
		.set_vsync_threshold(0.5)
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