// license:BSD-3-Clause
// copyright-holders:stonedDiscord

/*
Tech used by Bergmann Automaten GmbH from 2000 on
GSE stands for Gerätesteuereinheit (device control unit)

Main board
CPU Z8S18020VSC
RAM CY62256LL

Game module:
NVRAM M48T35

TODO:
everything
*/

#include "emu.h"

#include "bus/rs232/rs232.h"
#include "cpu/z180/z180.h"
#include "machine/timekpr.h"
#include "sound/okim6376.h"

#include "speaker.h"

#include "cr7a.lh"

//#define VERBOSE 1
#include "logmacro.h"

namespace {

class gse3_state : public driver_device
{
public:
	gse3_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_timekeeper(*this, "m48t35")
		, m_oki(*this, "oki")
	{
	}

	void gse3(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	required_device<z8s180_device> m_maincpu;
	required_device<m48t35_device> m_timekeeper;
	required_device<okim6376_device> m_oki;

	void mem_map(address_map &map);
	void io_map(address_map &map);

};

void gse3_state::machine_start()
{

}

void gse3_state::machine_reset()
{

}

void gse3_state::mem_map(address_map &map)
{
	map(0x00000, 0x0ffff).rom().region("maincpu", 0);
	map(0x80000, 0x87fff).ram();
	map(0xc0000, 0xc7fff).rw("m48t35", FUNC(m48t35_device::read), FUNC(m48t35_device::write));
}

void gse3_state::io_map(address_map &map)
{
	map.global_mask(0xff);
	map.unmap_value_high();
	map(0x80, 0x80).portr("DSW2");
	map(0x84, 0x84).nopw(); // unknown
}

static INPUT_PORTS_START( gse3 )
	PORT_START("DSW2")
	PORT_DIPNAME(0x01, 0x01, "Currency") PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(   0x01, "Euro")
	PORT_DIPSETTING(   0x00, "DM")
	PORT_BIT(0xfe, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

void gse3_state::gse3(machine_config &config)
{
	Z8S180(config, m_maincpu, 4_MHz_XTAL/2);
	m_maincpu->set_addrmap(AS_PROGRAM, &gse3_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &gse3_state::io_map);
	m_maincpu->txa0_wr_callback().set("rs232a", FUNC(rs232_port_device::write_txd));
	m_maincpu->txa1_wr_callback().set("rs232b", FUNC(rs232_port_device::write_txd));

	rs232_port_device &rs232a(RS232_PORT(config, "rs232a", default_rs232_devices, "terminal"));
	rs232a.rxd_handler().set(m_maincpu, FUNC(z180_device::rxa0_w));

	rs232_port_device &rs232b(RS232_PORT(config, "rs232b", default_rs232_devices, nullptr));
	rs232b.rxd_handler().set(m_maincpu, FUNC(z180_device::rxa1_w));

	M48T35(config, m_timekeeper, 0);

	OKIM6376(config, m_oki, 4_MHz_XTAL).add_route(ALL_OUTPUTS, "mono", 1.0);

	SPEAKER(config, "mono").front_center();
}

ROM_START( cr1000 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "M27C512.ic4", 0x00000, 0x10000, CRC(6c900adf) SHA1(89b3a2b28a479d5a1e55adeba78701cf573eca6b) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( cr7a12 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "v1.1_01.01.02_7a_spiel_27c512.ic4", 0x00000, 0x10000, CRC(5189a613) SHA1(a290917c026e4dafdf482fcc92d75c298c6baadc) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "v_1.0_sound.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( cr7a15 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ic4 M27C512.bin", 0x00000, 0x10000, CRC(5189a613) SHA1(a290917c026e4dafdf482fcc92d75c298c6baadc) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "v_1.0_sound.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )

	ROM_REGION( 0x2000, "nvram", ROMREGION_ERASEFF )
	ROM_LOAD( "M48T18 Roh.bin", 0x00000, 0x01ff8, CRC(a66c719b) SHA1(ca325c8082ef84441bd54897a3ce8f4bc9bf6105) )
ROM_END

ROM_START( cr7aplus )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001 V2.14.ic4", 0x00000, 0x40000, CRC(51be6995) SHA1(dffdeb28cbc4b7e1f865a6dfd01adcfa5df26904) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( cr7aplus84 )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "Crown 7 A Plus Ablauf 01.2084", 0x00000, 0x20000, CRC(1dfa8096) SHA1(f9fa772c3b9624ab19eefc51f5b04af569beecdb) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( cr7aroy )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "AM27C512.ic4", 0x00000, 0x10000, CRC(82630d68) SHA1(118eeafa34a12f53ce2e233115c18703b0a85c8a) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crclassic )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001 V1.20.ic4", 0x00000, 0x40000, CRC(ae31d847) SHA1(dc9ced0f8b4ec6eaf57d6e9e34cc596f22c5b19a) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "NM27C040 V1.7.ic5", 0x00000, 0x80000, CRC(eb601947) SHA1(4c5d6519fd20adb273108b747315ac3beb3bad31) )
ROM_END

ROM_START( crcobra )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001 V 2.20-16.03.04.ic4", 0x00000, 0x40000, CRC(c6f3bff5) SHA1(f728523de4d94489d89ad4a66daf0e389cdb05ea) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crdpower )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "FM27C512.ic4 V1.0", 0x00000, 0x10000, CRC(b3581f33) SHA1(eee0e37a0979413be8868dfe60bdabdb00778f0c) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "NM27C040 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crdpowers )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001 V1.30 20.02.06.ic4", 0x00000, 0x40000, CRC(df9e3206) SHA1(6ceb011291d127babf54c41a8d720460e2433557) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "NM27C040 V1.6.ic5", 0x00000, 0x80000, CRC(7e1bcf1b) SHA1(5b510298f683e3b95cfe5658417f4c2d9f8a53bf) )
ROM_END

ROM_START( creldora )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001 V1.1-09.04.03.ic4", 0x00000, 0x40000, CRC(8659d47e) SHA1(0cda309cb51c95a7cfcdbf1e0556d6db61e75232) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crla )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(ff720a1d) SHA1(943abf8081aeb73503cac9f869434b4ee1f12050) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001.ic5", 0x00000, 0x80000, CRC(c1bcb2ef) SHA1(e1297ddd93fad2a7b2a97725fa866d6de2072afa) )
ROM_END

ROM_START( crmm )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(84917a08) SHA1(743225a624a4ab6e5b43a2ac20f1ae91ed4348b7) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "NM27C040 V1.6.ic5", 0x00000, 0x80000, CRC(7e1bcf1b) SHA1(5b510298f683e3b95cfe5658417f4c2d9f8a53bf) )
ROM_END

ROM_START( crmamba )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(26494191) SHA1(57525f67aa9e2e41d0514066656d6c3e79eda417) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crmaxact2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "M27C512.ic4", 0x00000, 0x10000, CRC(9a9ea176) SHA1(62d28ee2b570d8ec84e3dac73460b8807ee5084e) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crmumm )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(67722a44) SHA1(2092f42ba5192f5331e1fcc7ceca9b30fd9e2d51) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001.ic5", 0x00000, 0x80000, CRC(c1bcb2ef) SHA1(e1297ddd93fad2a7b2a97725fa866d6de2072afa) )
ROM_END

