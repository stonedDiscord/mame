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
- Identify cabinet inputs and the remaining serial bus peripherals.
- Emulate the peripheral on bus address 0xf: CR7A12 sends parity-protected
  serial commands using the data latch line and halts at 0x5911 when no reply
  is received (error 0x0241).
- Verify the sound clock and remaining sound control lines.
- Reel motors, payout hardware and cabinet lamp assignments.
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
		, m_digits(*this, "digit%u", 0U)
		, m_lamps(*this, "lamp%u%u", 0U, 0U)
		, m_led(*this, "led_error")
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
	output_finder<6> m_digits;
	output_finder<22, 8> m_lamps;
	output_finder<> m_led;

	u8 m_control = 0xff;
	u8 m_bus_address = 0;
	u8 m_address_shift = 0;
	u8 m_output_shift[16] = {};
	u16 m_input_shift = 0xffff;

	void control_w(u8 data);
	void sound_w(u8 data);
	u8 status_r();

	void mem_map(address_map &map);
	void io_map(address_map &map);

};

void gse3_state::machine_start()
{
	save_item(NAME(m_control));
	save_item(NAME(m_bus_address));
	save_item(NAME(m_address_shift));
	save_item(NAME(m_output_shift));
	save_item(NAME(m_input_shift));
}

void gse3_state::machine_reset()
{
	m_control = 0xff;
	m_bus_address = 0;
	m_address_shift = 0;
	std::fill(std::begin(m_output_shift), std::end(m_output_shift), 0);
	m_input_shift = 0xffff;
}

u8 gse3_state::status_r()
{
	// Bit 5 is the active-low sound busy signal (polled at 0x2041/0x2054).
	// Bit 7 is the serial bus return line.  Other inputs are not yet identified.
	return (BIT(m_input_shift, 15) << 7) | (m_oki->busy_r() << 5) | 0x5f;
}

void gse3_state::sound_w(u8 data)
{
	// Seven-bit phrase number and active-low ST.  The firmware writes the
	// phrase with bit 7 high, pulses it low, then restores it high.
	m_oki->write(data & 0x7f);
	m_oki->st_w(BIT(data, 7));
}

void gse3_state::control_w(u8 data)
{
	// The firmware sends a four-bit address followed by up to 128 data bits.
	// Data is inverted on bit 3; clock, data latch and address latch are active low.
	if (BIT(m_control, 5) && !BIT(data, 5))
	{
		m_address_shift = ((m_address_shift << 1) | !BIT(data, 3)) & 0x0f;
		for (unsigned i = 0; i < std::size(m_output_shift) - 1; ++i)
			m_output_shift[i] = (m_output_shift[i] << 1) | BIT(m_output_shift[i + 1], 7);
		m_output_shift[15] = (m_output_shift[15] << 1) | !BIT(data, 3);
		m_input_shift = (m_input_shift << 1) | 1;
	}

	if (BIT(m_control, 6) && !BIT(data, 6))
		m_bus_address = m_address_shift;

	if (BIT(m_control, 4) && !BIT(data, 4))
	{
		switch (m_bus_address)
		{
		case 0x8:
			// Six seven-segment digits, sent inverted by the firmware.
			for (unsigned i = 0; i < 6; ++i)
			{
				u8 const segments = m_output_shift[15 - i] ^ 0xff;
				m_digits[i] = bitswap<8>(segments, 3, 1, 5, 4, 2, 7, 6, 0);
			}
			break;
		case 0x6:
		case 0xb:
			// Cabinet assignment is provisional; keep the complete chains visible.
			for (unsigned i = 0; i < ((m_bus_address == 6) ? 16 : 6); ++i)
				for (unsigned bit = 0; bit < 8; ++bit)
					m_lamps[i + ((m_bus_address == 6) ? 0 : 16)][bit] = BIT(m_output_shift[15 - i], bit) ^ (m_bus_address == 6);
			break;
		default:
			m_input_shift = 0xffff;
			break;
		}
	}

	m_led = BIT(data, 7);
	m_control = data;
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
	map(0x81, 0x81).r(FUNC(gse3_state::status_r));
	map(0x82, 0x82).w(FUNC(gse3_state::control_w));
	map(0x84, 0x84).nopw(); // unknown
	map(0x85, 0x85).w(FUNC(gse3_state::sound_w));
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

	rs232_port_device &rs232a(RS232_PORT(config, "rs232a", default_rs232_devices, nullptr));
	rs232a.rxd_handler().set(m_maincpu, FUNC(z180_device::rxa0_w));

	rs232_port_device &rs232b(RS232_PORT(config, "rs232b", default_rs232_devices, nullptr));
	rs232b.rxd_handler().set(m_maincpu, FUNC(z180_device::rxa1_w));

	M48T35(config, m_timekeeper, 0);

	OKIM6376(config, m_oki, 128'000).add_route(ALL_OUTPUTS, "mono", 1.0); // provisional 16 kHz sample rate, clock unverified

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

ROM_START( cr7apl84 )
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

ROM_START( crclssic )
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

ROM_START( crdpwers )
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

ROM_START( crmaxa2 )
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

ROM_START( crpneer )
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

ROM_START( crqttro )
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

GAMEL( 2002,   cr1000, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown 1000",               MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2000,   cr7a12, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown 7A (12Sek)",         MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2000,   cr7a15, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown 7A (15Sek)",         MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2002, cr7aplus, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown 7A Plus",            MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2002, cr7apl84, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown 7A Plus (b)",        MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2003,  cr7aroy, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown 7A Royal",           MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2005, crclssic, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Classic",            MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )	
GAMEL( 2003,  crcobra, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Cobra",              MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2001, crdpower, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Double Power",       MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2006, crdpwers, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Double Power Super", MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2003, creldora, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Eldorado",           MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2005,     crla, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown L.A.",               MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2004,     crmm, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown MM",                 MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2004,  crmamba, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Mamba",              MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2000,  crmaxa2, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Maxi Action II",     MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )	
GAMEL( 2006,   crmumm, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Mumm",               MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 200?,  crpneer, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Pioneer",            MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2003,   crpact, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Power Action",	      MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2001,   crpmax, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Power Maxi",         MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2003,  crpacta, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Power Action (b)",   MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2005,  crqttro, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Quattro",	          MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2004,  crroyal, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Royal",              MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2005,     crtt, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown TT",                 MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2004,   crtopt, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Top Ten",            MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )
GAMEL( 2003,  crviper, 0, gse3, gse3, gse3_state, empty_init, ROT0, "Bergmann Automaten", "Crown Viper",              MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_cr7a )	
