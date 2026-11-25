// license:BSD-3-Clause
// copyright-holders:stonedDiscord
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

TODO:
Verify crystal.
Most of the memory map is guessed.
Trace the lamp outs.
*/


#include "emu.h"

#include "cpu/m68000/m68008.h"
#include "machine/6840ptm.h"
#include "machine/6850acia.h"
#include "machine/clock.h"
#include "machine/msm6242.h"
#include "machine/nvram.h"
#include "sound/ay8910.h"
#include "video/roc10937.h"

#include "eurotec.lh"

#include "speaker.h"

namespace {

class t2000_state : public driver_device
{
public:
	t2000_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_nvram(*this, "nvram"),
		m_aysnd(*this, "aysnd"),
		m_ptm(*this, "ptm"),
		m_acia(*this, "acia"),
		m_rtc(*this, "rtc"),
		m_vfd(*this, "vfd")
	{ }

	void t2000(machine_config &config) ATTR_COLD;

private:
	void mem_map(address_map &map) ATTR_COLD;
	void cpu_space_map(address_map &map) ATTR_COLD;
	u8 display_r(offs_t offset);
	void display_w(offs_t offset, u8 data);
	u8 ptm_r(offs_t offset);
	void ptm_w(offs_t offset, u8 data);
	void ptm_irq(int state);
	void acia_irq(int state);
	void rtc_irq(int state);
	u8 ay_porta_r() const { return m_ay_porta; }
	u8 ay_portb_r() const { return m_ay_portb; }
	void ay_porta_w(u8 data) { m_ay_porta = data; }
	void ay_portb_w(u8 data) { m_ay_portb = data; }
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;
	IRQ_CALLBACK_MEMBER(irq_ack);

	// devices
	required_device<m68008_device> m_maincpu;
	required_device<nvram_device> m_nvram;
	required_device<ym2149_device> m_aysnd;
	required_device<ptm6840_device> m_ptm;
	required_device<acia6850_device> m_acia;
	required_device<rtc72421_device> m_rtc;
	required_device<roc10937_device> m_vfd;

