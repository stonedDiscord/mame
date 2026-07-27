// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/**********************************************************************

    Philips/Signetics SAA5350 Single-Chip Color CRT Controller

**********************************************************************/

#ifndef MAME_VIDEO_SAA5350_H
#define MAME_VIDEO_SAA5350_H

#pragma once

class saa5350_device : public device_t, public device_video_interface
{
public:
	saa5350_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	template <typename T> void set_memory(T &&tag, int spacenum, offs_t base = 0)
	{
		m_memory.set_tag(std::forward<T>(tag), spacenum);
		m_memory_base = base;
	}

	u8 read(offs_t offset);
	void write(offs_t offset, u8 data);
	u32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

protected:
	virtual void device_config_complete() override;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;

private:
	u16 character_pattern(u8 code, unsigned line) const;

	required_address_space m_memory;
	required_region_ptr<u8> m_char_rom;
	offs_t m_memory_base = 0;
	u8 m_registers[0x40];
};

DECLARE_DEVICE_TYPE(SAA5350, saa5350_device)

#endif // MAME_VIDEO_SAA5350_H
