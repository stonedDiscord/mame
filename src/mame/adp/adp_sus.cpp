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

 */

#include "emu.h"
#include "adp_sus.h"

DEFINE_DEVICE_TYPE(ADP_SUS_TK, adp_sus_tk_device, "adp_sus_tk", "ADP SuS CPU Board (Timekeeper)")
DEFINE_DEVICE_TYPE(ADP_SUS_RTC, adp_sus_rtc_device, "adp_sus_rtc", "ADP SuS CPU Board (SRAM/RTC)")

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
