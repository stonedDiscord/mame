#include "emu.h"
#include "k1520.h"

#include "zre.h"

DEFINE_DEVICE_TYPE(K1520_ZRE, k1520_zre_k2521_device, "k1520_zre", "K1520 K2521 ZRE Board")

// K1520 K2521 (012-7100)

static const z80_daisy_config k2521_daisy_chain[] =
{
	{ "ctc" },
	{ nullptr }
};

k1520_zre_k2521_device::k1520_zre_k2521_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	k1520_zre_k2521_device(mconfig, K1520_ZRE, tag, owner, clock)
{
}

k1520_zre_k2521_device::k1520_zre_k2521_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_maincpu(*this, "maincpu"),
	m_ctc(*this, "ctc"),
	m_pio(*this, "pio"),
	m_rom(*this, "rom"),
	m_ram{ }
{
}

void k1520_zre_k2521_device::device_add_mconfig(machine_config &config)
{
	Z80(config, m_maincpu, XTAL(9'830'400) / 4);
	m_maincpu->set_addrmap(AS_PROGRAM, &k1520_zre_k2521_device::mem_map);
	m_maincpu->set_addrmap(AS_IO, &k1520_zre_k2521_device::io_map);
	m_maincpu->set_daisy_config(k2521_daisy_chain);

	Z80CTC(config, m_ctc, XTAL(9'830'400) / 4);
	m_ctc->intr_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
	m_ctc->set_clk<0>(XTAL(9'830'400) / 64);
	m_ctc->set_clk<1>(XTAL(9'830'400) / 64);
	m_ctc->set_clk<2>(XTAL(9'830'400) / 64);
	m_ctc->set_clk<3>(XTAL(9'830'400) / 64);

	Z80PIO(config, m_pio, XTAL(9'830'400) / 4);
}

void k1520_zre_k2521_device::device_start()
{
	save_item(NAME(m_ram));
}

void k1520_zre_k2521_device::irq_line_w(int state)
{
	m_maincpu->set_input_line(INPUT_LINE_IRQ0, state ? ASSERT_LINE : CLEAR_LINE);
}

void k1520_zre_k2521_device::nmi_line_w(int state)
{
	m_maincpu->set_input_line(INPUT_LINE_NMI, state ? ASSERT_LINE : CLEAR_LINE);
}

void k1520_zre_k2521_device::mem_map(address_map &map)
{
	map(0x0000, 0xffff).rw(FUNC(k1520_zre_k2521_device::bus_memory_r), FUNC(k1520_zre_k2521_device::bus_memory_w));
}

void k1520_zre_k2521_device::io_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0xff).rw(FUNC(k1520_zre_k2521_device::bus_io_r), FUNC(k1520_zre_k2521_device::bus_io_w));
	map(0x80, 0x83).rw(m_ctc, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
	map(0x84, 0x87).rw(m_pio, FUNC(z80pio_device::read), FUNC(z80pio_device::write));
}

u8 k1520_zre_k2521_device::bus_memory_r(offs_t offset)
{
	return m_bus->memory_r(offset);
}

void k1520_zre_k2521_device::bus_memory_w(offs_t offset, u8 data)
{
	m_bus->memory_w(offset, data);
}

u8 k1520_zre_k2521_device::bus_io_r(offs_t offset)
{
	return m_bus->io_r(offset);
}

void k1520_zre_k2521_device::bus_io_w(offs_t offset, u8 data)
{
	m_bus->io_w(offset, data);
}

bool k1520_zre_k2521_device::memory_r(offs_t offset, u8 &data)
{
	if (offset <= 0x0bff)
	{
		data = m_rom[offset];
		return true;
	}

	if (offset >= 0x0c00 && offset <= 0x0fff)
	{
		data = m_ram[offset & 0x3ff];
		return true;
	}

	return false;
}

bool k1520_zre_k2521_device::memory_w(offs_t offset, u8 data)
{
	if (offset < 0x0c00 || offset > 0x0fff)
		return false;

	m_ram[offset & 0x3ff] = data;
	return true;
}
