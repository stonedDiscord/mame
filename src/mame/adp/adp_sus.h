// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina,David Haywood,stonedDiscord
#ifndef MAME_ADP_ADP_SUS_H
#define MAME_ADP_ADP_SUS_H

#pragma once

#include "cpu/m68000/m68000.h"
#include "machine/mc68681.h"
#include "machine/msm6242.h"
#include "machine/nvram.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "videocontroller.h"

class adp_steuereinheit_device : public device_t
{
public:
	adp_steuereinheit_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	auto input_callback() { return m_input_cb.bind(); }
	auto output_callback() { return m_output_cb.bind(); }
	auto shift_callback() { return m_shift_cb.bind(); }
	auto duart_output_callback() { return m_duart_output_cb.bind(); }
	auto duart_input_callback() { return m_duart_input_cb.bind(); }
	auto irq_callback() { return m_irq_cb.bind(); }
	auto serial_a_tx_callback() { return m_serial_a_tx_cb.bind(); }
	auto serial_b_tx_callback() { return m_serial_b_tx_cb.bind(); }

	void serial_a_rx_w(int state) { m_duart->rx_a_w(state); }
	void serial_b_rx_w(int state) { m_duart->rx_b_w(state); }

	u16 read(offs_t offset, u16 mem_mask = ~0);
	void write(offs_t offset, u16 data, u16 mem_mask = ~0);
	u8 irq_vector_r();
	mc68681_device &duart() const { return *m_duart; }
	ay8910_device &psg() const { return *m_psg; }

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;

private:
	void duart_output_w(u8 data);
	u8 duart_input_r();
	void serial_a_tx_w(int state) { m_serial_a_tx_cb(state); }
	void serial_b_tx_w(int state) { m_serial_b_tx_cb(state); }
	void irq_w(int state);
	required_device<mc68681_device> m_duart;
	required_device<ay8910_device> m_psg;
	required_device<ad7224_device> m_dac;
	optional_device_array<adp_videocontroller_device, 2> m_video;
	devcb_read16 m_input_cb;
	devcb_write16 m_output_cb;
	devcb_write8 m_shift_cb;
	devcb_write8 m_duart_output_cb;
	devcb_read8 m_duart_input_cb;
	devcb_write_line m_serial_a_tx_cb;
	devcb_write_line m_serial_b_tx_cb;
	devcb_write_line m_irq_cb;
};

class adp_sus_device : public device_t
{
public:
	adp_sus_device &set_skattva_nvram_init(bool enabled = true) { m_skattva_nvram_init = enabled; return *this; }
	m68000_device &cpu() const { return *m_maincpu; }
	nvram_device &nvram() const { return *m_nvram; }

protected:
	adp_sus_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock, bool rtc);
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

private:
	void nvram_init(nvram_device &nvram, void *base, size_t size);
	void program_map(address_map &map) ATTR_COLD;
	void cpu_space_map(address_map &map) ATTR_COLD;
	bool const m_has_rtc;
	bool m_skattva_nvram_init = false;
	required_device<m68000_device> m_maincpu;
	required_device<adp_steuereinheit_device> m_steuereinheit;
	required_device<nvram_device> m_nvram;
	required_shared_ptr<u16> m_nvram_data;
	optional_device<msm6242_device> m_rtc;
};

class adp_sus_tk_device : public adp_sus_device
{
public:
	adp_sus_tk_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);
};

class adp_sus_rtc_device : public adp_sus_device
{
public:
	adp_sus_rtc_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);
};

DECLARE_DEVICE_TYPE(ADP_STEUEREINHEIT, adp_steuereinheit_device)
DECLARE_DEVICE_TYPE(ADP_SUS_TK, adp_sus_tk_device)
DECLARE_DEVICE_TYPE(ADP_SUS_RTC, adp_sus_rtc_device)

#endif // MAME_ADP_ADP_SUS_H
