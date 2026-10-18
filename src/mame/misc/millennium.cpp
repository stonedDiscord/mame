// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Nortel Millennium Payphone

     CPU: Z8S18020
  MEMORY: M5M5256BP-12LL
     OSC: 28.636363 MHz
   EPROM: M27C2001 Program, M27C4001 Voiceware
  EEPROM: 93C46 (64x16 mode)
 DISPLAY: Noritake CU20026S (2x20 VFD)
   SOUND: UPD7759

   The telephony processor firmware is undumped.  Its CSI/O startup and
   diagnostic replies are simulated here; call handling is not implemented.

***************************************************************************/

#include "emu.h"

#include "bus/rs232/rs232.h"
#include "cpu/z180/z180.h"
#include "machine/eepromser.h"
#include "machine/i8255.h"
#include "machine/nvram.h"
#include "machine/timer.h"
#include "sound/upd7759.h"
#include "video/noritake_vfd.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"

#define VERBOSE 0
#include "logmacro.h"

namespace {

class millennium_state : public driver_device
{
public:
	millennium_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_ppi(*this, "ppi")
		, m_eeprom(*this, "eeprom")
		, m_adpcm(*this, "adpcm")
		, m_vfd(*this, "vfd")
	{
	}

	void millennium(machine_config &config);

private:
	required_device<z180_device> m_maincpu;
	required_device<i8255_device> m_ppi;
	required_device<eeprom_serial_93c46_16bit_device> m_eeprom;
	required_device<upd7759_device> m_adpcm;
	required_device<noritake_vfd_device> m_vfd;

	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

	void ppi_pa_w(u8 data);
	u8 ppi_pa_r();
	void ppi_pb_w(u8 data);
	void ppi_pc_w(u8 data);

	void p60_w(u8 data);
	void pa0_w(u8 data);
	void pc0_w(u8 data);
	u8 p60_r();
	u8 p80_r();
	u8 pe0_r();
	void telephony_clock_w(int state);
	TIMER_CALLBACK_MEMBER(telephony_tick);
	TIMER_CALLBACK_MEMBER(telephony_startup);
	void telephony_data_w(int state) { m_telephony_txd = state; }
	void telephony_command(u8 data);
	void telephony_reply(std::initializer_list<u8> data);

	void millennium_io(address_map &map) ATTR_COLD;
	void millennium_mem(address_map &map) ATTR_COLD;

	u8 m_p41; // shadow for PPI Port B
	u8 m_p40;
	u8 m_shift;
	u8 m_p60;
	u8 m_pe0_flip;

