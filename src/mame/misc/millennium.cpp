// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Nortel Millennium Payphone

     CPU: Z8S18020
  MEMORY: M5M5256BP-12LL
     OSC: 
  EEPROM: M27C2001 Program, M27C4001 Voiceware
 DISPLAY: Noritake CU20026S
   SOUND: UPD7759


***************************************************************************/

#include "emu.h"

#include "bus/rs232/rs232.h"
#include "cpu/z180/z180.h"
#include "machine/i8255.h"
#include "machine/nvram.h"
#include "machine/timer.h"
#include "sound/upd7759.h"
#include "video/noritake_vfd.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"

#include "millennium.lh"

#define VERBOSE 1
#include "logmacro.h"

namespace {

class millennium_state : public driver_device
{
public:
	millennium_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_ppi(*this, "ppi")
		, m_adpcm(*this, "adpcm")
		, m_vfd(*this, "vfd")
	{
	}

	void millennium(machine_config &config);

private:
	required_device<z180_device> m_maincpu;
	required_device<i8255_device> m_ppi;
	required_device<upd7759_device> m_adpcm;
	required_device<noritake_vfd_device> m_vfd;

	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;
	void io_w(offs_t offset, u8 data);
	u8 io_r(offs_t offset);
	void portb_w(u8 data);

	void millennium_io(address_map &map) ATTR_COLD;
	void millennium_mem(address_map &map) ATTR_COLD;

};


u8 millennium_state::io_r(offs_t offset)
{
	return 0xff;
}


void millennium_state::io_w(offs_t offset, u8 data)
{
	;
}

void millennium_state::portb_w(u8 data)
{
	LOG("PortB write: %02X\n", data);
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
	map(0x99, 0x99).rw(FUNC(millennium_state::io_r), FUNC(millennium_state::io_w));
	map(0x40, 0x43).rw(m_ppi, FUNC(i8255_device::read), FUNC(i8255_device::write));
	map(0x60, 0x60).rw(m_vfd, FUNC(noritake_vfd_device::data_r), FUNC(noritake_vfd_device::data_w));
	map(0x80, 0x80).rw(m_vfd, FUNC(noritake_vfd_device::control_r), FUNC(noritake_vfd_device::control_w));
}

/* Input ports */
static INPUT_PORTS_START( millennium )
	PORT_START("KEYPAD")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_START1)

INPUT_PORTS_END


void millennium_state::machine_start()
{
	// state saving
	;
}


void millennium_state::machine_reset()
{
	;
}

void millennium_state::millennium(machine_config &config)
{
	/* basic machine hardware */
	Z8S180(config, m_maincpu, 28'636'363_Hz_XTAL / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &millennium_state::millennium_mem);
	m_maincpu->set_addrmap(AS_IO, &millennium_state::millennium_io);
	m_maincpu->txa0_wr_callback().set("serial", FUNC(rs232_port_device::write_txd));
	m_maincpu->rts0_wr_callback().set("serial", FUNC(rs232_port_device::write_rts));

	I8255(config, m_ppi, 0);
	m_ppi->out_pb_callback().set(FUNC(millennium_state::portb_w));

	UPD7759(config, m_adpcm, 640_kHz_XTAL).add_route(ALL_OUTPUTS, "mono", 0.30);

	rs232_port_device &rs232(RS232_PORT(config, "serial", default_rs232_devices, nullptr));
	rs232.rxd_handler().set(m_maincpu, FUNC(z180_device::rxa0_w));
	rs232.cts_handler().set(m_maincpu, FUNC(z180_device::cts0_w));
	rs232.cts_handler().append_inputline(m_maincpu, Z180_INPUT_LINE_DREQ0).invert();

	// LCD CU20026 Noritake VFD
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_LCD));
	screen.set_color(rgb_t(6, 120, 245));
	screen.set_physical_aspect(7*20, 10*4);
	screen.set_refresh_hz(72);
	screen.set_size(6*20, 9*4);
	screen.set_visarea_full();
	screen.set_screen_update(m_vfd, FUNC(noritake_vfd_device::screen_update));
	screen.set_palette("palette");

	PALETTE(config, "palette", palette_device::MONOCHROME);

	NORITAKE_VFD(config, m_vfd, 270'000);
	m_vfd->set_lcd_size(4, 20); // 4 lines, 20 characters

	SPEAKER(config, "mono").front_center();
}

/* ROM definition */
ROM_START( mnba1f02 )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "nba1f02.u5", 0x00000, 0x40000, CRC(f6e04336) SHA1(4c9a7b47c048b81dd0f420ac82e403f776484aea))
	ROM_REGION( 0x80000, "adpcm", ROMREGION_ERASEFF )
	ROM_LOAD( "qo1sfk-01.u10", 0x00000, 0x80000, CRC(62e399e5) SHA1(2a3cd783c3d24d2daa2091aeda9c2f544f5f2a15))
ROM_END

} // anonymous namespace


/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT   CLASS         INIT        COMPANY        FULLNAME          FLAGS */
COMP( 1993, mnba1f02, 0,      0,      millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (Multipay Multicard E/F)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
