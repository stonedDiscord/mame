// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/*

ADP Minidrucker

CPU: SC80C31BCGN
RAM: LH5164A
      __________
     |  L7805   |
_____|          |_____
| P1      LH5164D    |
|      74HC573/EP    |
| O                  |
|      SC80C31BCGN   |
| 74HC86             |
|  74HC14  L6221  XT |
| 74HC4051     P2    |
|S1 S2    LED        |
|_____________________

O = Optocoupler
XT = Crystal 11059P110
P1 = Serial cable connector
P2 = Print head connector
S1 = On/Off switch
S2 = BCD code switch
EP = DIP-28 EPROM socket

*/

#include "emu.h"

#include "bus/rs232/rs232.h"
#include "cpu/mcs51/i80c51.h"
#include "imagedev/printer.h"

namespace {

class minid_state : public driver_device
{
public:
	minid_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_printer(*this, "printer"),
		m_rs232(*this, "rs232"),
		m_led(*this, "led")
	{ }

	void minid(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	uint8_t port1_r();
	void port1_w(uint8_t data);
	uint8_t port3_r();
	void port3_w(uint8_t data);
	
	void minid_data(address_map &map) ATTR_COLD;
	void minid_map(address_map &map) ATTR_COLD;

	required_device<mcs51_cpu_device> m_maincpu;
	required_device<printer_image_device> m_printer;
	required_device<rs232_port_device> m_rs232;
	output_finder<> m_led;

	uint8_t m_port1 = 0x00;
	uint8_t m_port3 = 0x00;

};

void minid_state::minid_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
}

void minid_state::minid_data(address_map &map)
{
	map(0x0000, 0x1fff).ram();
}

static INPUT_PORTS_START( minid )
	PORT_START("S2")
	PORT_CONFNAME( 0x0f, 0x0, "Datenauswahl" )
	PORT_CONFSETTING(    0x00, "L-G-S" )
	PORT_CONFSETTING(    0x01, "L-D" )
	PORT_CONFSETTING(    0x02, "L-G" )
	PORT_CONFSETTING(    0x03, "I-S" )
	PORT_CONFSETTING(    0x04, "L-D-S" )
	PORT_CONFSETTING(    0x05, "I-G-S" )
	PORT_CONFSETTING(    0x06, "I-D-S" )
	PORT_CONFSETTING(    0x07, "L-D-S-K-Li" )
	PORT_CONFSETTING(    0x08, "L-G-S-K-Li" )
	PORT_CONFSETTING(    0x09, "Werkstest" )

	PORT_DIPNAME( 0x10, 0x10, "S1" ) PORT_DIPLOCATION("S1:1")
	PORT_DIPSETTING(      0x10, "Drucker im Normalbetrieb" )
	PORT_DIPSETTING(      0x0, "Drucker zieht Papier ein" )

INPUT_PORTS_END

void minid_state::machine_start()
{
	save_item(NAME(m_port1));
	save_item(NAME(m_port3));
	m_led.resolve();
}

void minid_state::machine_reset()
{
	m_port1 = 0x00;
	m_port3 = 0x00;
}

uint8_t minid_state::port1_r()
{
	uint8_t data = m_port1;

	return data;
}

void minid_state::port1_w(uint8_t data)
{
	m_port1 = data;
	m_led = BIT(data, 7);
}

uint8_t minid_state::port3_r()
{
	uint8_t data = m_port3;

	// RXD (P3.0)
	data = (data & ~0x01) | (m_rs232->rxd_r() & 0x01);

	return data;
}

void minid_state::port3_w(uint8_t data)
{
	m_port3 = data;

	// TXD (P3.1)
	m_rs232->write_txd(BIT(data, 1) ? 1 : 0);
}

void minid_state::minid(machine_config &config)
{
	I80C31(config, m_maincpu, 11.0592_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &minid_state::minid_map);
	m_maincpu->set_addrmap(AS_DATA, &minid_state::minid_data);

	m_maincpu->port_in_cb<1>().set(FUNC(minid_state::port1_r));
	m_maincpu->port_out_cb<1>().set(FUNC(minid_state::port1_w));
	m_maincpu->port_in_cb<3>().set(FUNC(minid_state::port3_r));
	m_maincpu->port_out_cb<3>().set(FUNC(minid_state::port3_w));

	PRINTER(config, m_printer, 0);

	RS232_PORT(config, m_rs232, default_rs232_devices, nullptr);
}

ROM_START( minid06 )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "mini_drucker_v_0.6.bin", 0x0000, 0x8000, CRC(c478258e) SHA1(d846e20d92254117b2cf652cc90a11de8c9fd6da) )
ROM_END

ROM_START( minid110 )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "mini_drucker_v_1.10.bin", 0x0000, 0x8000, CRC(4b61ee4f) SHA1(e3fd04deb667afedf9b70ac9ce84b185c6240e4a) )
ROM_END

ROM_START( minid112 )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "mini_drucker_v_1.12.bin", 0x0000, 0x8000, CRC(07a3e80f) SHA1(7426cf06cf137ecba4c8044b65faa4a52a5f8484) )
ROM_END

ROM_START( doppd112 )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "mini_drucker_v_1.12_doppeldruck.bin", 0x0000, 0x8000, CRC(5178d065) SHA1(8c8a10795c4dcc0fc653f1a218f4c8eccebb157d) )
ROM_END

} // anonymous namespace

GAME( 1992, minid06,  doppd112, minid, minid, minid_state, empty_init, ROT0, "ADP", "Mini-Drucker (V 0.6)",              MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
GAME( 1992, minid110, doppd112, minid, minid, minid_state, empty_init, ROT0, "ADP", "Mini-Drucker (V 1.10)",             MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
GAME( 1992, minid112, doppd112, minid, minid, minid_state, empty_init, ROT0, "ADP", "Mini-Drucker (V 1.12)",             MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
GAME( 1992, doppd112,        0, minid, minid, minid_state, empty_init, ROT0, "ADP", "Mini-Drucker (V 1.12 Doppeldruck)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
