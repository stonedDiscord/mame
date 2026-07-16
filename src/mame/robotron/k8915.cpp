// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic, Robbbert
/***************************************************************************

Robotron K8915

2010-08-30

Platinenbestückung
K-Name 	Platine 	Kürzel 	Bedeutung des Kürzels 	Erläuterung
K6001	062-8500	ASL		Adapter für Schreibleseeinheit	Controller für Magnetkartenschreiber
?		045-8762	ZRE		Zentrale Recheneinheit	CPU und 256k RAM
K6022	012-7090	ADA		Adapter für Datenaustausch	SIF1000-Interface
?		045-8732	?		?	für Tastatur, Drucker und Fernleitung
K7024	012-6820	ABS		Adapter für Bildschirm	Grafikkarte

When it says DIAGNOSTIC RAZ P, press enter.

****************************************************************************/

#include "emu.h"

#include "bus/ddr/k1520.h"

#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "bus/rs232/rs232.h"


namespace {

class k8915_state : public driver_device
{
public:
	k8915_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_k1520(*this, "k1520")
		, m_rom(*this, "maincpu")
		, m_ram(*this, "mainram")
		, m_bank1(*this, "bank1")
	{ }

	void k8915(machine_config &config);

private:
	void k8915_a8_w(u8 data);
	u8 abs_r(offs_t offset);
	void abs_w(offs_t offset, u8 data);

	void io_map(address_map &map) ATTR_COLD;
	void mem_map(address_map &map) ATTR_COLD;

	void machine_start() override ATTR_COLD;
	void machine_reset() override ATTR_COLD;
	required_device<cpu_device> m_maincpu;
	required_device<k1520_bus_device> m_k1520;
	required_region_ptr<u8> m_rom;
	required_shared_ptr<u8> m_ram;
	required_memory_bank    m_bank1;
};


void k8915_state::k8915_a8_w(u8 data)
{
// seems to switch ram and rom around.
	m_bank1->set_entry((data == 0x87) ? 0 : 1);
}

void k8915_state::mem_map(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x0fff).ram().share("mainram").bankr("bank1");
	map(0x1000, 0x1fff).rw(FUNC(k8915_state::abs_r), FUNC(k8915_state::abs_w));
	map(0x2000, 0xffff).ram();
}

u8 k8915_state::abs_r(offs_t offset)
{
	return m_k1520->memory_r(0x1000 + offset);
}

void k8915_state::abs_w(offs_t offset, u8 data)
{
	m_k1520->memory_w(0x1000 + offset, data);
}

void k8915_state::io_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x50, 0x53).rw("sio", FUNC(z80sio_device::ba_cd_r), FUNC(z80sio_device::ba_cd_w));
	map(0x58, 0x5b).rw("ctc", FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
	map(0xa8, 0xa8).w(FUNC(k8915_state::k8915_a8_w));
}

/* Input ports */
static INPUT_PORTS_START( k8915 )
INPUT_PORTS_END

void k8915_state::machine_reset()
{
	m_bank1->set_entry(1);
}

void k8915_state::machine_start()
{
	m_bank1->configure_entry(0, m_ram);
	m_bank1->configure_entry(1, m_rom);
}


void k8915_state::k8915(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, XTAL(4'915'200) / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &k8915_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &k8915_state::io_map);

	K1520_BUS(config, m_k1520, XTAL(9'830'400));
	k1520_abs_k7024_device &abs(K1520_ABS(config, "abs", 0));
	abs.set_slot(*m_k1520, 1);

	z80ctc_device& ctc(Z80CTC(config, "ctc", XTAL(4'915'200) / 2));
	ctc.set_clk<2>(XTAL(4'915'200) / 2);
	ctc.zc_callback<2>().set("sio", FUNC(z80sio_device::rxtxcb_w));

	z80sio_device& sio(Z80SIO(config, "sio", XTAL(4'915'200) / 2));
	sio.out_txdb_callback().set("rs232", FUNC(rs232_port_device::write_txd));
	sio.out_dtrb_callback().set("rs232", FUNC(rs232_port_device::write_dtr));
	sio.out_rtsb_callback().set("rs232", FUNC(rs232_port_device::write_rts));

	rs232_port_device &rs232(RS232_PORT(config, "rs232", default_rs232_devices, "keyboard"));
	rs232.rxd_handler().set("sio", FUNC(z80sio_device::rxb_w));
	rs232.dcd_handler().set("sio", FUNC(z80sio_device::dcdb_w));
	rs232.cts_handler().set("sio", FUNC(z80sio_device::ctsb_w));
}


/* ROM definition */
ROM_START( k8915 )
	ROM_REGION( 0x1000, "maincpu", 0 )
	ROM_LOAD( "k8915.bin", 0x0000, 0x1000, CRC(ca70385f) SHA1(a34c14adae9be821678aed7f9e33932ee1f3e61c))

	ROM_REGION( 0x2000, "abs:chargen", 0 )
	ROM_LOAD( "v171.rom", 0x0000, 0x800, CRC(06c8c709) SHA1(35a1548398f8a8906678e48bfc15e5a5c3a106e6))
	ROM_LOAD( "v172.rom", 0x0800, 0x800, CRC(543a4cdb) SHA1(9a57ede2c4fc734de03c5c7f6352b75625f58fd7))
ROM_END

} // anonymous namespace


/* Driver */

//    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  CLASS        INIT        COMPANY     FULLNAME  FLAGS
COMP( 1982, k8915,  0,      0,      k8915,   k8915, k8915_state, empty_init, "Robotron", "K8915",  MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE )
