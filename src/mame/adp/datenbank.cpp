// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/*

ADP
German Fruit Machines / Gambling Machines

Machines using a "Datenbank" CPU Board


CPU Board:
Enclosed in a metal case and battery backed SRAM that will clear itself
if the case is openend or the light detectors detect light.
Patent: DE4135767C2
----------
Drawing is of a newer 1MB revision, the older 512kb revision has parts on both sides.
The differences are the AT90S1200 instead of the ATmega48V and the SRAM is split up into 4 chips.
 ______________________________________________________
 |                                                    |
 |        X    /\          ATMEGA48V  LS              |
 |            /  \                LED                 |
 |  |||      MC68331                    3V Bat   |||  |
 |  |||  2    \  /                               |||  |
 |  |||  4     \/        CS                      |||  |
 |  |||  5     S                R4543            |||  |
 |  |||   5    R   573 245              3V Bat   |||  |
 |        7    A                                      |
 |        3 LS M   573                                |
 |____________________________________________________|

Parts:
 245    - 74VHC245 Octal Bus Transceiver
 573    - 74VHC573 Octal D-Type Latch
 X      - 32KHz Oscillator
 3V Bat - CR2032 Battery
 SRAM   - K6X8016C3B-UF55 512Kx16 bit Static RAM
 CS     - Case sensor
 LS     - Light sensor (phototransistor)

Sound  and I/O board:
---------------------
"Steuereinheit 68000"
 _________________________________________________________________________________
 |                        TS271CN    74HC02                        ****  ****    |
 |*         74HC573      ________________                          P1    P2     *|
 |*                      | YM2149F      |                                       *|
 |*         74HC574  ||| |______________|   74HC393  74HC4015 ||| MX7224KN      *|
 |P3                 |||                                      |||              P6|
 |*         74HC245  ||| ________________   3.6864M  74HC125  ||| TL7705ACP     *|
 |*   L4974A         ||| |SCN68681C1N40 |                     |||               *|
 |*                  ||| |______________|   74HC32   74AC138  |||               *|
 |P7                 |||                                      |||              P8|
 |*                        TC428CPA                                             *|
 |*                                                                             *|
 |*    P11  P12    P13    P14       P15   P16   P17      P18   P19   P20  P21   *|
 |P9   **** *****  *****  ****  OO  ****  ****  *******  ****  ****  ***  *** P10|
 |_______________________________________________________________________________|

Parts:

 YM2149F         - Yamaha PSG
 SCN68681C1N40   - Dual Asynchronous Receiver/transmitter (DUART);
 TS271CN         - Programmable Low Power CMOS Single Op-amp
 MX7224KN        - Maxim CMOS 8-bit DAC with Output Amplifier
 TL7705ACP       - Supply Voltage Supervisor
 TC428CPA        - Dual CMOS High-speed Driver
 L4974A          - ST 3.5A Switching Regulator
 OO              - LEDs (red); "Fehlerdiagnose siehe Fehlertable"

Connectors:

 Two connectors to link with Video Board
 P1  - Türöffnungen [1-6]
 P2  - PSG In/Out [1-6]
 P3  - Lautsprecher [1-6]
 P6  - Service - Test Gerät [1-6]
 P7  - Maschine [1-8]
 P8  - Münzeinheit [1-8]
 P9  - Akzeptor [1-4]
 P10 - Fadenfoul [1-4]
 P11 - Netzteil [1-5]
 P12 - Serienplan [1-8]
 P13 - Serienplan 2 [1-8]
 P14 - Münzeinheit 2 [1-8]
 P15 - I2C-Bus [1-4]
 P16 - Kodierg. [1-4]
 P17 - TTL-Ein-/Ausgänge (PSG-Port) [1-10]
 P18 - RS485 Aus [1-2]
 P19 - RS485 Ein [1-2]
 P20 - Serielle-S. [1-5]
 P21 - Türschalter [1-4]

Protection:
The Atmega MCU on the CPU board handles protection, it
monitors the case and light sensors and clears the SRAM if tampering
is detected.
It also loads the initial bootloader from its internal EEPROM into the SRAM
over the BDM debug interface if the CPU is powered on with empty SRAM.
Patent: DE10142537A1
*/


#include "emu.h"

#include "cpu/m68000/m68000.h"
#include "machine/68340.h"
#include "machine/nvram.h"
#include "machine/rtc4543.h"
#include "sound/ay8910.h"
#include "sound/dac.h"

#include "speaker.h"

#include "stellafr.lh"

//#define VERBOSE 1
#include "logmacro.h"

