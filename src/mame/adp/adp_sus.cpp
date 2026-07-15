// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina,David Haywood,stonedDiscord
/*
CPU Board:
----------
 ____________________________________________________________
 |           ______________  ______________     ___________ |
 | 74HC245N  | t1 i       |  |KM681000ALP7|     |+        | |
 | 74HC573   |____________|  |____________|     |  3V Bat | |
 |                                              |         | |
 |           ______________  ______________     |        -| |
 |           | t1 ii      |  |KM681000ALP7|     |_________| |
 |     |||   |____________|  |____________| |||             |
 |     |||   ___________                    |||  M62X42B    |
 | X   |||   |         |                    |||             |
 |     |||   |68EC000 8|  74HC32   74HC245  |||  MAX691CPE  |
 |     |||   |         |  74AC138  74HC573  |||    74HC32   |
 |           |         |                                    |
 | 74HC573   |_________|  74HC08   74HC10  74HC32  74HC21   |
 |__________________________________________________________|

Parts:

 68EC000FN8         - Motorola 68k CPU
 KM681000ALP7       - 128K X 8 Bit Low Power CMOS Static RAM
 OKIM62X42B         - Real-time Clock ic With Built-in Crystal
 MAX691CPE          - P Reset ic With Watchdog And Battery Switchover
 X                    - 8MHz xtal
 3V Bat             - Lithium 3V power module

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

 */

#include "emu.h"
#include "adp_sus.h"
#include "videocontroller.h"

#include "speaker.h"

DEFINE_DEVICE_TYPE(ADP_STEUEREINHEIT, adp_steuereinheit_device, "adp_steuereinheit", "ADP Steuereinheit")
DEFINE_DEVICE_TYPE(ADP_SUS_TK, adp_sus_tk_device, "adp_sus_tk", "ADP SuS CPU Board (Timekeeper)")
DEFINE_DEVICE_TYPE(ADP_SUS_RTC, adp_sus_rtc_device, "adp_sus_rtc", "ADP SuS CPU Board (SRAM/RTC)")

adp_steuereinheit_device::adp_steuereinheit_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, ADP_STEUEREINHEIT, tag, owner, clock),
	m_duart(*this, "duart"),
	m_psg(*this, "aysnd"),
	m_dac(*this, "dac"),
	m_video(*this, ":videocontroller%u", 1U),
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
	MC68681(config, m_duart, 3'686'400);
	m_duart->irq_cb().set(FUNC(adp_steuereinheit_device::irq_w));
	m_duart->outport_cb().set(FUNC(adp_steuereinheit_device::duart_output_w));
	m_duart->inport_cb().set(FUNC(adp_steuereinheit_device::duart_input_r));
	m_duart->a_tx_cb().set(FUNC(adp_steuereinheit_device::serial_a_tx_w));
	m_duart->b_tx_cb().set(FUNC(adp_steuereinheit_device::serial_b_tx_w));

	AD7224(config, m_dac, 0);

	SPEAKER(config, "mono").front_center();
	ym2149_device &psg(YM2149(config, m_psg, 3'686'400 / 2));
	psg.add_route(ALL_OUTPUTS, "mono", 0.85);
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
	u16 const address = offset << 1;
	switch (address & 0x1c0)
	{
		case 0x000: // Y0: DAC (write only)
		case 0x040: // Y1: unknown
		case 0x080:
		{
			unsigned const slot = BIT(offset, 3);
			return m_video[slot] ? m_video[slot]->read(offset & 7, mem_mask) : 0xffff;
		}
		case 0x0c0: // Y3: secondary output (write only)
			return 0xffff;
		case 0x100: return m_input_cb(0, mem_mask);
		case 0x140: return 0xff00 | m_psg->data_r();
		case 0x180: return 0xff00 | m_duart->read((address >> 1) & 0x0f);
		case 0x1c0: // Y7: not connected
		default:
			return 0xffff;
	}
}

void adp_steuereinheit_device::write(offs_t offset, u16 data, u16 mem_mask)
{
	if (!ACCESSING_BITS_0_7)
		return;

	u16 const address = offset << 1;
	u8 const value = data;
	switch (address & 0x1c0)
	{
		case 0x00: m_dac->data_w(value); break;
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

adp_sus_device::adp_sus_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock, bool rtc) :
	device_t(mconfig, type, tag, owner, clock),
	m_has_rtc(rtc),
	m_maincpu(*this, "maincpu"),
	m_steuereinheit(*this, ":steuereinheit"),
	m_nvram(*this, "nvram"),
	m_nvram_data(*this, "nvram"),
	m_rtc(*this, "rtc")
{
}

adp_sus_tk_device::adp_sus_tk_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	adp_sus_device(mconfig, ADP_SUS_TK, tag, owner, clock, false)
{
}

adp_sus_rtc_device::adp_sus_rtc_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	adp_sus_device(mconfig, ADP_SUS_RTC, tag, owner, clock, true)
{
}

void adp_sus_device::device_add_mconfig(machine_config &config)
{
	M68000(config, m_maincpu, m_has_rtc ? 12'000'000 : 8'000'000);
	m_maincpu->set_addrmap(AS_PROGRAM, &adp_sus_device::program_map);
	m_maincpu->set_addrmap(m68000_device::AS_CPU_SPACE, &adp_sus_device::cpu_space_map);

	NVRAM(config, m_nvram, nvram_device::DEFAULT_NONE);
	m_nvram->set_custom_handler(FUNC(adp_sus_device::nvram_init));
	if (m_has_rtc)
		MSM6242(config, m_rtc, XTAL(32'768));
}

void adp_sus_device::nvram_init(nvram_device &nvram, void *base, size_t size)
{
	if (!m_skattva_nvram_init)
		return;

	u16 *const ram = reinterpret_cast<u16 *>(base);
	ram[(0xffe450 - 0xfc0000) >> 1] = 0x2400;
	ram[(0xffe452 - 0xfc0000) >> 1] = 0x0018;
	ram[(0xffc000 - 0xfc0000) >> 1] = 0x3141;
	ram[(0xffc002 - 0xfc0000) >> 1] = 0x5926;
}

void adp_sus_device::device_start()
{
}

void adp_sus_device::device_reset()
{
}

void adp_sus_device::program_map(address_map &map)
{
	/*
	U13 74HC138 splits the address space into four regions
	0x000000 - 0x3fffff	- ROM (U2, U6)
	0x400000 - 0x7fffff	- RTC (U22)
	0x800000 - 0xbfffff	- IO (U1, U4, U11, U14, U16)
	0xc00000 - 0xffffff	- RAM (U3, U7)
	*/
	map(0x000000, 0x0fffff).rom().region(":maincpu", 0);
	if (m_has_rtc)
	{
		map(0x400000, 0x40001f).rw(m_rtc, FUNC(msm6242_device::read), FUNC(msm6242_device::write)).umask16(0x00ff);
		map(0xfc0000, 0xffffff).ram().share("nvram");
	}
	else
	{
		map(0xff0000, 0xffffff).ram().share("nvram");
	}
	map(0x800000, 0xbfffff).rw(m_steuereinheit, FUNC(adp_steuereinheit_device::read), FUNC(adp_steuereinheit_device::write));
}

void adp_sus_device::cpu_space_map(address_map &map)
{
	map(0xfffff9, 0xfffff9).r(m_steuereinheit, FUNC(adp_steuereinheit_device::irq_vector_r));
}