	// Behavioural substitute for the undumped telephony processor, on the CSI/O bus.
	u8 m_telephony_reply[256] = {};
	u8 m_telephony_head = 0;
	u8 m_telephony_tail = 0;
	u8 m_telephony_bits = 0;
	u8 m_telephony_shift = 0;
	u8 m_telephony_length = 0;
	u8 m_telephony_checksum = 0;
	bool m_telephony_frame = false;
	bool m_telephony_sending = false;
	bool m_telephony_txd = true;
	bool m_telephony_clock = true;
	u8 m_telephony_edges = 0;
	emu_timer *m_telephony_timer = nullptr;
	emu_timer *m_telephony_startup_timer = nullptr;
};


u8 millennium_state::ppi_pa_r()
{
	u8 data = m_ppi->pa_r(); // Get current latch
	// When EEPROM is selected, Port A bit 0 mirrors MISO
	if (BIT(m_p60, 6))
	{
		if (m_eeprom->do_read())
			data |= 0x01;
		else
			data &= ~0x01;
	}
	return data;
}

void millennium_state::ppi_pa_w(u8 data)
{
	m_eeprom->di_write(BIT(data, 0));

	if (BIT(m_p40, 1) && !BIT(data, 1))
		m_shift = (m_shift >> 1) | (BIT(~data, 0) << 7);

	if (!BIT(m_p40, 3) && BIT(data, 3))
	{
		if ((data & 0x30) == 0x00)
			m_vfd->data_write(m_shift);
		else if ((data & 0x30) == 0x10)
			m_vfd->control_write(m_shift);
	}
	m_p40 = data;
}

void millennium_state::ppi_pb_w(u8 data)
{
	// EEPROM clock on falling edge of bit 0
	if (BIT(m_p41, 0) && !BIT(data, 0))
	{
		m_eeprom->clk_write(0);
		m_eeprom->clk_write(1);
		m_eeprom->clk_write(0);
	}
	m_p41 = data;
}

void millennium_state::ppi_pc_w(u8 data)
{
	// Card reader control etc.
}

u8 millennium_state::p60_r()
{
	return m_p60;
}

void millennium_state::p60_w(u8 data)
{
	m_eeprom->cs_write(BIT(data, 6));

	// TODO: verify the UPD7759 START wiring.
	m_adpcm->start_w(BIT(data, 7));

	// The telephony processor supplies the external CSI/O clock after the
	// control processor asserts its active-low transfer request.
	if (BIT(m_p60, 7) && !BIT(data, 7) && !m_telephony_edges)
	{
		// TODO: determine the telephony processor's actual serial clock rate.
		m_telephony_edges = 16;
		m_telephony_timer->adjust(attotime::from_usec(10), 0, attotime::from_usec(10));
	}

	m_p60 = data;
}

u8 millennium_state::p80_r()
{
	u8 data = 0;
	// Port 0x80 bit 4 is EEPROM MISO
	if (m_eeprom->do_read())
		data |= 0x10;
	return data;
}

void millennium_state::pa0_w(u8 data)
{
	// Port 0x41 bits 5-6 select target on secondary bus
	u8 addr = m_p41 & 0x60;
	if (addr == 0x00) // P41_VFD_DATA_ADDR
		m_vfd->data_write(data);
	else if (addr == 0x20) // P41_VFD_CMD_ADDR
		m_vfd->control_write(data);
	else if (addr == 0x40) // P41_SOUND_ADDR
		m_adpcm->port_w(data);
}

void millennium_state::pc0_w(u8 data)
{
	LOG("Coin control write: %02X\n", data);
}

u8 millennium_state::pe0_r()
{
	// Bit 6 is the telephony processor's active-low request to send.
	// Bit 5 is used as a data strobe in some ROMs.
	m_pe0_flip ^= 0x20;
	return ((m_telephony_head == m_telephony_tail) ? 0x40 : 0x00) | m_pe0_flip;
}

void millennium_state::telephony_reply(std::initializer_list<u8> data)
{
	for (u8 const byte : data)
	{
		if (u8(m_telephony_tail + 1) == m_telephony_head)
		{
			logerror("Telephony reply buffer overflow\n");
			return;
		}
		m_telephony_reply[m_telephony_tail++] = byte;
	}
}

void millennium_state::telephony_clock_w(int state)
{
	if (m_telephony_clock == bool(state))
		return;
	m_telephony_clock = state;

	// CSI/O shifts least significant bit first, changing data on falling edges.
	if (!state)
	{
		if (!m_telephony_bits)
		{
			m_telephony_sending = m_telephony_head != m_telephony_tail;
			m_telephony_shift = m_telephony_sending ? m_telephony_reply[m_telephony_head] : 0;
		}
		m_maincpu->rxs_cts1_w(m_telephony_sending ? BIT(m_telephony_shift, m_telephony_bits) : 1);
	}
	else
	{
		if (!m_telephony_sending)
			m_telephony_shift |= u8(m_telephony_txd) << m_telephony_bits;
		if (++m_telephony_bits == 8)
		{
			m_telephony_bits = 0;
			if (m_telephony_sending)
			{
				LOG("Telephony reply: %02x\n", m_telephony_shift);
				++m_telephony_head;
			}
			else
				telephony_command(m_telephony_shift);
		}
	}
}

TIMER_CALLBACK_MEMBER(millennium_state::telephony_tick)
{
	int const state = !m_telephony_clock;
	telephony_clock_w(state);
	m_maincpu->cks_w(state);
	if (!--m_telephony_edges)
		m_telephony_timer->adjust(attotime::never);
}

TIMER_CALLBACK_MEMBER(millennium_state::telephony_startup)
{
	telephony_reply({ 0x70 }); // telephony processor power-on reset
}

void millennium_state::telephony_command(u8 data)
{
	LOG("Telephony command: %02x\n", data);

	// Configuration frames contain an opcode, total length, payload and a payload sum8.
	if (m_telephony_frame)
	{
		if (!m_telephony_length)
		{
			if (data < 3)
				m_telephony_frame = false;
			else
				m_telephony_length = data - 2;
		}
		else if (--m_telephony_length)
			m_telephony_checksum += data;
		else
		{
			m_telephony_frame = false;
			if (data == m_telephony_checksum)
				telephony_reply({ 0x74 }); // configuration accepted
			else
				telephony_reply({ 0xc4, 4, 1, 0xc9 }); // checksum error
		}
		return;
	}

	switch (data)
	{
	case 0x30: // hook status: on hook
		telephony_reply({ 0x6c });
		break;
	case 0x31: // telephony status (checksum includes the opcode and length)
		telephony_reply({ 0xc0, 8, 0, 0, 0, 0, 0, 0xc8 });
		break;
	case 0x33: // power status
		telephony_reply({ 0x7a });
		break;
	case 0x37: // central office line status
		telephony_reply({ 0x80 });
		break;
	case 0x38: // error report
		telephony_reply({ 0xc4, 4, 0, 0xc8 });
		break;
	case 0x3a: // key matrix: no key pressed
		telephony_reply({ 0x88 });
		break;
	case 0x3b: // handset continuity
		telephony_reply({ 0x8c });
		break;
	case 0xc0: // telephony configuration
		m_telephony_frame = true;
		m_telephony_length = 0;
		m_telephony_checksum = 0;
		break;
	}
}

void millennium_state::millennium_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x00000, 0x3ffff).rom();
	map(0x40000, 0x47fff).ram();
}

