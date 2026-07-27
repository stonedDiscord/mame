// license:BSD-3-Clause
// copyright-holders:

/*
   Skeleton driver for Blaupunkt CDS 32-ID terminal with colour screen.
   More hardware inforation and photos:  https://www.oldcomputers.es/terminal-blaupunkt-cds-32-id/

   Hardware setup:

 Printer PCB (silkscreened as "BLAUPUNKT  BTX-DRUCKER-INTERFACE  TYP 8668 305 585")
    ____________________________________________________________________________
   |                    ___________   ___________   ___________   ___________  |
 __|__                 |CONN_MAIN_|  |SN74ALS174N  |SN74LS164N|  |SN74ALS161BN |
|     | ___________   ___________    ___________    ___________                |
|DB15 ||SN74LS367AN  |SN74LS02N_|   |SN74LS244N|   |SN74LS374N|                |
| FEM |               __________________________    ________________           |
|     | ___________  | Motorola MC6821P        |   | NEC D4016C-3  |           |
|     ||SN74LS367AN  |                         |   |               |           |
|_____|              |_________________________|   |_______________|           |
   |        Xtal      __________________________    ___________   ___________  |
   |_     400 kHz    | Motorola MC6802P        |   |SN74LS161AN  |SN74AS04N_|  |
   - |               |                         |    ___________   ___________  |
   -_|<- Dips x 4    |_________________________|   |SN74LS161AN  |SN74AS02N_|  |
   |    ___________   ________________              ___________   ___________  |
   |   |_MC14069U_|  | EPROM         |             |SN74LS161AN  |SN74LS00N_|  |
   |                 |_______________|                                         |
   |___________________________________________________________________________|

Main PCB
   _____________________________________________________________________________
  |  _______________   _______________                            ___________  |
  | | NEC D4016C-3 |  | NEC D4016C-3 |                           |PC74HCT367P  |
  | |______________|  |______________|                             __________  |
  |  ___________       ___________                                CONN PRINTER |
  | |CD74HCT365E      |CD74HCT373E         Xtal 6 MHz                          |
  |  ___________       ___________   ____________________                      |
  | |CD74HCT365E      |CD74HCT373E  | M4613D/A          |                      |
  |  ____________      ___________  | 4782 DUG8629      |                      |
  | |CD74HCT373E|     |CD74HCT365E  |___________________|       _____________  |
  |  ___________       ___________   ___   ______________      | ASTEC       | |
  | |CD74HCT86E|      |_N82S153N_|  |  |  | NMC9816AN-25|      | AD1D12A10   | |
  |  _____________________   Xtal   |  |  |_____________|      |_____________| |
  | | MAB 8031AH 12P     | 11.0592  |_<-CD74HCT245E______                      |
  | |                    |   MHz          | D4016C-3    |                      |
  | |____________________|                |_____________|                      |
  |                                        ______________                      |
  |                                       | EPROM       |                      |
  |                                       |_____________|                      |
  |                                                                            |
  |____                                                                        |
      |                   -----------------------------------------            |
      |                   -------------- V24 INTERFACE ------------            |
      |_______________________      ___      ___      ___      ________________|
                              |____|   |____|   |____|   |____|
                             Keyboard   Tape      CVS     Modem

V24 Interface riser PCB
   ____________________________________________________________
  |  ___________   ___________    ___________    ___________  |
  | |_SN75189N_|  |_SN75188N_|   |PC74HCT00P|   |PC74HCT157P  |
  |_____________________                                      |
                        |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|

Keyboard PCB
  ___________________________________________________________________________________________
 |  _____________   ___________   ___________________   ____________                        |
 | |CD74HCT4514E|  |_74LS244N_|  | M5L8039P-11      |  | EPROM     |   ___________          |
 | |____________|    Xtal        |__________________|  |___________|  |PC74HCT373P          |
 |________________ 9.216 MHz _______________________________________________________________|

*/

#include "emu.h"

#include "cpu/m6800/m6800.h"
#include "cpu/mcs48/mcs48.h"
#include "cpu/mcs51/i8051.h"

#include "machine/6821pia.h"
#include "video/saa5350.h"

#include "screen.h"

