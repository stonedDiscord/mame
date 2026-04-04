// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/*
    Arachnid - Galaxy

DS1244

*/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "cpu/mcs48/mcs48.h"
#include "machine/6821pia.h"
#include "machine/mc68681.h"
#include "machine/nvram.h"
#include "sound/spkrdev.h"
#include "video/tms9928a.h"
#include "speaker.h"


namespace {

#define SCREEN_TAG      "screen"
#define M6809_TAG       "u9"
#define TMS9118_TAG     "u5"
#define PIA6821_U11_TAG "u11"
#define PIA6821_U15_TAG "u15"
#define PIA6821_U17_TAG "u17"
#define SPEAKER_TAG     "speaker"

class arachnidgalaxy_state : public driver_device
{
public:
	arachnidgalaxy_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, M6809_TAG),
			m_pia_u11(*this, PIA6821_U11_TAG),
			m_pia_u15(*this, PIA6821_U15_TAG),
			m_pia_u17(*this, PIA6821_U17_TAG),
			m_speaker(*this, SPEAKER_TAG)
	{ }

	void arachnid(machine_config &config);
	void galaxy(machine_config &config);

private:
	required_device<cpu_device> m_maincpu;
	required_device<pia6821_device> m_pia_u11;
	required_device<pia6821_device> m_pia_u15;
	required_device<pia6821_device> m_pia_u17;
	required_device<speaker_sound_device> m_speaker;

	virtual void machine_start() override ATTR_COLD;
	uint8_t pia_u11_pa_r();
	uint8_t pia_u11_pb_r();
	int pia_u11_pca_r();
	int pia_u11_pcb_r();
	void pia_u11_pa_w(uint8_t data);
	void pia_u11_pb_w(uint8_t data);
	void pia_u15_pca_w(int state);
	void pia_u15_pcb_w(int state);

	uint8_t pia_u17_pa_r();
	void pia_u17_pb_w(uint8_t data);
	void pia_u17_pcb_w(int state);

	uint8_t read_keyboard(int pa);
	void arachnidgalaxy_map(address_map &map) ATTR_COLD;
};

/***************************************************************************
    MEMORY MAPS
***************************************************************************/

/*-------------------------------------------------
    ADDRESS_MAP( arachnidgalaxy_map )
-------------------------------------------------*/

void arachnidgalaxy_state::arachnidgalaxy_map(address_map &map)
{
	map(0x0000, 0x07ff).ram().share("nvram");
	map(0x2000, 0x2007).rw(PTM6840_TAG, FUNC(ptm6840_device::read), FUNC(ptm6840_device::write));
	map(0x4004, 0x4007).rw(m_pia_u4, FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0x4008, 0x400b).rw(m_pia_u17, FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0x6000, 0x6000).w(TMS9118_TAG, FUNC(tms9928a_device::vram_write));
	map(0x6002, 0x6002).w(TMS9118_TAG, FUNC(tms9928a_device::register_write));
	map(0x8000, 0xffff).bankr(m_program_rom_bank);
}

/***************************************************************************
    INPUT PORTS
***************************************************************************/

/*-------------------------------------------------
    INPUT_PORTS( arachnid )
-------------------------------------------------*/

static INPUT_PORTS_START( arachnid )
	PORT_START("PA0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') // SELECT
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W') // PLAYER
	PORT_BIT( 0xfd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 ) // COIN
	PORT_BIT( 0xfb, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T') // TEST
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-4")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-5")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-7")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_TOGGLE
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_TOGGLE
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	// Matrix Switch Part I
	PORT_START("PA1-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0xfd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0xfb, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-4")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0xef, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-5")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0xdf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-6")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0xbf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )

	// Matrix Switch Part II
	PORT_START("PB1-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB1-1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0xfd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB1-2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0xfb, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB1-3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB1-4")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0xef, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB1-5")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0xdf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB1-6")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0xbf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB1-7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/***************************************************************************
    DEVICE CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    ptm6840_interface ptm_intf
-------------------------------------------------*/

void arachnidgalaxy_state::ptm_o1_callback(int state)
{
	m_speaker->level_w(state);
}

uint8_t arachnidgalaxy_state::read_keyboard(int pa)
{
	int i;
	uint8_t value;
	static const char *const keynames[3][8] =
			{
				{ "PA0-0", "PA0-1", "PA0-2", "PA0-3", "PA0-4", "PA0-5", "PA0-6", "PA0-7" },
				{ "PA1-0", "PA1-1", "PA1-2", "PA1-3", "PA1-4", "PA1-5", "PA1-6", "PA1-7" },
				{ "PB1-0", "PB1-1", "PB1-2", "PB1-3", "PB1-4", "PB1-5", "PB1-6", "PB1-7" }
			};

	for (i = 0; i < 8; i++)
	{
		value = ioport(keynames[pa][i])->read();

		if (value != 0xff)
		{
			if (value == 0xff - (1 << i))
				return value;
			else
				return value - (1 << i);
		}
	}

	return 0xff;
}

uint8_t arachnidgalaxy_state::pia_u4_pa_r()
{
	// Pulses from Switch Matrix Part I
	// PA0 - G
	// PA1 - H
	// PA2 - E
	// PA3 - F
	// PA4 - C
	// PA5 - D
	// PA6 - A
	// PA7 - B

	uint8_t data = 0xff;
	data &= read_keyboard(1);

	return data;
}

uint8_t arachnidgalaxy_state::pia_u4_pb_r()
{
	// Pulses from Switch Matrix Part II
	// PB0 - J
	// PB1 - I
	// PB2 - L
	// PB3 - K
	// PB4 - N
	// PB5 - M
	// PB6 - P
	// PB7 - O

	uint8_t data = 0xff;
	data &= read_keyboard(2);

	return data;
}

