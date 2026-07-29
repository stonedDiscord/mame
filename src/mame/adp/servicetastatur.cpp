// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/*

ADP
Profitech 3000 Servicetastatur

Hardware:
- CPU: 80C52 @ 11.0592MHz
- LCD: LCD4002A
- Memory: 27C256 EPROM (32KB), 24CS16 I2C EEPROM (2KB)

Key Matrix Layout:
Col 0 (P1.0): OK, F4, UP
Col 1 (P1.1): RIGHT, LEFT, DOWN
Col 2 (P1.2): F3, F1, F2

GSG pinout to machine:
GND
Data Out
Enable
Data Clock
Data In
5V

Output is done by 74HC165.
Input is done by the 2 74HC4094.
D7 is connected to QP0 and D0 to QP7.
U19 has D1-D3 reversed from this.

    _____________________________________
   | 11.059     24CS16           TL7705  |
___| XTAL  80C31          +KEYPAD+       |__
|74HC00                               +    |
|          74HC165 74HC4094           G    |
|27C128                               S    |
|74LS573   74HC238 74HC4094           G    |
|___   +DISPLAY+  MC34063             + ___|
   |___________________________________|
*/

#include "emu.h"
#include "servicetastatur.h"
#include "emupal.h"
#include "screen.h"

#define VERBOSE (1U)
#include "logmacro.h"


DEFINE_DEVICE_TYPE(SERVICETASTATUR, servicetastatur_device, "servicet", "ADP Profitech 3000 Servicetastatur")

enum
{
	PORT_1_COL0,
	PORT_1_COL1,
	PORT_1_COL2,
	PORT_1_NC3,
	PORT_1_ROW0,
	PORT_1_ROW1,
	PORT_1_ROW2,
	PORT_1_NC7
};

enum
{
	PORT_3_RXD,
	PORT_3_TXD,
	PORT_3_INT0,
	PORT_3_INT1,
	PORT_3_SDA,
	PORT_3_SCL,
	PORT_3_WR,
	PORT_3_RD
};

servicetastatur_device::servicetastatur_device(machine_config const &mconfig, char const *tag, device_t *owner, uint32_t clock) :
		device_t(mconfig, SERVICETASTATUR, tag, owner, clock),
		m_maincpu(*this, "maincpu"),
		m_i2cmem(*this, "eeprom"),
		m_lcd(*this, "hd44780"),
		m_io_keys(*this, "IN%u", 0U)
{
}

void servicetastatur_device::program_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
}

void servicetastatur_device::data_map(address_map &map)
{
	map(0x0010, 0x003f).nopw();
	map(0x0040, 0x004f).r(FUNC(servicetastatur_device::gsg_r_lower)); // U20
	map(0x0050, 0x005f).r(FUNC(servicetastatur_device::gsg_r_upper)); // U19
	map(0x0060, 0x006f).w(FUNC(servicetastatur_device::gsg_w));
	map(0x0070, 0x0070).w(m_lcd, FUNC(hd44780_device::control_w));
	map(0x0071, 0x0071).r(m_lcd, FUNC(hd44780_device::control_r));
	map(0x0072, 0x0072).w(m_lcd, FUNC(hd44780_device::data_w));
	map(0x0073, 0x0073).r(m_lcd, FUNC(hd44780_device::data_r));
	map(0x4000, 0x4000).nopw();
	map(0x8000, 0x8001).nopw();
}

static INPUT_PORTS_START( servicet )
	PORT_START("IN0") // P1.0
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("OK") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("IN1") // P1.1
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("IN2") // P1.2
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

ioport_constructor servicetastatur_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(servicet);
}

void servicetastatur_device::device_start()
{
	save_item(NAME(m_port1));
	save_item(NAME(m_port3));
	save_item(NAME(m_input));
	save_item(NAME(m_output));
}

void servicetastatur_device::device_reset()
{
	m_port1 = 0xff;
	m_port3 = 0xff;
	m_input = 0xffff;
	m_output = 0xff;
}

uint8_t servicetastatur_device::port1_r()
{
	uint8_t data = m_port1;
	for (int col = 0; col < 3; col++)
		if (!BIT(m_port1, col))
			data &= m_io_keys[col]->read();

	return data;
}

