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

#include "cpu/z180/z180.h"
#include "machine/eepromser.h"
#include "machine/i8255.h"
#include "machine/nvram.h"
#include "machine/timer.h"
#include "sound/upd7759.h"

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
	{
	}

	void millennium(machine_config &config);

private:
	required_device<z180_device> m_maincpu;
	required_device<i8255_device> m_ppi;
	required_device<eeprom_serial_93c46_16bit_device> m_eeprom;
	required_device<upd7759_device> m_adpcm;

	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

	void ppi_pa_w(u8 data);
	u8 ppi_pa_r();
	void ppi_pb_w(u8 data);
	void ppi_pc_w(u8 data);

	void millennium_io(address_map &map) ATTR_COLD;
	void millennium_mem(address_map &map) ATTR_COLD;
};


u8 millennium_state::ppi_pa_r()
{
	u8 data = m_ppi->pa_r();

	return data;
}

void millennium_state::ppi_pa_w(u8 data)
{

}

void millennium_state::ppi_pb_w(u8 data)
{

}

void millennium_state::ppi_pc_w(u8 data)
{

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
}

/* Input ports */
static INPUT_PORTS_START( millennium )
	PORT_START("KEYPAD")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) // Dummy
INPUT_PORTS_END


void millennium_state::machine_start()
{

}


void millennium_state::machine_reset()
{

}

void millennium_state::millennium(machine_config &config)
{
	/* basic machine hardware */
	Z8S180(config, m_maincpu, 28'636'363_Hz_XTAL / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &millennium_state::millennium_mem);
	m_maincpu->set_addrmap(AS_IO, &millennium_state::millennium_io);

	I8255(config, m_ppi);
	m_ppi->in_pa_callback().set(FUNC(millennium_state::ppi_pa_r));
	m_ppi->out_pa_callback().set(FUNC(millennium_state::ppi_pa_w));
	m_ppi->out_pb_callback().set(FUNC(millennium_state::ppi_pb_w));
	m_ppi->out_pc_callback().set(FUNC(millennium_state::ppi_pc_w));

	EEPROM_93C46_16BIT(config, m_eeprom);

	UPD7759(config, m_adpcm, 640_kHz_XTAL).add_route(ALL_OUTPUTS, "mono", 0.30);

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
COMP( 1993, mnba1f02, 0,      0,      millennium,  millennium, millennium_state, empty_init, "Nortel", "Millennium (Multipay Multicard E/F)", MACHINE_NOT_WORKING )
