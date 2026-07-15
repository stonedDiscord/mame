// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina,David Haywood,stonedDiscord
/*

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
 OO              - LEDs (red); "Fehlerdiagnose siehe Fehlertabelle"
					Fault diagnosis see fault table (specific to each game)

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

The board connects to a separate CPU module and up to two video boards.
*/

#include "emu.h"
#include "adp_steuereinheit.h"

#include "speaker.h"

DEFINE_DEVICE_TYPE(ADP_STEUEREINHEIT, adp_steuereinheit_device, "adp_steuereinheit", "ADP Steuereinheit")

adp_steuereinheit_device::adp_steuereinheit_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, ADP_STEUEREINHEIT, tag, owner, clock),
	m_duart(*this, "duart"),
	m_psg(*this, "aysnd"),
	m_dac(*this, "dac"),
	m_video(*this, ":videocontroller%u", 0U),
	m_input_cb(*this, 0xffff),
	m_output_cb(*this),
	m_shift_cb(*this),
	m_duart_output_cb(*this),
	m_duart_input_cb(*this, 0xff),
	m_serial_a_tx_cb(*this),
	m_serial_b_tx_cb(*this),
	m_irq_cb(*this)
{
}

void adp_steuereinheit_device::device_add_mconfig(machine_config &config)
{
	MC68681(config, m_duart, 3'686'400); // U15
	m_duart->irq_cb().set(FUNC(adp_steuereinheit_device::irq_w));
	m_duart->outport_cb().set(FUNC(adp_steuereinheit_device::duart_output_w));
	m_duart->inport_cb().set(FUNC(adp_steuereinheit_device::duart_input_r));
	m_duart->a_tx_cb().set(FUNC(adp_steuereinheit_device::serial_a_tx_w));
	m_duart->b_tx_cb().set(FUNC(adp_steuereinheit_device::serial_b_tx_w));

	SPEAKER(config, "mono").front_center();

	AD7224(config, m_dac, 0); // U8
	m_dac->add_route(ALL_OUTPUTS, "mono", 0.85); // R10 15k

	YM2149(config, m_psg, 3'686'400 / 2); // U9
	m_psg->add_route(ALL_OUTPUTS, "mono", 0.90); // R8 10k
}

u8 adp_steuereinheit_device::irq_vector_r()
{
	return m_duart->get_irq_vector();
}

void adp_steuereinheit_device::device_start()
{
}

void adp_steuereinheit_device::duart_output_w(u8 data)
{
	m_duart_output_cb(data);
}

u8 adp_steuereinheit_device::duart_input_r()
{
	return m_duart_input_cb();
}

void adp_steuereinheit_device::irq_w(int state)
{
	m_irq_cb(state);
}

u16 adp_steuereinheit_device::read(offs_t offset, u16 mem_mask)
{
	// U17 74HC138
	u16 const address = offset << 1;
	switch (address & 0x1c0)
	{
	case 0x080:
	{
		unsigned const slot = BIT(offset, 3);
		return m_video[slot] ? m_video[slot]->read(offset & 7, mem_mask) : 0xffff;
	}
	case 0x100: return m_input_cb(0, mem_mask);
	case 0x140: return 0xff00 | m_psg->data_r();
	case 0x180: return 0xff00 | m_duart->read((address >> 1) & 0x0f);
	case 0x000: // Y0: DAC (write only)
	case 0x040: // Y1: unknown
	case 0x0c0: // Y3: secondary output (write only)
	case 0x1c0: // Y7: not connected
	default:
		return 0xffff;
	}
}

void adp_steuereinheit_device::write(offs_t offset, u16 data, u16 mem_mask)
{
	// U17 74HC138
	if (!ACCESSING_BITS_0_7)
		return;

	u16 const address = offset << 1;
	u8 const value = data;
	switch (address & 0x1c0)
	{
	case 0x000: m_dac->data_w(value); break;
	case 0x040: break; // Y1: unknown
	case 0x080:
	{
		unsigned const slot = BIT(offset, 3);
		if (m_video[slot])
			m_video[slot]->write(offset & 7, data, mem_mask);
		break;
	}
	case 0x0c0: m_shift_cb(value); break;
	case 0x100: m_output_cb(value); break;
	case 0x140:
		m_psg->address_data_w((address >> 1) & 1, value);
		break;
	case 0x180: m_duart->write((address >> 1) & 0x0f, value); break;
	case 0x1c0: break; // Y7: not connected
	default:
		break;
	}
}