void servicetastatur_device::port1_w(uint8_t data)
{
	m_port1 = data;
}

uint8_t servicetastatur_device::port3_r()
{
	uint8_t data = m_port3;

	uint8_t const sda = m_i2cmem->read_sda();

	// Clear bit 4 (SDA) and insert actual value from EEPROM
	data = (data & ~(1 << PORT_3_SDA)) | (sda ? (1 << PORT_3_SDA) : 0);

	return data;
}

void servicetastatur_device::port3_w(uint8_t data)
{
	m_port3 = data;

	m_i2cmem->write_sda(BIT(data, PORT_3_SDA));
	m_i2cmem->write_scl(BIT(data, PORT_3_SCL));
}

uint8_t servicetastatur_device::gsg_r_lower()
{
	uint8_t const data = bitswap<8>(uint8_t(m_input), 0, 1, 2, 3, 4, 5, 6, 7);
	LOG("U20 %04x -> %02x\n", m_input, data);
	return data;
}

uint8_t servicetastatur_device::gsg_r_upper()
{
	uint8_t const data = bitswap<8>(uint8_t(m_input >> 8), 0, 3, 2, 1, 4, 5, 6, 7);
	LOG("U19 %04x -> %02x\n", m_input, data);
	return data;
}

void servicetastatur_device::gsg_w(uint8_t data)
{
	m_output = data;
	LOG("U13 <- %02x\n", data);
}

void servicetastatur_device::enable_w(int state)
{
	m_maincpu->set_input_line(MCS51_INT1_LINE, state ? ASSERT_LINE : CLEAR_LINE);
	m_maincpu->set_input_line(MCS51_INT0_LINE, state ? CLEAR_LINE : ASSERT_LINE);
	LOG("enable %d word %04x\n", state, m_input);
}

void servicetastatur_device::device_add_mconfig(machine_config &config)
{
	I80C31(config, m_maincpu, 11.0592_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &servicetastatur_device::program_map);
	m_maincpu->set_addrmap(AS_DATA, &servicetastatur_device::data_map);

	m_maincpu->port_in_cb<1>().set(FUNC(servicetastatur_device::port1_r));
	m_maincpu->port_out_cb<1>().set(FUNC(servicetastatur_device::port1_w));
	m_maincpu->port_in_cb<3>().set(FUNC(servicetastatur_device::port3_r));
	m_maincpu->port_out_cb<3>().set(FUNC(servicetastatur_device::port3_w));

	// I2C EEPROM: 24C16 (2KB) - connected to P3.4 (SDA) and P3.5 (SCL)
	I2C_24C16(config, m_i2cmem);

	// LCD4002A
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_LCD));
	screen.set_color(rgb_t(6, 120, 245));
	screen.set_physical_aspect(7*40, 10*2);
	screen.set_refresh_hz(72);
	screen.set_size(6*40, 9*2);
	screen.set_visarea_full();
	screen.set_screen_update("hd44780", FUNC(hd44780_device::screen_update));
	screen.set_palette("palette");

	PALETTE(config, "palette", palette_device::MONOCHROME_INVERTED);

	HD44780(config, m_lcd, 270'000);
	m_lcd->set_lcd_size(2, 40); // 2 lines, 40 characters
}

ROM_START( servicetastatur )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "service_tastatur_v3.3.u3", 0x0000, 0x8000, CRC(8eb161c4) SHA1(d44f3b38e75e1095487893d8b30c4e3212c1a143) )

	ROM_REGION(0x800, "eeprom", ROMREGION_ERASEFF)
ROM_END

tiny_rom_entry const *servicetastatur_device::device_rom_region() const
{
	return ROM_NAME(servicetastatur);
}

namespace {

class servicet_state : public driver_device
{
public:
	servicet_state(machine_config const &mconfig, device_type type, char const *tag) : driver_device(mconfig, type, tag) { }
	void servicet(machine_config &config) { SERVICETASTATUR(config, "keyboard", 0); }
};

ROM_START( servicet )
ROM_END

} // anonymous namespace

GAME( 1992, servicet, 0, servicet, servicet, servicet_state, empty_init, ROT0, "ADP", u8"Merkur Service Testgerät", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
