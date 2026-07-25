// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina,stonedDiscord
#ifndef MAME_ADP_VIDEOCONTROLLER_H
#define MAME_ADP_VIDEOCONTROLLER_H

#pragma once

#include "video/hd63484.h"
#include "video/ramdac.h"
#include "emupal.h"
#include "screen.h"

class adp_videocontroller_device : public device_t
{
public:
	adp_videocontroller_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	adp_videocontroller_device &set_high_resolution(bool enabled = true);
	adp_videocontroller_device &set_gfx_region(const char *tag) { m_gfx_region = tag; return *this; }

	u16 read(offs_t offset, u16 mem_mask = ~0);
	virtual void write(offs_t offset, u16 data, u16 mem_mask = ~0);

protected:
	adp_videocontroller_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void palette_init(palette_device &palette) const;

	bool high_resolution() const { return m_high_resolution; }
	required_device<palette_device> &palette_finder() { return m_palette; }

private:
	void hd63484_map(address_map &map) ATTR_COLD;

	bool m_high_resolution = false;
	required_device<hd63484_device> m_acrtc;
	required_device<palette_device> m_palette;
	const char *m_gfx_region = ":gfx0";
};

class adp_videocontroller_encoder_device : public adp_videocontroller_device
{
public:
	adp_videocontroller_encoder_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	virtual void write(offs_t offset, u16 data, u16 mem_mask = ~0) override;

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void palette_init(palette_device &palette) const override;

private:
	void ramdac_map(address_map &map) ATTR_COLD;
	required_device<ramdac_device> m_ramdac;
};

DECLARE_DEVICE_TYPE(ADP_VIDEOCONTROLLER, adp_videocontroller_device)
DECLARE_DEVICE_TYPE(ADP_VIDEOCONTROLLER_ENCODER, adp_videocontroller_encoder_device)

#endif // MAME_ADP_VIDEOCONTROLLER_H
