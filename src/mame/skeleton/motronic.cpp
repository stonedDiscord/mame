// license:GPL-2.0+
// copyright-holders:stonedDiscord
/*************************************************************************

    Bosch Motronic

    The markings are all customized

    SIEMENS
    B57828

    is actually SAB 80515

    NEC IRELAND
    B57727

    is actually UPD4464

**************************************************************************

    TO-DO:

    * Everything

**************************************************************************/

#include "emu.h"

#include "cpu/mcs51/sab80c535.h"

#include "bmw.lh"

namespace {

class motronic_state : public driver_device
{
public:
	motronic_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_lamps(*this, "lamp%u", 0U)
	{ }

	void port0_w(uint8_t data);
    void port1_w(uint8_t data);
	void port2_w(uint8_t data);
    void port3_w(uint8_t data);
	uint8_t port2_r();
	void ml32(machine_config &config);
private:
	virtual void machine_start() override { m_lamps.resolve(); }

    void prog_map(address_map &map) ATTR_COLD;

	required_device<sab80c535_device> m_maincpu;
	output_finder<16> m_lamps;
};

/************************
*      Input Ports      *
************************/

static INPUT_PORTS_START( bmw )
	PORT_START("PORT1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

void motronic_state::port0_w(uint8_t data)
{
	// unk
}

void motronic_state::port1_w(uint8_t data)
{
	// unk
}

void motronic_state::port2_w(uint8_t data)
{
	// unk
}

void motronic_state::port3_w(uint8_t data)
{
	// unk
}

uint8_t motronic_state::port2_r()
{
	// unk
    return 0xff;
}

void motronic_state::prog_map(address_map &map)
{
    //map(0x0000, 0x1fff).ram(); // 8192 bytes
	map(0x0000, 0x7fff).rom().region("program", 0);

}

/************************
*    Machine Drivers    *
************************/

void motronic_state::ml32(machine_config &config)
{
	/* basic machine hardware */
	SAB80C535(config, m_maincpu, XTAL(12'000'000)); // wrong clock
    m_maincpu->set_addrmap(AS_PROGRAM, &motronic_state::prog_map);

	m_maincpu->port_out_cb<0>().set(FUNC(motronic_state::port0_w));
    m_maincpu->port_out_cb<1>().set(FUNC(motronic_state::port1_w));
	m_maincpu->port_in_cb<1>().set_ioport("PORT1");
	m_maincpu->port_in_cb<2>().set(FUNC(motronic_state::port2_r));
	m_maincpu->port_out_cb<2>().set(FUNC(motronic_state::port2_w));
    m_maincpu->port_out_cb<3>().set(FUNC(motronic_state::port3_w));

	config.set_default_layout(layout_bmw);

}

/*************************
*        Rom Load        *
*************************/

ROM_START( bmw )
	ROM_REGION( 0x8000, "program", 0 )
	ROM_LOAD( "1267356334.std", 0x0000, 0x8000, CRC(0dd39d95) SHA1(53cd9edbf7eea9e39c9639dfeaa6f861851b573d) )
ROM_END

} // anonymous namespace


GAME( 1982, bmw, 0, ml32, bmw, motronic_state, empty_init, ROT0, "BMW", "535i E34", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