	// The Technik display board is driven through the sixteen-byte I/O window.
	// Keep the latches separate: the firmware multiplexes segment data and the
	// digit/strobe lines independently.
	u8 m_display_latch[0x10]{};
	bool m_display_data_pending = false;
	bool m_acia_irq = false;
	bool m_rtc_irq = false;
	u8 m_irq_vector = 0x40;
	u8 m_ay_porta = 0xff;
	u8 m_ay_portb = 0xff;
};

u8 t2000_state::ptm_r(offs_t offset)
{
	u8 const data = m_ptm->read(offset);
	if (offset == 1)
	{
		// The board presents timer 3's interrupt to the firmware's timer-1
		// status input.  The common ISR acknowledges that input by reading
		// c0002 (timer 1 in the generic PTM map), so acknowledge timer 3 here
		// as well when that is the source being reported.
		if (data & 0x04)
			m_ptm->read(6);

		// The board routes the PTM channel-3 flag to the firmware's timer-1
		// status input.  The ROM polls bit 0 while the 6840 reports bit 2.
		return (data & ~0x04) | ((data & 0x04) >> 2);
	}
	return data;
}

void t2000_state::ptm_w(offs_t offset, u8 data)
{
	if (offset == 0)
		data |= 0x40;
	m_ptm->write(offset, data);
}

void t2000_state::acia_irq(int state)
{
	m_acia_irq = bool(state);
	m_maincpu->set_input_line(M68K_IRQ_4, (m_acia_irq || m_ptm->irq_state()) ? ASSERT_LINE : CLEAR_LINE);
}

void t2000_state::ptm_irq(int state)
{
	m_maincpu->set_input_line(M68K_IRQ_4, (state || m_acia_irq || m_rtc_irq) ? ASSERT_LINE : CLEAR_LINE);
}

void t2000_state::rtc_irq(int state)
{
	m_rtc_irq = bool(state);
	m_maincpu->set_input_line(M68K_IRQ_4, (m_rtc_irq || m_acia_irq || m_ptm->irq_state()) ? ASSERT_LINE : CLEAR_LINE);
}

IRQ_CALLBACK_MEMBER(t2000_state::irq_ack)
{
	// Board glue supplies a vector for each interrupt source.  Timer 3 drives
	// the display ISR (vector 0x46); timer 1 drives the scheduler (0x40).
	u8 const status = m_ptm->status_reg();
	if (m_acia_irq)
		m_irq_vector = 0x42;
	else if (m_rtc_irq)
		m_irq_vector = 0x43;
	else if (status & 0x01)
		m_irq_vector = 0x40;
	else if (status & 0x04)
		m_irq_vector = 0x46;
	else if (status & 0x02)
		m_irq_vector = 0x43;
	return m_irq_vector;
}

void t2000_state::mem_map(address_map &map)
{
	map(0x00000, 0x3ffff).rom();
	map(0x40000, 0x47fff).ram().share("nvram"); //84256A
	map(0x80000, 0x8000f).rw(m_rtc, FUNC(rtc72421_device::read), FUNC(rtc72421_device::write));
	map(0xc0000, 0xc0007).rw(FUNC(t2000_state::ptm_r), FUNC(t2000_state::ptm_w));
	map(0xc0010, 0xc0010).w(m_aysnd, FUNC(ym2149_device::address_w));
	map(0xc0011, 0xc0011).w(m_aysnd, FUNC(ym2149_device::address_w));
	map(0xc0012, 0xc0012).rw(m_aysnd, FUNC(ym2149_device::data_r), FUNC(ym2149_device::data_w));
	map(0xc0013, 0xc0013).rw(m_aysnd, FUNC(ym2149_device::data_r), FUNC(ym2149_device::data_w));
	map(0xc0020, 0xc0020).rw(m_acia, FUNC(acia6850_device::status_r), FUNC(acia6850_device::control_w));
	map(0xc0022, 0xc0022).rw(m_acia, FUNC(acia6850_device::data_r), FUNC(acia6850_device::data_w));
	map(0xd0000, 0xd000f).rw(FUNC(t2000_state::display_r), FUNC(t2000_state::display_w)); // multiplexed inputs and outputs
}

void t2000_state::cpu_space_map(address_map &map)
{
	// IRQ4 is a vectored PTM interrupt.  The acknowledge callback selects the
	// source-specific vector before the CPU reads this CPU-space location.
	map(0xffff9, 0xffff9).lr8(NAME([this]() { return m_irq_vector; }));
}

u8 t2000_state::display_r(offs_t offset)
{
	if ((offset & 0x0f) == 0x03)
		// Board input-multiplexer power-on test: d0004 selects the input bank and d0003 loops
		// that selector back before the firmware enables normal operation.
		return m_display_latch[0x04];
	return m_display_latch[offset & 0x0f];
}

void t2000_state::display_w(offs_t offset, u8 data)
{
	offset &= 0x0f;
	m_display_latch[offset] = data;
	// Board glue presents each d0005 write as one serial byte while d0006 bit 0
	// is asserted.  ROC10937 shifts data on the falling clock edge.
	if (offset == 0x05 && BIT(m_display_latch[0x06], 0))
	{
		for (int bit = 7; bit >= 0; bit--)
		{
			m_vfd->data(BIT(data, bit));
			m_vfd->sclk(1);
			m_vfd->sclk(0);
		}
		m_display_data_pending = false;
	}
}

void t2000_state::machine_start()
{
	save_item(NAME(m_display_latch));
	save_item(NAME(m_display_data_pending));
	save_item(NAME(m_acia_irq));
	save_item(NAME(m_rtc_irq));
	save_item(NAME(m_irq_vector));
	save_item(NAME(m_ay_porta));
	save_item(NAME(m_ay_portb));
}

void t2000_state::machine_reset()
{
	std::fill(std::begin(m_display_latch), std::end(m_display_latch), 0);
	m_display_data_pending = false;
	m_acia_irq = false;
	m_rtc_irq = false;
	m_irq_vector = 0x40;
	m_ay_porta = 0xff;
	m_ay_portb = 0xff;
	m_vfd->por(1);
}

static INPUT_PORTS_START( t2000 )
	PORT_START("IN0")
INPUT_PORTS_END


void t2000_state::t2000(machine_config &config)
{
	M68008(config, m_maincpu, 16_MHz_XTAL / 2); // guess
	m_maincpu->set_addrmap(AS_PROGRAM, &t2000_state::mem_map);
	m_maincpu->set_addrmap(m68000_base_device::AS_CPU_SPACE, &t2000_state::cpu_space_map);
	m_maincpu->set_irq_acknowledge_callback(FUNC(t2000_state::irq_ack));

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0); // battery backed

