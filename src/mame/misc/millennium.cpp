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

	void millennium_io(address_map &map) ATTR_COLD;
	void millennium_mem(address_map &map) ATTR_COLD;

	u8 m_p41; // shadow for PPI Port B
	u8 m_p40;
	u8 m_shift;
	u8 m_p60;
	u8 m_pe0_flip;
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

	// Port 0x60 bit 7 is START line for UPD7759
	m_adpcm->start_w(BIT(data, 7));

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
	// Coin validator status. Bit 6 = Ready.
	// Bit 5 is used as a data strobe in some ROMs.
	m_pe0_flip ^= 0x20;
	return 0x40 | m_pe0_flip;
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
	save_item(NAME(m_p40));
	save_item(NAME(m_p41));
	save_item(NAME(m_shift));
	save_item(NAME(m_p60));
	save_item(NAME(m_pe0_flip));
}


void millennium_state::machine_reset()
{
	m_p40 = 0;
	m_p41 = 0;
	m_shift = 0;
	m_p60 = 0;
	m_pe0_flip = 0;
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
	ROM_LOAD( "06jaa03.u5", 0x00000, 0x40000, CRC(0f25bfbd) SHA1(935638031ad9f2f23d95d4f753407aa99221442d))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "06fen01.u10", 0x00000, 0x80000, CRC(86258462) SHA1(af365e55991cc709232fa8a5715d56ca843b0a36))
ROM_END

ROM_START( m12aav08 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "12aav08.u5", 0x00000, 0x40000, CRC(62867a1f) SHA1(d75912a5c0705a1669ae9456feea034e0183abaf))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "12aav08.u10", 0x00000, 0x80000, CRC(c24b856c) SHA1(bd92b3941401ba76cbdbce9df370cc75c45509b4))
ROM_END

ROM_START( mnba1f02 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "nba1f02.u5", 0x00000, 0x40000, CRC(f6e04336) SHA1(4c9a7b47c048b81dd0f420ac82e403f776484aea))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "qo1sfk-01.u10", 0x00000, 0x80000, CRC(62e399e5) SHA1(2a3cd783c3d24d2daa2091aeda9c2f544f5f2a15))
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

} // anonymous namespace

//    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT   CLASS         INIT        COMPANY        FULLNAME          FLAGS */
COMP( 1993, m06jaa03, 0,      0,      millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.7 Multipay Multicard English/Japanese)", MACHINE_NOT_WORKING )
COMP( 1993, m12aav08, 0,      0,      millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.7 Multipay Multicard)", MACHINE_NOT_WORKING )
COMP( 1993, mnba1f02, 0,      0,      millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.9 Multipay Multicard English/French)", MACHINE_NOT_WORKING )
COMP( 1993, mnna1f02, 0,      0,      millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.9 Multipay Multicard TTY English/French)", MACHINE_NOT_WORKING )
COMP( 1993, mnnk1f05, 0,      0,      millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.13 Multipay Multicard TTY English/French)", MACHINE_NOT_WORKING )
COMP( 1993, mnpa1f01, 0,      0,      millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (1.20 Multipay Multicard TTY English/French)", MACHINE_NOT_WORKING )