void millennium_state::millennium_io(address_map &map)
{
	map.unmap_value_high();
	map.global_mask(0xff);
	map(0x00, 0x3f).noprw(); /* Z180 internal registers */
	map(0x40, 0x43).rw(m_ppi, FUNC(i8255_device::read), FUNC(i8255_device::write));
	map(0x60, 0x60).rw(FUNC(millennium_state::p60_r), FUNC(millennium_state::p60_w));
	map(0x80, 0x80).r(FUNC(millennium_state::p80_r));
	map(0xa0, 0xa0).w(FUNC(millennium_state::pa0_w));
	map(0xc0, 0xc0).w(FUNC(millennium_state::pc0_w));
	map(0xe0, 0xe0).r(FUNC(millennium_state::pe0_r));
}

static INPUT_PORTS_START( millennium )
	PORT_START("KEYPAD")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) // Dummy
INPUT_PORTS_END


void millennium_state::machine_start()
{
	m_telephony_timer = timer_alloc(FUNC(millennium_state::telephony_tick), this);
	m_telephony_startup_timer = timer_alloc(FUNC(millennium_state::telephony_startup), this);

	save_item(NAME(m_p40));
	save_item(NAME(m_p41));
	save_item(NAME(m_shift));
	save_item(NAME(m_p60));
	save_item(NAME(m_pe0_flip));
	save_item(NAME(m_telephony_reply));
	save_item(NAME(m_telephony_head));
	save_item(NAME(m_telephony_tail));
	save_item(NAME(m_telephony_bits));
	save_item(NAME(m_telephony_shift));
	save_item(NAME(m_telephony_length));
	save_item(NAME(m_telephony_checksum));
	save_item(NAME(m_telephony_frame));
	save_item(NAME(m_telephony_sending));
	save_item(NAME(m_telephony_txd));
	save_item(NAME(m_telephony_clock));
	save_item(NAME(m_telephony_edges));
}


