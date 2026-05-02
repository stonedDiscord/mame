// license: BSD-3-Clause
// copyright-holders:
/***************************************************************************
 *
 *   Noritake CU20026SCPB VFD display device
 *
 *   2x20 character VFD display with custom command set
 *   Not HD44780 compatible - uses Noritake-specific protocol
 *
 ***************************************************************************/

#include "emu.h"
#include "noritake_vfd.h"

#define VERBOSE 1
#include "logmacro.h"

DEFINE_DEVICE_TYPE(NORITAKE_VFD, noritake_vfd_device, "noritake_vfd", "Noritake CU20026SCPB VFD")

//**************************************************************************
//  CONSTANTS
//**************************************************************************

// Command codes
#define CMD_SET_CURSOR    0x00  // 00XX XXXX: Set cursor position
#define CMD_SOFT_RESET    0x50  // Software reset
#define CMD_READ_CURSOR   0x51  // Read cursor position

// Data write control codes (sent as data, not commands)
#define CTL_BS            0x08  // Back Space
#define CTL_HT            0x09  // Horizontal Tab
#define CTL_LF            0x0A  // Line Feed
#define CTL_CR            0x0D  // Carriage Return
#define CTL_FF            0x0C  // Form Feed - Clear display
#define CTL_DC1           0x11  // Normal mode (cursor auto-increment)
#define CTL_DC2           0x12  // Scroll mode
#define CTL_DC3           0x13  // Cursor on (underline)
#define CTL_DC4           0x14  // Block cursor mode
#define CTL_DC5           0x15  // Cursor off
#define CTL_DC6           0x16  // Cursor blink (2-byte: position)
#define CTL_DC7           0x17  // Blink off
#define CTL_SUB           0x1A  // Brightness control (2-byte) - English font is default
#define CTL_BRIGHTNESS    0x1A  // Brightness control (2-byte) - same as SUB
#define CTL_FS            0x1C  // Danish font
#define CTL_GS            0x1D  // General European font
#define CTL_RS            0x1E  // Swedish font
#define CTL_US            0x1F  // German font
#define CTL_ESC           0x1B  // Escape - define custom char

// Brightness control values (sent after 0x1A)
#define BRIGHT_100        0x00
#define BRIGHT_75         0x10
#define BRIGHT_50         0x20
#define BRIGHT_25         0x30

// Timing (in microseconds)
#define DELAY_CLEAR       560   // Clear display
#define DELAY_RESET       150000 // Hardware/soft reset (150ms)
#define DELAY_CURSOR      40    // Set cursor position
#define DELAY_NORMAL      120   // Normal data write
#define DELAY_MODE        100   // Mode change
#define DELAY_BLINK       70    // Blink position set
#define DELAY_HSCROLL     90    // Horizontal scroll write

// CGRAM layout - only 2 custom characters can be defined at a time (rotating buffer)
#define CGRAM_USER_CHARS  2
#define CGRAM_BYTES_PER   8

// Blink positions that can be set to blink (per datasheet)
#define BLINK_POS_0       0
#define BLINK_POS_19      19
#define BLINK_POS_20      20
#define BLINK_POS_39      39

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  noritake_vfd_device - constructor
//-------------------------------------------------