int arachnidgalaxy_state::pia_u4_pca_r()
{
	// CA1 - SW1 Coin In (Coin Door)

	uint8_t data = 1;
	data &= ioport("SW1")->read();

	return data;
}

int arachnidgalaxy_state::pia_u4_pcb_r()
{
	// CB1 - SW2 Test Mode (Coin Door)

	uint8_t data = 1;
	data &= ioport("SW2")->read();

	return data;
}

uint8_t arachnidgalaxy_state::pia_u17_pa_r()
{
	// PA0 - Select
	// PA1 - Player Change
	// PA2 - Coin
	// PA3 - Test
	// PA4 thru PA7 - DIP SW1

	uint8_t data = 0xff;
	data &= read_keyboard(0);

	return data;
}

void arachnidgalaxy_state::pia_u4_pa_w(uint8_t data)
{
	// PA0 thru PA7 Pulses to Switch Matrix Part I
}

void arachnidgalaxy_state::pia_u4_pb_w(uint8_t data)
{
	// PA0 thru PA7 Pulses to Switch Matrix Part II
}

void arachnidgalaxy_state::pia_u4_pca_w(int state)
{
	// CA1 - Remove Darts Lamp
}

void arachnidgalaxy_state::pia_u4_pcb_w(int state)
{
	// CB2 - Throw Darts Lamp
}

void arachnidgalaxy_state::pia_u17_pb_w(uint8_t data)
{
	// PB0 - Select Lamp
	// PB1 - Player Change Lamp
	// PB2 - Not Used
	// PB3 - Not Used
	// PB4 - Not Used
	// PB5 - Not Used
	// PB6 - Not Used
	// PB7 - N/C
}

void arachnidgalaxy_state::pia_u17_pcb_w(int state)
{
	// CB2 - Target Lamp
}

/***************************************************************************
    MACHINE INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    MACHINE_START( arachnid )
-------------------------------------------------*/

void arachnidgalaxy_state::machine_start()
{
}

/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

/*-------------------------------------------------
    machine_config( arachnid )
-------------------------------------------------*/

void arachnidgalaxy_state::arachnid(machine_config &config)
{
	// basic machine hardware
	MC6809(config, m_maincpu, 10.738635_MHz_XTAL / 3);
	m_maincpu->set_addrmap(AS_PROGRAM, &arachnidgalaxy_state::arachnidgalaxy_map);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0); // MK48Z02 (or DS1220Y)

	// devices
	PIA6821(config, m_pia_u4);
	m_pia_u4->readpa_handler().set(FUNC(arachnidgalaxy_state::pia_u4_pa_r));
	m_pia_u4->readpb_handler().set(FUNC(arachnidgalaxy_state::pia_u4_pb_r));
	m_pia_u4->readca1_handler().set(FUNC(arachnidgalaxy_state::pia_u4_pca_r));
	m_pia_u4->readcb1_handler().set(FUNC(arachnidgalaxy_state::pia_u4_pcb_r));
	m_pia_u4->writepa_handler().set(FUNC(arachnidgalaxy_state::pia_u4_pa_w));
	m_pia_u4->writepb_handler().set(FUNC(arachnidgalaxy_state::pia_u4_pb_w));
	m_pia_u4->ca2_handler().set(FUNC(arachnidgalaxy_state::pia_u4_pca_w));
	m_pia_u4->cb2_handler().set(FUNC(arachnidgalaxy_state::pia_u4_pcb_w));

	PIA6821(config, m_pia_u17);
	m_pia_u17->readpa_handler().set(FUNC(arachnidgalaxy_state::pia_u17_pa_r));
	m_pia_u17->ca1_w(1); // CA1 - 1000 HZ Input
	m_pia_u17->writepb_handler().set(FUNC(arachnidgalaxy_state::pia_u17_pb_w));
	m_pia_u17->cb2_handler().set(FUNC(arachnidgalaxy_state::pia_u17_pcb_w));

	// video hardware
	tms9118_device &vdp(TMS9118(config, TMS9118_TAG, 10.738635_MHz_XTAL));
	vdp.set_screen("screen");
	vdp.set_vram_size(0x4000);
	vdp.int_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
	SCREEN(config, "screen", SCREEN_TYPE_RASTER);

	// sound hardware
	SPEAKER(config, "mono").front_center();
	SPEAKER_SOUND(config, m_speaker).add_route(ALL_OUTPUTS, "mono", 1.0);

	ptm6840_device &ptm(PTM6840(config, PTM6840_TAG, 10.738635_MHz_XTAL / 3 / 4));
	ptm.set_external_clocks(0, 0, 0);
	ptm.o1_callback().set(FUNC(arachnidgalaxy_state::ptm_o1_callback));
}

/***************************************************************************
    ROMS
***************************************************************************/

ROM_START( aracgal )
	ROM_REGION( 0x8000, M6809_TAG, 0 )
	ROM_LOAD( "galla.u15",                      0x0000, 0x40000, CRC(8bd53e96) SHA1(0f3357b1ac56ab08c6608474e6ae96910daa8fc0) )
ROM_END

} // anonymous namespace


/***************************************************************************
    SYSTEM DRIVERS
***************************************************************************/

//    YEAR  NAME        PARENT    MACHINE   INPUT     STATE           INIT        MONITOR  COMPANY     FULLNAME
GAME( 1996, aracgal,    0,		  arachnid, arachnid, arachnidgalaxy_state, empty_init, ROT0,    "Arachnid", "Galaxy (v6.00 German)",                                MACHINE_MECHANICAL | MACHINE_NOT_WORKING )
