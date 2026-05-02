// license: BSD-3-Clause
// copyright-holders:

#ifndef MAME_VIDEO_NORITAKE_VFD_H
#define MAME_VIDEO_NORITAKE_VFD_H

#pragma once

#include "emu.h"

DECLARE_DEVICE_TYPE(NORITAKE_VFD, noritake_vfd_device)

#define NORITAKE_VFD_PIXEL_UPDATE(name) void name(bitmap_ind16 &bitmap, u8 line, u8 pos, u8 y, u8 x, int state)

class noritake_vfd_device : public device_t
{
public:
	noritake_vfd_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);

	// device interface
	void write(offs_t offset, u8 data);
	u8 read(offs_t offset);
	void control_w(u8 data) { write(0, data); }
	u8 control_r() { return read(0); }
	void data_w(u8 data) { write(1, data); }
	u8 data_r() { return read(1); }

	void db_w(u8 data);
	void rs_w(int state);
	void rw_w(int state);
	void e_w(int state);

	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void set_lcd_size(int lines, int chars) { m_lines = lines; m_chars = chars; }
	template <typename... T> void set_pixel_update_cb(T &&... args) { m_pixel_update_cb.set(std::forward<T>(args)...); }
	void set_busy_factor(float f) { m_busy_factor = f; }

	typedef device_delegate<void (bitmap_ind16 &bitmap, u8 line, u8 pos, u8 y, u8 x, int state)> pixel_update_delegate;

protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_clock_changed() override;
	virtual void device_validity_check(validity_checker &valid) const override;

private:
	TIMER_CALLBACK_MEMBER(clear_busy_flag);
	TIMER_CALLBACK_MEMBER(blink_tick);

	// command/data processing
	void control_write(u8 data);
	u8 control_read();
	void data_write(u8 data);
	u8 data_read();

	// internal helpers
	void set_busy_flag(uint16_t cycles);
	void correct_ac();
	void update_ac(int direction);
	void pixel_update(bitmap_ind16 &bitmap, u8 line, u8 pos, u8 y, u8 x, int state);
	void render_char(int line, int pos, u8 *dest);
	void process_esc_sequence();

	// internal state
	emu_timer *m_busy_timer;
	emu_timer *m_blink_timer;

	u8 m_lines;
	u8 m_chars;
	pixel_update_delegate m_pixel_update_cb;
	float m_busy_factor;

	bool m_busy_flag;
	u8 m_ddram[40];      // 2 lines x 20 chars
	u8 m_cgram[32];      // 8 custom chars x 4 bytes (5x7 font, we use 4 bytes for 4x7 or similar)
	u8 m_cgram_full[64]; // full custom char storage (8 chars x 8 bytes)
	int m_ac;
	u8 m_dr;
	u8 m_ir;
	u8 m_active_ram;
	bool m_display_on;
	bool m_cursor_on;
	bool m_blink_on;
	int m_direction;
	u8 m_data_len;
	u8 m_num_line;
	u8 m_char_size;
	bool m_blink;
	bool m_first_cmd;
	int m_rs_input;
	int m_rw_input;
	u8 m_db_input;
	bool m_enabled;
	int m_rs_state;
	int m_rw_state;
	bool m_nibble;

	// Noritake-specific state
	u8 m_mode;           // 0=DC1 (normal), 1=DC2 (scroll)
	u8 m_cursor_mode;    // 0=DC3 (underline), 1=DC4 (block), 2=DC5 (off), 3=DC6 (blink)
	u8 m_font_select;    // current font (SUB/FS/GS/RS/US)
	bool m_esc_mode;
	u8 m_esc_step;
	u8 m_esc_data[7];
	u8 m_blink_pos;
	u8 m_brightness;

	u8 m_render_buf[80 * 16];
};

#endif // MAME_VIDEO_NORITAKE_VFD_H