noritake_vfd_device::noritake_vfd_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, NORITAKE_VFD, tag, owner, clock)
	, m_lines(2)
	, m_chars(20)
	, m_pixel_update_cb(*this)
	, m_busy_factor(1.0)
	, m_busy_flag(false)
	, m_ac(0)
	, m_dr(0)
	, m_ir(0)
	, m_active_ram(0)
	, m_display_on(false)
	, m_cursor_on(false)
	, m_blink_on(false)
	, m_direction(1)
	, m_data_len(8)
	, m_num_line(2)
	, m_char_size(8)
	, m_blink(false)
	, m_first_cmd(true)
	, m_rs_input(0)
	, m_rw_input(0)
	, m_db_input(0)
	, m_enabled(false)
	, m_rs_state(0)
	, m_rw_state(0)
	, m_nibble(false)
	, m_mode(0)
	, m_cursor_mode(0)
	, m_font_select(0x1A)
	, m_esc_mode(false)
	, m_esc_step(0)
	, m_blink_pos(0)
	, m_brightness(0)
	, m_blink_char_0(-1)
	, m_blink_char_1(-1)
	, m_esc_2byte_cmd(false)
	, m_2byte_cmd_pending(false)
	, m_2byte_cmd(0)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void noritake_vfd_device::device_start()
{
	m_busy_timer = timer_alloc(FUNC(noritake_vfd_device::clear_busy_flag), this);
	m_blink_timer = timer_alloc(FUNC(noritake_vfd_device::blink_tick), this);

	// state saving
	save_item(NAME(m_busy_factor));
	save_item(NAME(m_busy_flag));
	save_item(NAME(m_ac));
	save_item(NAME(m_dr));
	save_item(NAME(m_ir));
	save_item(NAME(m_active_ram));
	save_item(NAME(m_display_on));
	save_item(NAME(m_cursor_on));
	save_item(NAME(m_blink_on));
	save_item(NAME(m_direction));
	save_item(NAME(m_data_len));
	save_item(NAME(m_num_line));
	save_item(NAME(m_char_size));
	save_item(NAME(m_blink));
	save_item(NAME(m_ddram));
	save_item(NAME(m_cgram));
	save_item(NAME(m_cgram_full));
	save_item(NAME(m_nibble));
	save_item(NAME(m_rs_input));
	save_item(NAME(m_rw_input));
	save_item(NAME(m_db_input));
	save_item(NAME(m_enabled));
	save_item(NAME(m_rs_state));
	save_item(NAME(m_rw_state));
	save_item(NAME(m_mode));
	save_item(NAME(m_cursor_mode));
	save_item(NAME(m_font_select));
	save_item(NAME(m_esc_mode));
	save_item(NAME(m_esc_step));
	save_item(NAME(m_esc_data));
	save_item(NAME(m_blink_pos));
	save_item(NAME(m_brightness));
	save_item(NAME(m_blink_char_0));
	save_item(NAME(m_blink_char_1));
	save_item(NAME(m_esc_2byte_cmd));
	save_item(NAME(m_2byte_cmd_pending));
	save_item(NAME(m_2byte_cmd));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void noritake_vfd_device::device_reset()
{
	memset(m_ddram, 0x20, sizeof(m_ddram)); // filled with SPACE char
	memset(m_cgram, 0, sizeof(m_cgram));
	memset(m_cgram_full, 0, sizeof(m_cgram_full));

	m_ac = 0;
	m_dr = 0;
	m_ir = 0;
	m_active_ram = 0;
	m_display_on = true;
	m_cursor_on = true;
	m_blink_on = false;
	m_direction = 1;
	m_data_len = 8;
	m_num_line = 2;
	m_char_size = 8;
	m_blink = false;
	m_first_cmd = true;
	m_rs_state = 0;
	m_rw_state = 0;
	m_nibble = false;

	// Noritake-specific defaults
	m_mode = 0;           // DC1 normal mode
	m_cursor_mode = 0;    // DC3 underline cursor
	m_font_select = 0x1A; // SUB English font
	m_esc_mode = false;
	m_esc_step = 0;
	memset(m_esc_data, 0, sizeof(m_esc_data));
	m_blink_pos = 0;
	m_brightness = 0;
	m_blink_char_0 = -1;
	m_blink_char_1 = -1;
	m_esc_2byte_cmd = false;
	m_2byte_cmd_pending = false;
	m_2byte_cmd = 0;

	m_pixel_update_cb.resolve();

	set_busy_flag(u16(DELAY_RESET));
}

//-------------------------------------------------
//  device_clock_changed
//-------------------------------------------------

void noritake_vfd_device::device_clock_changed()
{
	// blink happens every 102400 cycles
	attotime period = attotime::from_ticks(102400, clock());
	attotime remain = m_blink_timer->remaining();
	m_blink_timer->adjust((remain > period) ? period : remain, 0, period);
}

//-------------------------------------------------
//  device_validity_check
//-------------------------------------------------

void noritake_vfd_device::device_validity_check(validity_checker &valid) const
{
	if (clock() == 0)
		osd_printf_error("Noritake VFD clock cannot be zero!\n");
}

//**************************************************************************
//  TIMER EVENTS
//**************************************************************************

TIMER_CALLBACK_MEMBER(noritake_vfd_device::clear_busy_flag)
{
	m_busy_flag = false;
}

TIMER_CALLBACK_MEMBER(noritake_vfd_device::blink_tick)
{
	m_blink = !m_blink;
}

//**************************************************************************
//  INTERNAL HELPERS
//**************************************************************************

void noritake_vfd_device::set_busy_flag(uint16_t cycles)
{
	m_busy_flag = true;
	m_busy_timer->adjust(attotime::from_ticks(cycles, clock() / m_busy_factor));
}

void noritake_vfd_device::correct_ac()
{
	if (m_ac < 0)
		m_ac = 0;
	else if (m_ac >= m_chars * m_num_line)
		m_ac = m_chars * m_num_line - 1;
}

void noritake_vfd_device::update_ac(int direction)
{
	m_ac += direction;
	correct_ac();
}

void noritake_vfd_device::pixel_update(bitmap_ind16 &bitmap, u8 line, u8 pos, u8 y, u8 x, int state)
{
	if (!m_pixel_update_cb.isnull())
	{
		m_pixel_update_cb(bitmap, line, pos, y, x, state);
	}
	else
	{
		u8 line_height = m_char_size;

		if (m_lines <= 2)
		{
			if (pos < m_chars)
				bitmap.pix(line * (line_height + 1) + y, pos * 6 + x) = state;
		}
		else if (m_lines <= 4)
		{
			if (pos < m_chars*2)
			{
				if (pos >= m_chars)
				{
					line += 2;
					pos -= m_chars;
				}

				if (line < m_lines)
					bitmap.pix(line * (line_height + 1) + y, pos * 6 + x) = state;
			}
		}
		else
		{
			fatalerror("%s: use a custom callback for this LCD configuration (%d x %d)\n", tag(), m_lines, m_chars);
		}
	}
}

void noritake_vfd_device::render_char(int line, int pos, u8 *dest)
{
	uint16_t char_pos = line * m_chars + pos;
	u8 ch = m_ddram[char_pos];

	const u8 *src = nullptr;

	// Check CGRAM first (custom characters) - only 2 user-definable chars (0-1)
	if (ch < CGRAM_USER_CHARS && m_cgram_full[ch * CGRAM_BYTES_PER] != 0)
	{
		src = m_cgram_full + ch * CGRAM_BYTES_PER;
	}
	else
	{
		// Use CGROM (built-in font) - not implemented, render as blank
		// In a real implementation, this would use the actual font ROM
		memset(dest, 0, m_char_size);
		
		// Draw cursor if at cursor position
		if (char_pos == m_ac && m_cursor_on && m_cursor_mode != 2) // not DC5 (cursor off)
		{
			if (m_cursor_mode == 3) // DC6 blink (corner blink, not cursor)
			{
				// For corner blink mode, only blink if this position is a blink position
				if (is_blink_position(char_pos) && m_blink)
					memset(dest, 0x1f, m_char_size);
			}
			else
			{
				// underline or block cursor
				dest[m_char_size - 1] = 0x1f;
			}
		}
		return;
	}

	if (src)
		memcpy(dest, src, m_char_size);
	else
		memset(dest, 0, m_char_size);

	// Draw cursor
	if (char_pos == m_ac && m_cursor_on && m_cursor_mode != 2) // not DC5 (cursor off)
	{
		if (m_cursor_mode == 3) // DC6 blink
		{
			// Blink mode: only blink if this position is a designated blink position
			if (is_blink_position(char_pos) && m_blink)
				memset(dest, 0x1f, m_char_size);
		}
		else
		{
			// underline or block cursor
			dest[m_char_size - 1] = 0x1f;
		}
	}
}

void noritake_vfd_device::process_esc_sequence()
{
	if (m_esc_step < 6)
		return;

	// ESC sequence: 1B, char_code, D0, D1, D2, D3, D4
	// Defines a custom character
	// Only 2 custom characters can be stored at a time (rotating buffer)
	u8 char_code = m_esc_data[1] & 0x7f;
	if (char_code >= CGRAM_USER_CHARS)
	{
		LOG("Noritake VFD: ESC - char code %d out of range (only 0-1 supported)\n", char_code);
		m_esc_mode = false;
		m_esc_step = 0;
		m_esc_2byte_cmd = false;
		return;
	}

	// Determine which buffer slot to use (0 or 1)
	// If redefining same character, keep same slot (but datasheet says this has a bug)
	// If defining new character, use the other slot (rotating buffer)
	int slot = -1;
	if (char_code == m_blink_char_0 || char_code == m_blink_char_1)
	{
		// Character already has a buffer assigned - use same slot
		// But per datasheet bug: redefining overridden char doesn't update to new buffer
		// We'll track which slot each char uses
		if (char_code == m_blink_char_0)
			slot = 0;
		else
			slot = 1;
		LOG("Noritake VFD: ESC - redefining char %d (using slot %d)\n", char_code, slot);
	}
	else
	{
		// New character - use the oldest slot (rotate)
		// Simple rotation: if slot 0 is older, use it, otherwise use slot 1
		// We track by blink_char assignment order
		if (m_blink_char_0 == -1 || (m_blink_char_1 == -1 && m_blink_char_0 != char_code))
		{
			slot = 0;
			m_blink_char_0 = char_code;
		}
		else
		{
			slot = 1;
			m_blink_char_1 = char_code;
		}
		LOG("Noritake VFD: ESC - defining char %d (using slot %d)\n", char_code, slot);
	}

	// Pack 5-bit pixel data into 8-byte format
	// Bits from datasheet:
	// Byte 1: H G F E D C B A -> pixels (row 0-4, bits 4-0)
	// Byte 2: P O N M L K J I -> pixels (row 5-7, bits 4-0) + row 0-4 bit 5
	// Byte 3: X W V U T S R Q -> pixels (row 0-4, bit 5) + row 5-7 bit 5
	// Byte 4: 6 5 4 3 2 1 Z Y -> row 0-4 bit 6, row 5-7 bit 6
	// Byte 5: . . . . a 9 8 7 -> row 0-4 bit 7, row 5-7 bit 7
	//
	// Pixel layout in display (5 wide, 7 tall):
	// A C E G I  (col 0-4, row 0)
	// K M O Q S  (col 0-4, row 1)  
	// U W Y 1 3  (col 0-4, row 2)
	// 5 7 9 B D  (col 0-4, row 3)
	// F H J L N  (col 0-4, row 4)
	// R T V X Z  (col 0-4, row 5)
	// 2 4 6 8 a  (col 0-4, row 6)
	// P P P P P  (col 0-4, row 7 - appears to be padding/always 0)
	
	u8 char_data[8] = {0};
	
	// Extract 5-bit values from each data byte
	u8 bits[5];
	for (int i = 0; i < 5; i++)
	{
		bits[i] = m_esc_data[2 + i] & 0x1f;  // D0-D4 are the 5 LSBs
	}
	
	// Build 8 rows of 5-bit pixel data
	// Row 0: bits[0] D0 (A=bit0, C=bit1, E=bit2, G=bit3, I=bit4)
	char_data[0] = bits[0] & 0x1f;
	// Row 1: bits[1] D1 (K=bit0, M=bit1, O=bit2, Q=bit3, S=bit4)
	char_data[1] = bits[1] & 0x1f;
	// Row 2: bits[2] D2 (U=bit0, W=bit1, Y=bit2, 1=bit3, 3=bit4)
	char_data[2] = bits[2] & 0x1f;
	// Row 3: bits[3] D3 (5=bit0, 7=bit1, 9=bit2, B=bit3, D=bit4)
	char_data[3] = bits[3] & 0x1f;
	// Row 4: bits[4] D4 (F=bit0, H=bit1, J=bit2, L=bit3, N=bit4)
	char_data[4] = bits[4] & 0x1f;
	// Row 5: bits[0] D0 bit5, bits[1] D1 bit5, bits[2] D2 bit5, bits[3] D3 bit5, bits[4] D4 bit5
	// Actually row 5 is R T V X Z - from datasheet byte 3 bits
	char_data[5] = ((m_esc_data[2] >> 5) & 1) | (((m_esc_data[3] >> 5) & 1) << 1) | 
	               (((m_esc_data[4] >> 5) & 1) << 2) | (((m_esc_data[5] >> 5) & 1) << 3) |
	               (((m_esc_data[6] >> 5) & 1) << 4);
	// Row 6: bits from byte 4 (6 5 4 3 2 1 Z Y)
	char_data[6] = ((m_esc_data[2] >> 6) & 1) | (((m_esc_data[3] >> 6) & 1) << 1) |
	               (((m_esc_data[4] >> 6) & 1) << 2) | (((m_esc_data[5] >> 6) & 1) << 3) |
	               (((m_esc_data[6] >> 6) & 1) << 4);
	// Row 7: P P P P P - always 0 (padding)
	char_data[7] = 0;

	// Store in the appropriate slot (each slot is 8 bytes)
	for (int i = 0; i < 8; i++)
	{
		m_cgram_full[slot * CGRAM_BYTES_PER + i] = char_data[i];
	}
	
	// Also update the compact CGRAM (4 bytes per char, 5x7 = 35 bits in 4 bytes)
	// Pack 5 rows x 5 bits = 25 bits into first 3 bytes, remaining 10 bits into byte 4
	u32 packed = 0;
	for (int i = 0; i < 5; i++)
	{
		packed |= (u32)(char_data[i] & 0x1f) << (i * 5);
	}
	m_cgram[slot * 4 + 0] = packed & 0xff;
	m_cgram[slot * 4 + 1] = (packed >> 8) & 0xff;
	m_cgram[slot * 4 + 2] = (packed >> 16) & 0xff;
	m_cgram[slot * 4 + 3] = (packed >> 24) & 0xff;

	m_esc_mode = false;
	m_esc_step = 0;
	m_esc_2byte_cmd = false;
}

// Helper to check if a character position should be blinking
bool noritake_vfd_device::is_blink_position(u8 pos) const
{
	if (!m_blink_on)
		return false;
	return (pos == m_blink_char_0 || pos == m_blink_char_1);
}

//**************************************************************************
//  COMMAND/DATA PROCESSING
//**************************************************************************

void noritake_vfd_device::control_write(u8 data)
{
	m_ir = data;

	// Command: 00XX XXXX - Set cursor position
	if ((data & 0xc0) == 0x00)
	{
		m_ac = data & 0x3f;
		correct_ac();
		set_busy_flag(DELAY_CURSOR);
		LOG("Noritake VFD: Set cursor to %d\n", m_ac);
		return;
	}

	// Command: 0101 0000 - Software reset
	if (data == CMD_SOFT_RESET)
	{
		device_reset();
		set_busy_flag(u16(DELAY_RESET));
		LOG("Noritake VFD: Software reset\n");
		return;
	}

	// Command: 0101 0001 - Read cursor position
	if (data == CMD_READ_CURSOR)
	{
		set_busy_flag(DELAY_CURSOR);
		LOG("Noritake VFD: Read cursor position\n");
		return;
	}

	LOG("Noritake VFD: Unknown command 0x%02X\n", data);
}

u8 noritake_vfd_device::control_read()
{
	// Return busy flag and cursor position
	u8 result = (m_busy_flag ? 0x80 : 0) | (m_ac & 0x7f);
	return result;
}

void noritake_vfd_device::data_write(u8 data)
{
	m_dr = data;

	// Check for control codes (0x08-0x1F range)
	if (data >= 0x08 && data <= 0x1F)
	{
			switch (data)
			{
				case CTL_BS: // Back Space
					update_ac(-1);
					LOG("Noritake VFD: Back Space\n");
					break;

				case CTL_HT: // Horizontal Tab
					update_ac(1);
					LOG("Noritake VFD: Horizontal Tab\n");
					break;

				case CTL_LF: // Line Feed
					if (m_mode == 0) // DC1 normal mode
					{
						m_ac += m_chars;
						if (m_ac >= m_chars * m_num_line)
							m_ac -= m_chars * m_num_line;
					}
					else // DC2 scroll mode
					{
						if (m_ac >= m_chars * (m_num_line - 1))
						{
							// Scroll up: move top line to bottom
							memmove(m_ddram, m_ddram + m_chars, m_chars);
							memset(m_ddram + m_chars, 0x20, m_chars);
							m_ac = (m_num_line - 1) * m_chars;
						}
					}
					LOG("Noritake VFD: Line Feed\n");
					break;

				case CTL_CR: // Carriage Return
					m_ac -= (m_ac % m_chars);
					LOG("Noritake VFD: Carriage Return\n");
					break;

				case CTL_FF: // Form Feed - Clear display
					memset(m_ddram, 0x20, sizeof(m_ddram));
					LOG("Noritake VFD: Clear display (FF)\n");
					set_busy_flag(DELAY_CLEAR);
					return;

				case CTL_DC1: // Normal mode
					m_mode = 0;
					LOG("Noritake VFD: Normal mode (DC1)\n");
					break;

				case CTL_DC2: // Scroll mode
					m_mode = 1;
					LOG("Noritake VFD: Scroll mode (DC2)\n");
					break;

				case CTL_DC3: // Cursor on (underline)
					m_cursor_mode = 0;
					m_cursor_on = true;
					LOG("Noritake VFD: Cursor on - underline (DC3)\n");
					break;

				case CTL_DC4: // Block cursor mode
					m_cursor_mode = 1;
					m_cursor_on = true;
					LOG("Noritake VFD: Block cursor (DC4)\n");
					break;

				case CTL_DC5: // Cursor off
					m_cursor_mode = 2;
					m_cursor_on = false;
					LOG("Noritake VFD: Cursor off (DC5)\n");
					break;

				case CTL_DC6: // Cursor blink - 2 byte command: position to blink
					m_2byte_cmd_pending = true;
					m_2byte_cmd = CTL_DC6;
					m_cursor_mode = 3;
					m_cursor_on = true;
					m_blink_on = true;
					LOG("Noritake VFD: Cursor blink (DC6) - awaiting position byte\n");
					return;

				case CTL_DC7: // Blink off
					m_blink_on = false;
					m_blink_char_0 = -1;
					m_blink_char_1 = -1;
					LOG("Noritake VFD: Blink off (DC7)\n");
					break;

				case CTL_SUB: // Brightness control (2-byte command, 0x1A)
					m_2byte_cmd_pending = true;
					m_2byte_cmd = CTL_SUB;
					LOG("Noritake VFD: Brightness control - awaiting level byte\n");
					return;

				case CTL_ESC: // Escape - start custom char definition
					m_esc_mode = true;
					m_esc_step = 0;
					m_esc_2byte_cmd = true;
					LOG("Noritake VFD: ESC - custom char definition\n");
					return;

				default:
					LOG("Noritake VFD: Unknown control code 0x%02X\n", data);
					break;
			}

		set_busy_flag(DELAY_MODE);
		return;
	}

	// Handle 2-byte command second byte (brightness, blink position)
	if (m_2byte_cmd_pending)
	{
		m_2byte_cmd_pending = false;
		if (m_2byte_cmd == CTL_BRIGHTNESS)
		{
			// Brightness: 0, 16, 32, 48 (0x00, 0x10, 0x20, 0x30)
			m_brightness = data & 0x30;
			LOG("Noritake VFD: Brightness set to 0x%02X\n", m_brightness);
			set_busy_flag(DELAY_MODE);
			return;
		}
		else if (m_2byte_cmd == CTL_DC6)
		{
			// Blink position: only corners 0, 19, 20, 39
			u8 pos = data & 0x3f;
			if (pos == BLINK_POS_0 || pos == BLINK_POS_19 || pos == BLINK_POS_20 || pos == BLINK_POS_39)
			{
				// Track which character is assigned to which blink buffer
				// Only 2 blink positions can be active at once
				if (m_blink_char_0 == -1)
				{
					m_blink_char_0 = pos;
				}
				else if (m_blink_char_1 == -1 && pos != m_blink_char_0)
				{
					m_blink_char_1 = pos;
				}
				else
				{
					// Replace oldest (rotate)
					m_blink_char_0 = m_blink_char_1;
					m_blink_char_1 = pos;
				}
				LOG("Noritake VFD: Blink position set to %d\n", pos);
			}
			else
			{
				LOG("Noritake VFD: Invalid blink position %d (ignored, only 0,19,20,39)\n", pos);
			}
			set_busy_flag(DELAY_BLINK);
			return;
		}
	}

	// Handle ESC sequence data
	if (m_esc_mode)
	{
		if (m_esc_step < 7)
		{
			m_esc_data[m_esc_step] = data;
			m_esc_step++;
		}
		if (m_esc_step >= 6)
		{
			process_esc_sequence();
			set_busy_flag(DELAY_NORMAL);
		}
		return;
	}

	// Regular character write
	LOG("Noritake VFD: Write char '%c' (0x%02X) at %d\n", isprint(data) ? data : '.', data, m_ac);

	if (m_active_ram == 0) // DDRAM
	{
		if (m_ac >= 0 && m_ac < m_chars * m_num_line)
		{
			m_ddram[m_ac] = data;
		}
		update_ac(1);
	}
	else // CGRAM
	{
		if (m_ac >= 0 && m_ac < (int)sizeof(m_cgram))
		{
			m_cgram[m_ac] = data;
		}
		update_ac(1);
	}

	set_busy_flag(DELAY_NORMAL);
}

u8 noritake_vfd_device::data_read()
{
	u8 data = 0;

	if (m_active_ram == 0) // DDRAM
	{
		if (m_ac >= 0 && m_ac < m_chars * m_num_line)
			data = m_ddram[m_ac];
	}
	else // CGRAM
	{
		if (m_ac >= 0 && m_ac < (int)sizeof(m_cgram))
			data = m_cgram[m_ac];
	}

	LOG("Noritake VFD: Read 0x%02X from %sRAM at %d\n", data, m_active_ram ? "C" : "D", m_ac);

	if (!machine().side_effects_disabled())
	{
		update_ac(1);
		set_busy_flag(DELAY_NORMAL);
	}

	return data;
}

//**************************************************************************
//  DEVICE INTERFACE
//**************************************************************************

void noritake_vfd_device::write(offs_t offset, u8 data)
{
	switch (offset & 0x01)
	{
		case 0: control_write(data); break;
		case 1: data_write(data);    break;
	}
}

u8 noritake_vfd_device::read(offs_t offset)
{
	switch (offset & 0x01)
	{
		case 0: return control_read();
		case 1: return data_read();
	}
	return 0;
}

void noritake_vfd_device::db_w(u8 data)
{
	m_db_input = data;
}

void noritake_vfd_device::rs_w(int state)
{
	m_rs_input = state;
}

void noritake_vfd_device::rw_w(int state)
{
	m_rw_input = state;
}

void noritake_vfd_device::e_w(int state)
{
	if (m_data_len == 4 && state && !m_enabled && !machine().side_effects_disabled())
		update_nibble(m_rs_input, m_rw_input);

	if (!state && m_enabled && m_rw_input == 0)
	{
		switch (m_rs_input)
		{
			case 0: control_write(m_db_input);  break;
			case 1: data_write(m_db_input);     break;
		}
	}

	m_enabled = state;
}

void noritake_vfd_device::update_nibble(int rs, int rw)
{
	if (m_rs_state != rs || m_rw_state != rw)
	{
		m_rs_state = rs;
		m_rw_state = rw;
		m_nibble = false;
	}

	m_nibble = !m_nibble;
}

//**************************************************************************
//  SCREEN UPDATE
//**************************************************************************

uint32_t noritake_vfd_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0, cliprect);

	if (!m_display_on)
		return 0;

	for (int line = 0; line < m_num_line; line++)
	{
		for (int pos = 0; pos < m_chars; pos++)
		{
			u8 *dest = m_render_buf + 16 * (line * m_chars + pos);
			render_char(line, pos, dest);

			for (int y = 0; y < m_char_size; y++)
				for (int x = 0; x < 5; x++)
					pixel_update(bitmap, line, pos, y, x, BIT(dest[y], 4 - x));
		}
	}

	return 0;
}