void millennium_state::machine_reset()
{
	m_p40 = 0;
	m_p41 = 0;
	m_shift = 0;
	m_p60 = 0;
	m_pe0_flip = 0;
	m_telephony_head = 0;
	m_telephony_tail = 0;
	m_telephony_bits = 0;
	m_telephony_shift = 0;
	m_telephony_length = 0;
	m_telephony_checksum = 0;
	m_telephony_frame = false;
	m_telephony_sending = false;
	m_telephony_txd = true;
	m_telephony_clock = true;
	m_telephony_edges = 0;
	m_telephony_timer->adjust(attotime::never);
	m_maincpu->cks_w(1);
	m_maincpu->rxs_cts1_w(1);
	// Allow the telephony processor to initialise before advertising its reply.
	// TODO: measure the actual power-on delay.
	m_telephony_startup_timer->adjust(attotime::from_msec(100));
}

void millennium_state::millennium(machine_config &config)
{
	/* basic machine hardware */
	Z8S180(config, m_maincpu, 28'636'363_Hz_XTAL / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &millennium_state::millennium_mem);
	m_maincpu->set_addrmap(AS_IO, &millennium_state::millennium_io);
	m_maincpu->set_periodic_int(FUNC(millennium_state::irq2_line_hold), attotime::from_hz(100));
	m_maincpu->txa0_wr_callback().set("serial", FUNC(rs232_port_device::write_txd));
	m_maincpu->rts0_wr_callback().set("serial", FUNC(rs232_port_device::write_rts));
	m_maincpu->txs_wr_callback().set(FUNC(millennium_state::telephony_data_w));

	I8255(config, m_ppi);
	m_ppi->in_pa_callback().set(FUNC(millennium_state::ppi_pa_r));
	m_ppi->out_pa_callback().set(FUNC(millennium_state::ppi_pa_w));
	m_ppi->out_pb_callback().set(FUNC(millennium_state::ppi_pb_w));
	m_ppi->out_pc_callback().set(FUNC(millennium_state::ppi_pc_w));

	EEPROM_93C46_16BIT(config, m_eeprom);

	UPD7759(config, m_adpcm, 640_kHz_XTAL).add_route(ALL_OUTPUTS, "mono", 0.30);

	rs232_port_device &rs232(RS232_PORT(config, "serial", default_rs232_devices, nullptr));
	rs232.rxd_handler().set(m_maincpu, FUNC(z180_device::rxa0_w));
	rs232.cts_handler().set(m_maincpu, FUNC(z180_device::cts0_w));
	rs232.cts_handler().append_inputline(m_maincpu, Z180_INPUT_LINE_DREQ0).invert();

	// Noritake VFD CU20026SCPB
	screen_device &screen(SCREEN(config, "screen"));
	screen.set_color(rgb_t(6, 120, 245));
	screen.set_physical_aspect(20, 2);
	screen.set_refresh_hz(72);
	screen.set_size(6*20, 9*2);
	screen.set_visarea_full();
	screen.set_screen_update(m_vfd, FUNC(noritake_vfd_device::screen_update));
	screen.set_palette("palette");

	PALETTE(config, "palette", palette_device::MONOCHROME);

	NORITAKE_VFD(config, m_vfd, 270'000);
	m_vfd->set_lcd_size(2, 20);

	SPEAKER(config, "mono").front_center();
}

ROM_START( m06jaa03 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "06jaa03_a0651818_2386.u5", 0x00000, 0x40000, CRC(0f25bfbd) SHA1(935638031ad9f2f23d95d4f753407aa99221442d))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "06fen01_nt5u40bj.u10", 0x00000, 0x80000, CRC(86258462) SHA1(af365e55991cc709232fa8a5715d56ca843b0a36))
ROM_END