namespace {

enum
{
	PORT_I_SDA,
	PORT_I_COIN,
	PORT_I_IP2, //bridged to IP5
	PORT_I_MISO,
	PORT_I_DOOR,
	PORT_I_IP5
};

enum
{
	PORT_O_ALARM,
	PORT_O_EN_COIN,
	PORT_O_EN_SPK,
	PORT_O_PROT_OD,
	PORT_O_SCL,
	PORT_O_LED0,
	PORT_O_SDA,
	PORT_O_SPZ
};

enum
{
	PORT_A_IO0,
	PORT_A_IO1,
	PORT_A_IO2,
	PORT_A_IO3,
	PORT_A_IO4,
	PORT_A_IO5,
	PORT_A_RESET,
	PORT_A_DOOR_SIN
};

enum
{
	PORT_B_IO0,
	PORT_B_IO1,
	PORT_B_DAC,
	PORT_B_COIN_AW,
	PORT_B_RS485_OUT_EN,
	PORT_B_RS485_IN_EN,
	PORT_B_DOOR_SCK,
	PORT_B_DOOR_SOUT
};

// outputs
enum
{
	U1_1MA, //shared with output to service keyboard
	U1_2MA,
	U1_ME,
	U1_D3OUT,
	U1_ANZ1,
	U1_MUX1,
	U1_ANZ2, //shared with output to coin unit 2
	U1_MUX2
};

enum
{
	U5_EN1ME, // shared machine1 and coin unit 1
	U5_EN2ME,
	U5_AW1,
	U5_AW2,
	U5_ENANZ1, //shared with output to service keyboard
	U5_ENMUX1, //share with muxma
	U5_ENANZ2, //shared with output to coin unit 2
	U5_ENMUX2
};

// inputs
enum
{
	U10_OUTLI1,
	U10_OUTEMP1,
	U10_OUTMA,
	U10_OUTST,
	U10_OUTT1,
	U10_OUTT2,
	U10_OUTEMP2,
	U10_OUTLI2
};

class datenbank_state : public driver_device
{
public:
	datenbank_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_duart(*this, "duart"),
		m_nvram(*this, "nvram"),
        m_rtc(*this, "rtc"),
		m_dac(*this, "dac"),
		m_digits(*this, "digit%u", 0U),
		m_lamps(*this, "lamp%u%u", 0U),
		m_leds(*this, "led%u", 0U),
		m_in0(*this, "IN0")
	{ }