ROM_START( crpioneer )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(7570eaaf) SHA1(5e890bcf58695b6a487913817345aa51e5562d88) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crpact )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001 V1.30.ic4", 0x00000, 0x40000, CRC(df616540) SHA1(6b96633e830d1ec6080aa1a48f1196ce171f39f8) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crpmax )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "FM27C512.ic4", 0x00000, 0x10000, CRC(9a9ea176) SHA1(62d28ee2b570d8ec84e3dac73460b8807ee5084e) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "NM27C040 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crpacta )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001 V1.30-23.09.03 Spieleprogramm", 0x00000, 0x40000, CRC(df616540) SHA1(6b96633e830d1ec6080aa1a48f1196ce171f39f8) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crquattro )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001 V1.20-24.03.05.ic4", 0x00000, 0x40000, CRC(85e9179d) SHA1(7cc9ef140931f72c63f5572ba77c468061b65e2e) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "FM27C040.ic5", 0x00000, 0x80000, CRC(7e1bcf1b) SHA1(5b510298f683e3b95cfe5658417f4c2d9f8a53bf) )
ROM_END

ROM_START( crroyal )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(b1fa3a07) SHA1(117b78d65386976b9856514a8148ca95563bd98b) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crtt )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(67a29e9c) SHA1(f966851b8593320a6763186be93c2214ce1cb177) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(c1bcb2ef) SHA1(e1297ddd93fad2a7b2a97725fa866d6de2072afa) )
ROM_END

ROM_START( crtopt )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(89c20df1) SHA1(451087876a78c0d41846987387a121cdbda289a4) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

ROM_START( crviper )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "M27C2001.ic4", 0x00000, 0x40000, CRC(4416b848) SHA1(09ab4ab9d9944ac44bf5adcca218fc9fea77d005) )

	ROM_REGION( 0x80000, "oki", 0 )
	ROM_LOAD( "M27C4001 V1.0.ic5", 0x00000, 0x80000, CRC(139dada6) SHA1(1885cf3fc76805e942ce8644f4d0d42d7302c98e) )
ROM_END

} // anonymous namespace

GAMEL( 2000,   cr7a12,  0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown 7A",    MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