	YM2149(config, m_aysnd, 16_MHz_XTAL / 8); // guess
	m_aysnd->port_a_read_callback().set(FUNC(t2000_state::ay_porta_r));
	m_aysnd->port_b_read_callback().set(FUNC(t2000_state::ay_portb_r));
	m_aysnd->port_a_write_callback().set(FUNC(t2000_state::ay_porta_w));
	m_aysnd->port_b_write_callback().set(FUNC(t2000_state::ay_portb_w));
	m_aysnd->add_route(ALL_OUTPUTS, "mono", 1);

	SPEAKER(config, "mono").front_center();

	PTM6840(config, m_ptm, 16_MHz_XTAL / 1024);
	m_ptm->irq_callback().set(FUNC(t2000_state::ptm_irq));

	ACIA6850(config, m_acia);
	m_acia->irq_handler().set(FUNC(t2000_state::acia_irq));
	CLOCK(config, "acia_clock", 16_MHz_XTAL / 1024).signal_handler().set(m_acia, FUNC(acia6850_device::write_txc));
	CLOCK(config, "acia_clock_rxc", 16_MHz_XTAL / 1024).signal_handler().set(m_acia, FUNC(acia6850_device::write_rxc));

	RTC72421(config, m_rtc, XTAL(32'768));
	m_rtc->out_int_handler().set(FUNC(t2000_state::rtc_irq));

	config.set_default_layout(layout_eurotec);
	ROC10937(config, m_vfd);
}

ROM_START( bmonop )
    ROM_REGION( 0x40000, "maincpu", 0 )
    ROM_LOAD("monopoly.ic2", 0x00000, 0x20000, CRC(ec000687) SHA1(4f8aec5eeece21681b2430c209eede223e30673e))
	ROM_LOAD("monopoly.ic4", 0x20000, 0x20000, CRC(d8c26b59) SHA1(ca78fe99bbba46ee49fcca261947a345e8badb50))
ROM_END

ROM_START( bwjbond )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "324_james_bond.ic2", 0x00000, 0x20000, CRC(7581ca43) SHA1(fe2e40572644058b9a8084bec73e3dac7d625ba5) )
	ROM_LOAD( "324_james_bond.ic4", 0x20000, 0x20000, CRC(0621c582) SHA1(26a9f93d7122cf0cd29e419de89be735d11b84b2) )
ROM_END

ROM_START( bwlucky )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "265_lucky.ic2", 0x00000, 0x20000, CRC(85fe461d) SHA1(2f57fb40e704f0662c0c6b8b48b4ce93892d0842) )
	ROM_LOAD( "265_lucky.ic4", 0x20000, 0x20000, CRC(1e455b39) SHA1(ebde669805a27f8b677c2f467a81a2b23e79e00c) )
ROM_END

ROM_START( bwroadst )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "235_road_star.ic2", 0x00000, 0x20000, CRC(44b762e9) SHA1(12891417845a8fd4112148f0a9cdd7fb1795e7de) )
	ROM_LOAD( "235_road_star.ic4", 0x20000, 0x10000, CRC(32b74769) SHA1(d9831234f3fdfd1541d6fff0799a4d68514e653a) )
ROM_END

ROM_START( bwwuerfl )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "254_wuerfel.ic2", 0x00000, 0x20000, CRC(fa3517fa) SHA1(f1738cd64ac3ca87d91d584a098936e2256d0d7c) )
	ROM_LOAD( "254_wuerfel.ic4", 0x20000, 0x20000, CRC(8c3c4e28) SHA1(512646ffe095ffd5c249e1a2d9d2931797052431) )