namespace {

class blaucds32_state : public driver_device
{
public:
	blaucds32_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_kbdc(*this, "kbdc"),
		m_crtc(*this, "crtc"),
		m_screen(*this, "screen"),
		m_printercpu(*this, "printercpu"),
		m_pia(*this, "pia"),
		m_keys(*this, "ROW%u", 0U)
	{ }

	void blaucds32(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

	void main_program_map(address_map &map) ATTR_COLD;
	void main_data_map(address_map &map) ATTR_COLD;
	void printer_program_map(address_map &map) ATTR_COLD;
	void keyboard_program_map(address_map &map) ATTR_COLD;
	void keyboard_io_map(address_map &map) ATTR_COLD;
	void main_p1_w(u8 data);
	u8 main_p3_r();
	u8 keyboard_p1_r();
	void keyboard_p1_w(u8 data);
	void keyboard_latch_w(u8 data);

	required_device<mcs51_cpu_device> m_maincpu;
	required_device<i8039_device> m_kbdc;
	required_device<saa5350_device> m_crtc;
	required_device<screen_device> m_screen;
	required_device<m6802_cpu_device> m_printercpu;
	required_device<pia6821_device> m_pia;
	required_ioport_array<10> m_keys;

	u8 m_keyboard_latch = 0xff;
};

void blaucds32_state::machine_start()
{
	save_item(NAME(m_keyboard_latch));
}

void blaucds32_state::machine_reset()
{
	m_keyboard_latch = 0xff;
	m_maincpu->set_input_line(MCS51_INT0_LINE, CLEAR_LINE);
	m_kbdc->set_input_line(MCS48_INPUT_IRQ, CLEAR_LINE);
}

void blaucds32_state::main_program_map(address_map &map)
{
	map(0x0000, 0xffff).rom().region("maincpu", 0);
}

void blaucds32_state::main_data_map(address_map &map)
{
	map(0x0000, 0x0fff).ram();
	map(0xd000, 0xe7ff).ram();
	map(0xfe00, 0xfe3f).rw(m_crtc, FUNC(saa5350_device::read), FUNC(saa5350_device::write));
}

void blaucds32_state::printer_program_map(address_map &map)
{
	map(0x1000, 0x1003).rw("pia", FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0x2000, 0x27ff).ram();
	map(0x3000, 0x3fff).mirror(0xc000).rom().region("printercpu", 0);
}

void blaucds32_state::keyboard_program_map(address_map &map)
{
	map(0x000, 0x7ff).rom().region("keyboard", 0);
}

void blaucds32_state::keyboard_io_map(address_map &map)
{
	map(0x00, 0xff).nopr().w(FUNC(blaucds32_state::keyboard_latch_w));
}

void blaucds32_state::main_p1_w(u8 data)
{
	// P1.6 is the active-low request line from the host to the keyboard MCU.
	m_kbdc->set_input_line(MCS48_INPUT_IRQ, BIT(data, 6) ? CLEAR_LINE : ASSERT_LINE);
}

u8 blaucds32_state::main_p3_r()
{
	// T0 is field timing; T1 is high while the M4613's open-drain BR is inactive.
	return 0xef | (m_screen->vblank() ? 0x10 : 0x00);
}

u8 blaucds32_state::keyboard_p1_r()
{
	u8 const row = m_kbdc->p2_r() >> 4;
	u8 data = 0xc0;
	for (unsigned column = 0; column < 5; column++)
	{
		unsigned const key = row + 16 * column;
		data |= BIT(m_keys[key >> 3]->read(), key & 0x07) << column;
	}
	return data;
}

void blaucds32_state::keyboard_p1_w(u8 data)
{
	// The keyboard's low-idle P1.7 output is inverted before reaching host INT0.
	m_maincpu->set_input_line(MCS51_INT0_LINE, BIT(data, 7) ? ASSERT_LINE : CLEAR_LINE);
}

void blaucds32_state::keyboard_latch_w(u8 data)
{
	m_keyboard_latch = data;
}

static INPUT_PORTS_START( blaucds32 )
	PORT_START("ROW0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('+') PORT_CHAR('*')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')

	PORT_START("ROW1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')

	PORT_START("ROW2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1) PORT_CHAR('#') PORT_CHAR('^')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')

	PORT_START("ROW3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("ROW4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(10)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')

	PORT_START("ROW5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')

	PORT_START("ROW6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC) PORT_NAME("Stop")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LALT) PORT_NAME("Alt")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')

	PORT_START("ROW7")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_HOME) PORT_NAME("Home")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_END) PORT_NAME("End")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CAPSLOCK) PORT_NAME("Caps Lock")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('<') PORT_CHAR('>')

	PORT_START("ROW8")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('`')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('~')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))

	PORT_START("ROW9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))

	// Printer interface config
	PORT_START("DSW1")
	PORT_DIPUNKNOWN_DIPLOC(0x01, 0x01, "SW1:1")
	PORT_DIPUNKNOWN_DIPLOC(0x02, 0x02, "SW1:2")
	PORT_DIPUNKNOWN_DIPLOC(0x04, 0x04, "SW1:3")
	PORT_DIPUNKNOWN_DIPLOC(0x08, 0x08, "SW1:4")
INPUT_PORTS_END

void blaucds32_state::blaucds32(machine_config &config)
{
	I8031(config, m_maincpu, 11.0592_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &blaucds32_state::main_program_map);
	m_maincpu->set_addrmap(AS_DATA, &blaucds32_state::main_data_map);
	m_maincpu->port_in_cb<1>().set_constant(0xff); // Unconnected V.24 and peripheral inputs idle high
	m_maincpu->port_out_cb<1>().set(FUNC(blaucds32_state::main_p1_w));
	m_maincpu->port_in_cb<3>().set(FUNC(blaucds32_state::main_p3_r));

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	SAA5350(config, m_crtc, 6_MHz_XTAL); // Siemens M4613D/A, compatible with SAA5350/SAA5351
	m_crtc->set_screen(m_screen);
	m_crtc->set_memory(m_maincpu, AS_DATA);

	// Printer interface
	M6802(config, m_printercpu, 400_kHz_XTAL).set_addrmap(AS_PROGRAM, &blaucds32_state::printer_program_map); // Motorola MC6802P
	PIA6821(config, m_pia); // Motorola MC6821P

	// Cherry 601-1415 keyboard (TODO: Convert to device)
	I8039(config, m_kbdc, 9.216_MHz_XTAL); // Mitsubishi M5L8039P-11
	m_kbdc->set_addrmap(AS_PROGRAM, &blaucds32_state::keyboard_program_map);
	m_kbdc->set_addrmap(AS_IO, &blaucds32_state::keyboard_io_map);
	m_kbdc->p1_in_cb().set(FUNC(blaucds32_state::keyboard_p1_r));
	m_kbdc->p1_out_cb().set(FUNC(blaucds32_state::keyboard_p1_w));
	m_kbdc->t0_in_cb().set_constant(0); // Select translated character codes instead of raw scan codes.
	m_kbdc->t1_in_cb().set_constant(0); // Select the serial host interface.

	config.set_perfect_quantum(m_maincpu);
}

ROM_START( blaucds32 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "v4292_b0147-00.v4292", 0x00000, 0x10000, CRC(a25d8966) SHA1(72298dd4f3d8bb333c545fec2683bcae4bb18e26) )

	ROM_REGION( 0x2000, "printercpu", 0 )
	ROM_LOAD( "v5270_b0141-01.v270",  0x00000, 0x02000, CRC(95943b74) SHA1(4554b796b75c6790d07dc4bf5278df00f00b6804) )

	ROM_REGION( 0x800, "keyboard", 0 )
	ROM_LOAD( "426.bin",              0x00000, 0x00800, CRC(0465f6a7) SHA1(14d9d9ae58baad2f7ccbf1f35ef8599e32ec1ed1) )

	ROM_REGION( 0x0eb, "prom", 0 )
	ROM_LOAD( "n82s153n.v4245",       0x00000, 0x000eb, NO_DUMP ) // On main PCB
ROM_END

} // anonymous namespace


//    YEAR  NAME       PARENT  COMPAT  MACHINE    INPUT      CLASS            INIT        COMPANY      FULLNAME     FLAGS
COMP( 19??, blaucds32, 0,      0,      blaucds32, blaucds32, blaucds32_state, empty_init, "Blaupunkt", "CDS 32-ID", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