ROM_START( m12aav08 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "12aav08.u5", 0x00000, 0x40000, CRC(62867a1f) SHA1(d75912a5c0705a1669ae9456feea034e0183abaf))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "12aav08.u10", 0x00000, 0x80000, CRC(c24b856c) SHA1(bd92b3941401ba76cbdbce9df370cc75c45509b4))
ROM_END

ROM_START( mnba1f02 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "nba1f02_a0670082_0307.u5", 0x00000, 0x40000, CRC(f6e04336) SHA1(4c9a7b47c048b81dd0f420ac82e403f776484aea))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "qo1sfk-01_a0370347.u10", 0x00000, 0x80000, CRC(62e399e5) SHA1(2a3cd783c3d24d2daa2091aeda9c2f544f5f2a15))
ROM_END

ROM_START( mnna1f02 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "nna1f02_31c6_1.9_tty.u5", 0x00000, 0x40000, CRC(5cfd69e8) SHA1(c644ef404013237a18e46364a7b70c99c4c8c042))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "lm5u09ba_a0623480_2b52.u10", 0x00000, 0x80000, CRC(11bbeefc) SHA1(302daf62d5a0d4f91266a3bcc4364f3a523bb168))
ROM_END

ROM_START( mnnk1f05 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "nnk1f05.u5", 0x00000, 0x40000, CRC(be97a428) SHA1(1d15daef8b3408548d1372cd1daba1945a4bb9e0))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "06ebc01_a0623480_2b52.u10", 0x00000, 0x80000, CRC(f7e19315) SHA1(e93c4be85d63b6e8179c6e250465186e532d7597))
ROM_END

ROM_START( mnpa1f01 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "npa1f01_d5d6.u5", 0x00000, 0x40000, CRC(f4f8e606) SHA1(f65cf73e3228b8007b024919503a740bd9c09269))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "06ebc01_a0623480_2b52.u10", 0x00000, 0x80000, CRC(f7e19315) SHA1(e93c4be85d63b6e8179c6e250465186e532d7597))
ROM_END

ROM_START( mnpa1s01 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "npa1s01_d5d6.u5", 0x00000, 0x40000, CRC(04595d4d) SHA1(0cd090549c5ef9c8ee532d0b4972d3214db96741))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "06eaj01_a0620793.u10", 0x00000, 0x80000, CRC(d4a12c41) SHA1(160bc53559798e63d0252829f8341839bb0d1b27))
ROM_END

} // anonymous namespace

//    YEAR  NAME      PARENT COMPAT MACHINE     INPUT       CLASS             INIT        COMPANY   FULLNAME                                                   FLAGS
COMP( 1993, m06jaa03, mnpa1s01, 0, millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.7 Multipay Multicard English/Japanese)",    MACHINE_NOT_WORKING )
COMP( 1993, m12aav08, mnpa1s01, 0, millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.7 Multipay Multicard English/Spanish)",     MACHINE_NOT_WORKING )
COMP( 1993, mnba1f02, mnpa1s01, 0, millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.9 Multipay Multicard English/French)",      MACHINE_NOT_WORKING )
COMP( 1993, mnna1f02, mnpa1s01, 0, millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.9 Multipay Multicard TTY English/French)",  MACHINE_NOT_WORKING )
COMP( 1993, mnnk1f05, mnpa1s01, 0, millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.13 Multipay Multicard TTY English/French)", MACHINE_NOT_WORKING )
COMP( 1993, mnpa1f01, mnpa1s01, 0, millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.20 Multipay Multicard English/French)",     MACHINE_NOT_WORKING )
COMP( 1993, mnpa1s01,        0, 0, millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.20 Multipay Multicard English/Spanish)",    MACHINE_NOT_WORKING )