ROM_END

ROM_START( glorias )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "gloria_super_dm_pr1.bin", 0x00000, 0x20000, CRC(4f5615a7) SHA1(9264d4dc1bb651ad8c4f84873e6e14ebbe9cd477) )
	ROM_LOAD( "gloria_super_dm_pr2.bin", 0x20000, 0x20000, CRC(34964967) SHA1(4dd4a918fcd00a35aca443cbbce1ee0cf0c25c3b) )
ROM_END

ROM_START( graffiti )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "281-0-e6.0_graffiti.ic2", 0x00000, 0x20000, CRC(90333f4c) SHA1(a81de22627f86c4889cbd65ff7a45a3d38966cc8) )
	ROM_LOAD( "281-2-e6.0_graffiti.ic4", 0x20000, 0x20000, CRC(0d94cab0) SHA1(a6a27208f0bc1c529d60c0574f00305f64fb7ade) )
ROM_END

ROM_START( pokerast )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "576_poker_as.ic2", 0x00000, 0x20000, CRC(f1d29b54) SHA1(bfa26f24db2e93834792062463bdc4c40a40bfac) )
	ROM_LOAD( "576_poker_as.ic4", 0x20000, 0x20000, CRC(46734685) SHA1(84c6ac4763768f5998dd7c602a08689669bd120a) )
ROM_END

ROM_START( roxyc )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "roxy_classic_dm_pr1.bin", 0x00000, 0x20000, CRC(23d7169c) SHA1(e154e57e8ca03dce190178a0221a059f9b00085e) )
	ROM_LOAD( "roxy_classic_dm_pr2.bin", 0x20000, 0x20000, CRC(f7e86f09) SHA1(8144378332b21bfe0a91c9124d37afcc9946d367) )
ROM_END

ROM_START( royaltr )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "255_royal_trio.ic2", 0x00000, 0x20000, CRC(8e682e6e) SHA1(de0cfe683019876c80d6c1195f268434704dd734) )
	ROM_LOAD( "255_royal_trio.ic4", 0x20000, 0x20000, CRC(0e7eeb48) SHA1(24709803356d0e27b0a2a2892769b3a1db3a2ce2) )
ROM_END

ROM_START( sonkrne6 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "325-e6.0_sonnenkrone.ic2", 0x00000, 0x20000, CRC(a92d6161) SHA1(f3589cc4592e6bebbb1ef2f4463a7cccdf4e1aae) )
	ROM_LOAD( "325-e6.0_sonnenkrone.ic4", 0x20000, 0x20000, CRC(2336fdcc) SHA1(8df2233ade47d163a28a4f506bceffd2ed23a8de) )
ROM_END

ROM_START( ultraple3 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "339-e3.0_ultra_play.ic2", 0x00000, 0x20000, CRC(2a21b331) SHA1(857a0094b809d7d86ce20d378f390981c6adc6d2) )
	ROM_LOAD( "339-e3.0_ultra_play.ic4", 0x20000, 0x20000, CRC(db913a2d) SHA1(1456b34e890abd477797585fc5af4b4efef022e8) )
ROM_END

} // anonymous namespace

GAME(1993, bwroadst, 0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Road Star",        MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAME(1994, bmonop,   0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Monopoly",         MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_REQUIRES_ARTWORK )
GAME(1994, bwuerfl,  0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", u8"Ballys Würfel",  MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAME(1995, graffiti, 0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Graffiti",         MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_REQUIRES_ARTWORK )
GAME(1995, pokerast, 0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Poker As",         MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAME(1996, bwjbond,  0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "James Bond",       MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAME(1996, sonkrne6, 0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Sonnenkrone (E6)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAME(1997, bwlucky,  0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Lucky",            MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAME(1997, glorias,  0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Gloria Super",     MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_REQUIRES_ARTWORK )
GAME(1997, roxyc,    0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Roxy Classic",     MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_REQUIRES_ARTWORK )
GAME(1997, royaltr,  0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Royal Trio",       MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAME(1997, ultrapl3, 0, t2000, t2000, t2000_state, empty_init, ROT0, "Rototron", "Ultra Play (E3)",  MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
