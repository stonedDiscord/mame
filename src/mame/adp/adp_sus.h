// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina,David Haywood,stonedDiscord
#ifndef MAME_ADP_ADP_SUS_H
#define MAME_ADP_ADP_SUS_H

#pragma once

#include "adp_steuereinheit.h"
#include "cpu/m68000/m68000.h"
#include "machine/msm6242.h"
#include "machine/nvram.h"

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

private:
	void nvram_init(nvram_device &nvram, void *base, size_t size);
	void program_map(address_map &map) ATTR_COLD;
	void cpu_space_map(address_map &map) ATTR_COLD;
	bool const m_has_rtc;
	bool m_skattva_nvram_init = false;
	required_device<m68000_device> m_maincpu;
	required_device<adp_steuereinheit_device> m_steuereinheit;
	required_device<nvram_device> m_nvram;
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

DECLARE_DEVICE_TYPE(ADP_SUS_TK, adp_sus_tk_device)
DECLARE_DEVICE_TYPE(ADP_SUS_RTC, adp_sus_rtc_device)

#endif // MAME_ADP_ADP_SUS_H
