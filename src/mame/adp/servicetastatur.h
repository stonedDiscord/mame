// license:BSD-3-Clause
// copyright-holders:stonedDiscord
#ifndef MAME_ADP_SERVICETASTATUR_H
#define MAME_ADP_SERVICETASTATUR_H

#pragma once

#include "cpu/mcs51/i80c51.h"
#include "machine/i2cmem.h"
#include "video/hd44780.h"

class servicetastatur_device : public device_t
{
public:
	servicetastatur_device(machine_config const &mconfig, char const *tag, device_t *owner, uint32_t clock = 0);

	void input_w(uint16_t data) { m_input = data; }
	int output_r() const { return BIT(m_output, 7); }
	void clock_w() { m_output <<= 1; }
	void enable_w(int state);

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
	virtual tiny_rom_entry const *device_rom_region() const override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

private:
	uint8_t port1_r();
	void port1_w(uint8_t data);
	uint8_t port3_r();
	void port3_w(uint8_t data);
	uint8_t gsg_r_lower();
	uint8_t gsg_r_upper();
	void gsg_w(uint8_t data);
	void data_map(address_map &map) ATTR_COLD;
	void program_map(address_map &map) ATTR_COLD;

	required_device<mcs51_cpu_device> m_maincpu;
	required_device<i2cmem_device> m_i2cmem;
	required_device<hd44780_device> m_lcd;
	required_ioport_array<3> m_io_keys;
	uint8_t m_port1 = 0xff;
	uint8_t m_port3 = 0xff;
	uint16_t m_input = 0xffff;
	uint8_t m_output = 0xff;
};

DECLARE_DEVICE_TYPE(SERVICETASTATUR, servicetastatur_device)

#endif // MAME_ADP_SERVICETASTATUR_H