	void showdownec1(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<mc68681_device> m_duart;
	required_device<nvram_device> m_nvram;
    required_device<rtc4543_device> m_rtc;
	required_device<ad7224_device> m_dac;
	output_finder<8> m_digits;
	output_finder<16,12> m_lamps;
	output_finder<2> m_leds;
	required_ioport m_in0;

	uint16_t m_out_ma1;
	uint16_t m_out_ma2;
	uint16_t m_out_me;
	uint16_t m_out_data3;
	uint8_t m_out_anz1;
	uint16_t m_out_mux1;
	uint8_t m_out_anz2;
	uint16_t m_out_mux2;
	uint8_t m_out_aw1;
	uint8_t m_out_aw2;

	uint8_t m_in_li1;
	uint8_t m_in_emp1;
	uint8_t m_in_ma;
	uint8_t m_in_st;
	uint8_t m_in_t1;
	uint8_t m_in_t2;
	uint8_t m_in_emp2;
	uint8_t m_in_li2;

	uint8_t mux_r();
	void anz_w(bool second, uint8_t data);
	void aw_w(bool second, bool data);
	void mux_w(uint8_t data);
	void mux2_w(uint8_t data);
	void st_w(uint16_t data);
	void duart_output_w(uint8_t data);
	void ay8910_portb_w(uint8_t data);
	void lamp_w(bool second, uint8_t row, uint16_t data);

	void mem_map(address_map &map) ATTR_COLD;
	void fc7_map(address_map &map) ATTR_COLD;

};


void datenbank_state::anz_w(bool second, uint8_t data)
{
	LOG("ANZ %02x\n",data);
}

void datenbank_state::aw_w(bool second, bool data)
{
	if (second)
		m_out_aw2 = (m_out_aw2 << 1) | data;
	else
		m_out_aw1 = (m_out_aw1 << 1) | data;
}

void datenbank_state::lamp_w(bool second, uint8_t row, uint16_t data)
{
	if (row > 7)
		return; // inhibit flag set
	uint8_t lrow = (row & 0x07) + (second ? 8 : 0);
	for (int i = 0; i < 12; i++)
	{
		bool lamp_value = BIT(data, i);
		m_lamps[lrow][i] = lamp_value;
	}
}

void datenbank_state::st_w(uint16_t data)
{
	LOG("ST %04x\n",data);
}

void datenbank_state::mux_w(uint8_t data)
{
	bool enme1  = BIT(data,U5_EN1ME);
	bool enme2  = BIT(data,U5_EN2ME);
	bool aw1    = BIT(data,U5_AW1);
	bool aw2    = BIT(data,U5_AW2);
	bool enanz1 = BIT(data,U5_ENANZ1); //enable 7seg
	bool enmux1 = BIT(data,U5_ENMUX1); //enable lamps/buttons
	bool enanz2 = BIT(data,U5_ENANZ2);
	bool enmux2 = BIT(data,U5_ENMUX2);

	if (enme1)
	{
		LOG("1MA  %04x\n",m_out_ma1);
		LOG("ME   %04x\n",m_out_me);
		m_in_emp1 = 0x00;
		m_in_li1  = 0x00;
	}

	if (enme2)
	{
		LOG("2MA  %04x\n",m_out_ma2);
		m_in_emp2 = 0x00;
		m_in_li2  = 0x00;
	}

	aw_w(false, aw1);
	aw_w(true, aw2);

	if (enanz1) 
	{
		anz_w(false, m_out_anz1);
		st_w(m_out_ma1);
		m_in_st = 0x00;
	}

	if (enmux1)
	{
		lamp_w(false, (m_out_mux1 >> 12), m_out_mux1 & 0x0FFF);
		m_in_li1 = 0x00;
	}

	if (enanz2)
		anz_w(true, m_out_anz2);

	if (enmux2)
	{
		lamp_w(true, (m_out_mux2 >> 12), m_out_mux2 & 0x0FFF);
		m_in_li2 = 0x00;
	}
}

uint8_t datenbank_state::mux_r()
{
	uint8_t data = 0x00;

	if (BIT(m_in_li1, 0))   data |= (1 << U10_OUTLI1);
	if (BIT(m_in_emp1, 0))  data |= (1 << U10_OUTEMP1);
	if (BIT(m_in_ma, 0))    data |= (1 << U10_OUTMA);
	if (BIT(m_in_st, 0))    data |= (1 << U10_OUTST);
	if (BIT(m_in_t1, 0))    data |= (1 << U10_OUTT1);
	if (BIT(m_in_t2, 0))    data |= (1 << U10_OUTT2);
	if (BIT(m_in_emp2, 0))  data |= (1 << U10_OUTEMP2);
	if (BIT(m_in_li2, 0))   data |= (1 << U10_OUTLI2);

	m_in_li1 = m_in_li1 >> 1;
	m_in_emp1 = m_in_emp1 >> 1;
	m_in_ma = m_in_ma >> 1;
	m_in_st = m_in_st >> 1;
	m_in_t1 = m_in_t1 >> 1;
	m_in_t2 = m_in_t2 >> 1;
	m_in_emp2 = m_in_emp2 >> 1;
	m_in_li2 = m_in_li2 >> 1;

	return data;
}

void datenbank_state::mux2_w(uint8_t data)
{
	// anz goes into one 74hc4094
	// mux has 2 chained for lamp cols 0 - 11, 3 bits for lz encoded and EnSDAp

	m_out_ma1   = (m_out_ma1   << 1) | BIT(data,U1_1MA); //double
	m_out_ma2   = (m_out_ma2   << 1) | BIT(data,U1_2MA);
	m_out_me    = (m_out_me    << 1) | BIT(data,U1_ME);
	m_out_data3 = (m_out_data3 << 1) | BIT(data,U1_D3OUT);
	m_out_anz1  = (m_out_anz1  << 1) | BIT(data,U1_ANZ1);
	m_out_mux1  = (m_out_mux1  << 1) | BIT(data,U1_MUX1);
	m_out_anz2  = (m_out_anz2  << 1) | BIT(data,U1_ANZ2); //double
	m_out_mux2  = (m_out_mux2  << 1) | BIT(data,U1_MUX2);
}

void datenbank_state::duart_output_w(uint8_t data)
{
	m_leds[0] = !BIT(data, PORT_O_LED0);
	m_leds[1] = !BIT(data, PORT_O_SDA);
}

void datenbank_state::ay8910_portb_w(uint8_t data)
{
}

void datenbank_state::mem_map(address_map &map)
{
	map(0x000000, 0x7fffff).ram().share("nvram");
	// controlled by U17 74HC138
	map(0x800001, 0x800001).w(m_dac, FUNC(dac_byte_interface::data_w)); // Y0
	// Y1 device on cpu board
	// Y2 device on cpu board
	map(0x8000c1, 0x8000c1).w(FUNC(datenbank_state::mux2_w)); // Y3 SP/ME II out
	map(0x800100, 0x800101).rw(FUNC(datenbank_state::mux_r), FUNC(datenbank_state::mux_w)); // Y4 SP/ME I out / Inputs
	map(0x800141, 0x800141).rw("aysnd", FUNC(ay8910_device::data_r), FUNC(ay8910_device::address_w)); // Y5
	map(0x800143, 0x800143).w("aysnd", FUNC(ay8910_device::data_w)); // Y5
	map(0x800180, 0x80019f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write)).umask16(0x00ff); // Y6
	// Y7 NC
}

void datenbank_state::fc7_map(address_map &map)
{
	map(0xfffff5, 0xfffff5).r(m_duart, FUNC(mc68681_device::get_irq_vector));
}

void datenbank_state::machine_start()
{
	m_digits.resolve();
	m_lamps.resolve();
	m_leds.resolve();
}

void datenbank_state::machine_reset()
{
	m_out_mux1 = 0;
}

static INPUT_PORTS_START( showdownec1 )
	PORT_START("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_GAMBLE_HIGH ) // Left
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_START )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_SLOT_STOP1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SLOT_STOP2 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_GAMBLE_LOW ) // Right
INPUT_PORTS_END


