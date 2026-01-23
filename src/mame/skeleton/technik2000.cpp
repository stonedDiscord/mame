// license:BSD-3-Clause
/*

Bally Wulff Technik 2000
German Fruit Machines / Gambling Machines

CPU: Motorola MC68008P10 DIP-48
RAM: Fujitsu MB84256A
RTC: Epson RTC-72421A
Timer: Motorola MC6840P
Serial: ST EF6850P
Audio: Yamaha YM2149F

Zentraleinheit 200.600.00

*/


#include "emu.h"

#include "cpu/m68000/m68008.h"
#include "machine/6840ptm.h"
#include "machine/6850acia.h"
#include "machine/msm6242.h"
#include "machine/nvram.h"
#include "sound/ay8910.h"
#include "video/roc10937.h"

#include "speaker.h"

#include "proconn.lh"

namespace {

class t2000_state : public driver_device
{
public:
	t2000_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_nvram(*this, "nvram"),
		m_vfd(*this, "vfd"),
		m_aysnd(*this, "aysnd"),
		m_ptm(*this, "ptm"),
		m_acia(*this, "acia"),
		m_rtc(*this, "rtc")
	{ }

	void t2000(machine_config &config);

private:
	virtual void machine_start() override;
	void mem_map(address_map &map) ATTR_COLD;

	void mux1_w(uint8_t data);

	INTERRUPT_GEN_MEMBER(watchdog_interrupt);
	void watchdog_interrupt_clear(uint8_t data);

	// devices
	required_device<cpu_device> m_maincpu;
	required_device<nvram_device> m_nvram;
	optional_device<rocvfd_device> m_vfd;
	required_device<ym2149_device> m_aysnd;
	required_device<ptm6840_device> m_ptm;
	required_device<acia6850_device> m_acia;
	required_device<rtc72421_device> m_rtc;
};
/*
void t2000_state::mux1_w(uint8_t data)
{
	m_vfd->por(data & 0x20);// wrong
	m_vfd->sclk(data & 0x80);
	m_vfd->data(data & 0x40);
}
*/
void t2000_state::mem_map(address_map &map)
{
	map(0x00000, 0x3ffff).rom(); 
	map(0x40000, 0x47fff).ram().share("nvram"); //84256A

}

void t2000_state::machine_start()
{
	;
}

INTERRUPT_GEN_MEMBER(t2000_state::watchdog_interrupt)
{
	m_maincpu->set_input_line(M68K_IRQ_IPL0, ASSERT_LINE);
}
/*
void t2000_state::watchdog_interrupt_clear(uint8_t data)
{
	m_maincpu->set_input_line(M68K_IRQ_IPL0, CLEAR_LINE);
}
*/
static INPUT_PORTS_START( t2000 )
	PORT_START("IN0")
INPUT_PORTS_END


void t2000_state::t2000(machine_config &config)
{
	M68008(config, m_maincpu, 16_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &t2000_state::mem_map);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0); // battery backed

	MSC1937(config, m_vfd);

	YM2149(config, m_aysnd, 16_MHz_XTAL);
	m_aysnd->add_route(ALL_OUTPUTS, "mono", 1);

	SPEAKER(config, "mono").front_center();

	PTM6840(config, m_ptm, 0);

	ACIA6850(config, m_acia);

	RTC72421(config, m_rtc, XTAL(32'768));

	config.set_default_layout(layout_proconn);

	m_maincpu->set_periodic_int(FUNC(t2000_state::watchdog_interrupt), attotime::from_hz(25000));

}

ROM_START( graffiti )
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD( "graffiti_281_0_e6.0.bin", 0x00000, 0x20000, CRC(90333f4c) SHA1(a81de22627f86c4889cbd65ff7a45a3d38966cc8) )
	ROM_LOAD( "graffiti_281_2_e6.0.bin", 0x20000, 0x20000, CRC(0d94cab0) SHA1(a6a27208f0bc1c529d60c0574f00305f64fb7ade) )
ROM_END

} // anonymous namespace

GAMEL(1995, graffiti, 0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Graffiti", MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_proconn )