void datenbank_state::showdownec1(machine_config &config)
{
	M68340(config, m_maincpu, 16000000); //MC68331
	m_maincpu->set_addrmap(AS_PROGRAM, &datenbank_state::mem_map);
	m_maincpu->set_addrmap(m68000_device::AS_CPU_SPACE, &datenbank_state::fc7_map);

	RTC4543(config, m_rtc, 32.768_kHz_XTAL);

	MC68681(config, m_duart, 3686400);
	m_duart->irq_cb().set_inputline(m_maincpu, M68K_IRQ_2); // ?
	m_duart->outport_cb().set(FUNC(datenbank_state::duart_output_w));

	NVRAM(config, m_nvram, nvram_device::DEFAULT_NONE);

	AD7224(config, m_dac, 0);

	SPEAKER(config, "mono").front_center();
	ay8910_device &aysnd(AY8910(config, "aysnd", 1000000));
	aysnd.add_route(ALL_OUTPUTS, "mono", 0.85);
	aysnd.port_a_read_callback().set_ioport("IN0");
	aysnd.port_b_write_callback().set(FUNC(datenbank_state::ay8910_portb_w));
}

ROM_START( showdownec1 )
    ROM_REGION16_BE( 0x80000, "nvram", 0 )
	ROM_LOAD( "loader_rote.bin", 0x00400, 0x00bc0, CRC(6f6a4f49) SHA1(fd2ec05d52aeea588edcf6e22c7f6bc6dfb8d0d1) )
	ROM_LOAD( "eeprom_1mb_at90s1200.bin", 0x00fc0, 0x00040, CRC(900fa426) SHA1(386b562b827665273fbc251f7c212651fff8c315) )
	ROM_LOAD( "showdown_ec1_decrypted.bin", 0x01000, 0x50c04, CRC(39f72304) SHA1(a4c383f83a8c455c59fd16af3608119b1fab4f5b) )
ROM_END

ROM_START( brisant )
    ROM_REGION16_BE( 0x80000, "nvram", 0 )
	ROM_LOAD( "loader_rote.bin", 0x00400, 0x00bc0, CRC(6f6a4f49) SHA1(fd2ec05d52aeea588edcf6e22c7f6bc6dfb8d0d1) )
	ROM_LOAD( "eeprom_1mb_at90s1200.bin", 0x00fc0, 0x00040, CRC(900fa426) SHA1(386b562b827665273fbc251f7c212651fff8c315) )
	ROM_LOAD( "brisant_ec1.xc.dec.bin",     0x000000, 0x052404, CRC(83b81f46) SHA1(5c83bf81f285cac8a918dc5fdcb270a57588a1b9) )
ROM_END

ROM_START( siriusje )
    ROM_REGION16_BE( 0x80000, "nvram", 0 )
	ROM_LOAD( "loader_rote.bin", 0x00400, 0x00bc0, CRC(6f6a4f49) SHA1(fd2ec05d52aeea588edcf6e22c7f6bc6dfb8d0d1) )
	ROM_LOAD( "eeprom_1mb_at90s1200.bin", 0x00fc0, 0x00040, CRC(900fa426) SHA1(386b562b827665273fbc251f7c212651fff8c315) )
	ROM_LOAD( "sirius_jackpot_ext_c2.xc.dec.bin",   0x000000, 0x018804, CRC(010a61e5) SHA1(1117ad3d97f0a08c4111ac52e9bf4edea0b0bac5) )
ROM_END

} // anonymous namespace

GAMEL(1998, showdownec1,             0, showdownec1, showdownec1, datenbank_state, empty_init, ROT0, "Mega",   "Showdown",           MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_stellafr )
GAMEL(1999, brisant,             0, showdownec1, showdownec1, datenbank_state, empty_init, ROT0, "Mega",   "Brisant",           MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_stellafr )
GAMEL(2006, siriusje,             0, showdownec1, showdownec1, datenbank_state, empty_init, ROT0, "Mega",   "Sirius Jackpot Nug",           MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_stellafr )
