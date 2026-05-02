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

//**************************************************************************
//  CONSTANTS
//**************************************************************************

static const u8 vfd_font[128][7] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 00
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 01
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 02
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 03
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 04
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 05
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 06
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 07
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 08
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 09
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0A
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0B
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0C
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0D
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0E
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0F
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 10
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 11
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 12
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 13
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 14
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 15
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 16
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 17
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 18
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 19
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 1A
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 1B
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 1C
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 1D
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 1E
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 1F
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 20 (space)
	{ 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 }, // 21 !
	{ 0x0a, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00 }, // 22 "
	{ 0x0a, 0x0a, 0x1f, 0x0a, 0x1f, 0x0a, 0x0a }, // 23 #
	{ 0x04, 0x0f, 0x14, 0x0e, 0x05, 0x1e, 0x04 }, // 24 $
	{ 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03 }, // 25 %
	{ 0x0c, 0x12, 0x12, 0x0c, 0x15, 0x12, 0x0d }, // 26 &
	{ 0x0c, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00 }, // 27 '
	{ 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 }, // 28 (
	{ 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 }, // 29 )
	{ 0x04, 0x15, 0x0e, 0x1f, 0x0e, 0x15, 0x04 }, // 2a *
	{ 0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00 }, // 2b +
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x04 }, // 2c ,
	{ 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00 }, // 2d -
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c }, // 2e .
	{ 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00 }, // 2f /
	{ 0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e }, // 30 0
	{ 0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e }, // 31 1
	{ 0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f }, // 32 2
	{ 0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e }, // 33 3
	{ 0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02 }, // 34 4
	{ 0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e }, // 35 5
	{ 0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e }, // 36 6
	{ 0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }, // 37 7
	{ 0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e }, // 38 8
	{ 0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c }, // 39 9
	{ 0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00 }, // 3a :
	{ 0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x04, 0x08 }, // 3b ;
	{ 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02 }, // 3c <
	{ 0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00 }, // 3d =
	{ 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 }, // 3e >
	{ 0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 }, // 3f ?
	{ 0x0e, 0x11, 0x01, 0x0d, 0x15, 0x15, 0x0e }, // 40 @
	{ 0x04, 0x0a, 0x11, 0x11, 0x1f, 0x11, 0x11 }, // 41 A
	{ 0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e }, // 42 B
	{ 0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e }, // 43 C
	{ 0x1c, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1c }, // 44 D
	{ 0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f }, // 45 E
	{ 0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10 }, // 46 F
	{ 0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f }, // 47 G
	{ 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11 }, // 48 H
	{ 0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e }, // 49 I
	{ 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0c }, // 4a J
	{ 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }, // 4b K
	{ 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f }, // 4c L
	{ 0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11 }, // 4d M
	{ 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11 }, // 4e N
	{ 0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e }, // 4f O
	{ 0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10 }, // 50 P
	{ 0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d }, // 51 Q
	{ 0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11 }, // 52 R
	{ 0x0e, 0x11, 0x10, 0x0e, 0x01, 0x11, 0x0e }, // 53 S
	{ 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }, // 54 T
	{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e }, // 55 U
	{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04 }, // 56 V
	{ 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a }, // 57 W
	{ 0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11 }, // 58 X
	{ 0x11, 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04 }, // 59 Y
	{ 0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f }, // 5a Z
	{ 0x0e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0e }, // 5b [
	{ 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 }, // 5c \.
	{ 0x0e, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0e }, // 5d ]
	{ 0x04, 0x0a, 0x11, 0x00, 0x00, 0x00, 0x00 }, // 5e ^
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f }, // 5f _
	{ 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00 }, // 60 `
	{ 0x00, 0x00, 0x0e, 0x01, 0x0f, 0x11, 0x0f }, // 61 a
	{ 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1e }, // 62 b
	{ 0x00, 0x00, 0x0e, 0x10, 0x10, 0x11, 0x0e }, // 63 c
	{ 0x01, 0x01, 0x0d, 0x13, 0x11, 0x11, 0x0f }, // 64 d
	{ 0x00, 0x00, 0x0e, 0x11, 0x1f, 0x10, 0x0e }, // 65 e
	{ 0x06, 0x09, 0x08, 0x1c, 0x08, 0x08, 0x08 }, // 66 f
	{ 0x00, 0x0f, 0x11, 0x11, 0x0f, 0x01, 0x0e }, // 67 g
	{ 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11 }, // 68 h
	{ 0x04, 0x00, 0x0c, 0x04, 0x04, 0x04, 0x0e }, // 69 i
	{ 0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0c }, // 6a j
	{ 0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12 }, // 6b k
	{ 0x0c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e }, // 6c l
	{ 0x00, 0x00, 0x1a, 0x15, 0x15, 0x11, 0x11 }, // 6d m
	{ 0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11 }, // 6e n
	{ 0x00, 0x00, 0x0e, 0x11, 0x11, 0x11, 0x0e }, // 6f o
	{ 0x00, 0x00, 0x1e, 0x11, 0x1e, 0x10, 0x10 }, // 70 p
	{ 0x00, 0x00, 0x0d, 0x13, 0x0f, 0x01, 0x01 }, // 71 q
	{ 0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10 }, // 72 r
	{ 0x00, 0x00, 0x0f, 0x10, 0x0e, 0x01, 0x1e }, // 73 s
	{ 0x08, 0x08, 0x1c, 0x08, 0x08, 0x09, 0x06 }, // 74 t
	{ 0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0d }, // 75 u
	{ 0x00, 0x00, 0x11, 0x11, 0x11, 0x0a, 0x04 }, // 76 v
	{ 0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0a }, // 77 w
	{ 0x00, 0x00, 0x11, 0x0a, 0x04, 0x0a, 0x11 }, // 78 x
	{ 0x00, 0x00, 0x11, 0x11, 0x0f, 0x01, 0x0e }, // 79 y
	{ 0x00, 0x00, 0x1f, 0x02, 0x04, 0x08, 0x1f }, // 7a z
	{ 0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02 }, // 7b {
	{ 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }, // 7c |
	{ 0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08 }, // 7d }
	{ 0x00, 0x00, 0x04, 0x0a, 0x11, 0x00, 0x00 }, // 7e ~
	{ 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f }  // 7f 
};

// Command codes
#define CMD_SET_CURSOR    0x00  // 00XX XXXX: Set cursor position
#define CMD_SOFT_RESET    0x40  // Software reset (0x40 for CU20026, 0x50 for CU40026)
#define CMD_READ_CURSOR   0x51  // Read cursor position

// Data write control codes (sent as data, not commands)
#define CTL_BS            0x08  // Back Space
#define CTL_HT            0x09  // Horizontal Tab
#define CTL_LF            0x0A  // Line Feed
#define CTL_FF            0x0C  // Form Feed - Clear display
#define CTL_CR            0x0D  // Carriage Return
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
	, m_brightness(0)
	, m_esc_2byte_cmd(false)
	, m_2byte_cmd_pending(false)
	, m_2byte_cmd(0)
	, m_p40_prev(0)
	, m_vfd_data(0)
	, m_vfd_bits(0)
{
	m_next_slot = 0;
	m_esc_type = 0;
	m_esc_param = 0;
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
	save_item(NAME(m_cgram_char));
	save_item(NAME(m_next_slot));
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
	save_item(NAME(m_esc_2byte_cmd));
	save_item(NAME(m_2byte_cmd_pending));
	save_item(NAME(m_2byte_cmd));
	save_item(NAME(m_p40_prev));
	save_item(NAME(m_vfd_data));
	save_item(NAME(m_vfd_bits));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void noritake_vfd_device::device_reset()
{
	memset(m_ddram, 0x20, sizeof(m_ddram)); // filled with SPACE char
	memset(m_cgram, 0, sizeof(m_cgram));
	m_cgram_char[0] = m_cgram_char[1] = -1;
	m_next_slot = 0;

	m_ac = 0;
	m_dr = 0;
	m_ir = 0;
	m_active_ram = 0;
	m_display_on = true;
	m_cursor_on = true;
	m_blink_on = false;
	m_direction = 1;
	m_data_len = 8;
	m_num_line = m_lines;
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
	m_blink_pos[0] = m_blink_pos[1] = 0xff;
	m_brightness = 0;
	m_esc_2byte_cmd = false;
	m_2byte_cmd_pending = false;
	m_2byte_cmd = 0;

	m_p40_prev = 0;
	m_vfd_data = 0;
	m_vfd_bits = 0;

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
	int max_ac = m_chars * m_num_line;
	if (m_ac < 0)
		m_ac = max_ac + (m_ac % max_ac);
	else if (m_ac >= max_ac)
		m_ac %= max_ac;
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

	// Check CGRAM first (custom characters)
	if (ch == m_cgram_char[0])
		src = m_cgram[0];
	else if (ch == m_cgram_char[1])
		src = m_cgram[1];

	if (src)
	{
		memcpy(dest, src, 7);
		dest[7] = 0; // padding
	}
	else
	{
		// Use CGROM (built-in font)
		if (ch < 128)
		{
			memcpy(dest, vfd_font[ch], 7);
			dest[7] = 0;
		}
		else
		{
			memset(dest, 0, 8);
		}
	}

	// Draw cursor
	if (char_pos == m_ac && m_cursor_on)
	{
		switch (m_cursor_mode)
		{
		case 0: // DC3 underline
			dest[6] |= 0x1f;
			break;
		case 1: // DC4 block
			if (!m_blink)
				memset(dest, 0x1f, 7);
			break;
		case 2: // DC5 off
			break;
		case 3: // DC6 blink
			if (m_blink)
				dest[6] |= 0x1f;
			break;
		}
	}

	// Handle blinking characters
	if (m_blink_on && m_blink && is_blink_position(char_pos))
	{
		memset(dest, 0, 8);
	}
}

void noritake_vfd_device::process_esc_sequence()
{
	// ESC sequence: 1B, char_code, D0, D1, D2, D3, D4
	// Total 7 bytes. m_esc_step reaches 7 when finished.
	u8 char_code = m_esc_data[1];
	int slot = m_next_slot;

	// Double-buffer bug: if redefining an existing char, it doesn't update mapping
	// But it always writes to the next slot and toggles it.
	bool found = false;
	if (char_code == (u8)m_cgram_char[0] || char_code == (u8)m_cgram_char[1])
		found = true;

	if (!found)
		m_cgram_char[slot] = char_code;

	// Dot mapping from datasheet
	// BYTE 3 (m_esc_data[2]): D7=23 D6=15 D5=22 D4=16 D3=21 D2=17 D1=20 D0=18
	// BYTE 4 (m_esc_data[3]): D7=27 D6=11 D5=26 D4=12 D3=25 D2=13 D1=24 D0=14
	// BYTE 5 (m_esc_data[4]): D7=31 D6=9  D5=30 D4=*  D3=29 D2=*  D1=28 D0=10
	// BYTE 6 (m_esc_data[5]): D7=35 D6=5  D5=34 D4=6  D3=33 D2=7  D1=32 D0=8
	// BYTE 7 (m_esc_data[6]): D7=*  D6=3  D5=*  D4=2  D3=19 D2=1  D1=UL D0=4

	u8 d[7] = {0}; // Row 0-6 data (5 bits each)
	u8 b[5]; // Byte 3-7
	for (int i = 0; i < 5; i++) b[i] = m_esc_data[2 + i];

	auto set_dot = [&](int dot, int state) {
		if (dot < 1 || dot > 35) return;
		int row = (dot - 1) / 5;
		int col = (dot - 1) % 5;
		if (state) d[row] |= (1 << (4 - col));
	};

	// Byte 3
	set_dot(18, BIT(b[0], 0)); set_dot(20, BIT(b[0], 1)); set_dot(17, BIT(b[0], 2)); set_dot(21, BIT(b[0], 3));
	set_dot(16, BIT(b[0], 4)); set_dot(22, BIT(b[0], 5)); set_dot(15, BIT(b[0], 6)); set_dot(23, BIT(b[0], 7));
	// Byte 4
	set_dot(14, BIT(b[1], 0)); set_dot(24, BIT(b[1], 1)); set_dot(13, BIT(b[1], 2)); set_dot(25, BIT(b[1], 3));
	set_dot(12, BIT(b[1], 4)); set_dot(26, BIT(b[1], 5)); set_dot(11, BIT(b[1], 6)); set_dot(27, BIT(b[1], 7));
	// Byte 5
	set_dot(10, BIT(b[2], 0)); set_dot(28, BIT(b[2], 1)); /* D2 */ set_dot(29, BIT(b[2], 3));
	/* D4 */ set_dot(30, BIT(b[2], 5)); set_dot(9, BIT(b[2], 6)); set_dot(31, BIT(b[2], 7));
	// Byte 6
	set_dot(8, BIT(b[3], 0)); set_dot(32, BIT(b[3], 1)); set_dot(7, BIT(b[3], 2)); set_dot(33, BIT(b[3], 3));
	set_dot(6, BIT(b[3], 4)); set_dot(34, BIT(b[3], 5)); set_dot(5, BIT(b[3], 6)); set_dot(35, BIT(b[3], 7));
	// Byte 7
	set_dot(4, BIT(b[4], 0)); /* D1=UL */ set_dot(1, BIT(b[4], 2)); set_dot(19, BIT(b[4], 3));
	set_dot(2, BIT(b[4], 4)); /* D5 */ set_dot(3, BIT(b[4], 6));

	memcpy(m_cgram[slot], d, 7);
	m_cgram[slot][7] = 0;

	m_next_slot = 1 - m_next_slot;
	m_esc_mode = false;
	m_esc_step = 0;
	m_esc_2byte_cmd = false;
}

// Helper to check if a character position should be blinking
bool noritake_vfd_device::is_blink_position(u8 pos) const
{
	return (pos == m_blink_pos[0] || pos == m_blink_pos[1]);
}

//**************************************************************************
//  COMMAND/DATA PROCESSING
//**************************************************************************

void noritake_vfd_device::control_write(u8 data)
{
	data_write(data);
}

void noritake_vfd_device::data_write(u8 data)
{
	m_dr = data;

	// Handle ESC sequence data
	if (m_esc_mode)
	{
		if (m_esc_step == 1)
		{
			m_esc_type = data;
			switch (data)
			{
			case 0x43: // ESC C - font? (waiting for count)
			case 0x48: // ESC H - position (waiting for row)
			case 0x4C: // ESC L - luminance
			case 0x53: // ESC S - flickerless
			case 0x54: // ESC T - blink speed
				m_esc_step++;
				return;
			case 0x49: // ESC I - clear screen
				memset(m_ddram, 0x20, sizeof(m_ddram));
				m_ac = 0;
				m_esc_mode = false;
				set_busy_flag(DELAY_CLEAR);
				return;
			default:
				// Assume it's the old 7-byte custom char ESC sequence
				m_esc_data[m_esc_step++] = data;
				return;
			}
		}

		if (m_esc_type == 0x48) // ESC H [row] [col]
		{
			if (m_esc_step == 2)
			{
				m_esc_param = data;
				m_esc_step++;
				return;
			}
			else if (m_esc_step == 3)
			{
				m_ac = (m_esc_param % m_num_line) * m_chars + (data % m_chars);
				m_esc_mode = false;
				set_busy_flag(DELAY_CURSOR);
				LOG("Noritake VFD: Cursor set to row %d col %d (AC %d)\n", m_esc_param, data, m_ac);
				return;
			}
		}
		else if (m_esc_type == 0x4C) // ESC L [val]
		{
			m_brightness = data;
			m_esc_mode = false;
			set_busy_flag(DELAY_MODE);
			return;
		}
		else if (m_esc_type == 0x43 || m_esc_type == 0x53) // ESC C/S [count] [data...]
		{
			if (m_esc_step == 2)
			{
				m_esc_param = data;
				if (m_esc_param == 0)
				{
					m_esc_mode = false;
					return;
				}
				m_esc_step++;
				return;
			}
			else
			{
				// consuming data bytes (ignored for now)
				if (--m_esc_param == 0)
					m_esc_mode = false;
				return;
			}
		}
		else if (m_esc_type == 0x54) // ESC T [val]
		{
			m_esc_mode = false;
			return;
		}

		// Fallback for old custom char sequence
		if (m_esc_step < 7)
		{
			m_esc_data[m_esc_step] = data;
			m_esc_step++;
		}
		if (m_esc_step >= 7)
		{
			process_esc_sequence();
			set_busy_flag(DELAY_NORMAL);
		}
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
			// Blink position: datasheet says corner characters (0, 19, 20, 39)
			// but we'll allow any position and track up to 2.
			u8 pos = data & 0x7f;
			if (pos < m_chars * m_num_line)
			{
				m_blink_pos[0] = m_blink_pos[1];
				m_blink_pos[1] = pos;
				LOG("Noritake VFD: Blink position set to %d\n", pos);
			}
			set_busy_flag(DELAY_BLINK);
			return;
		}
	}

	// Check for control codes (0x08-0x1F range)
	if (data < 0x20)
	{
			switch (data)
			{
				case CTL_BS: // Back Space
					if (m_ac > 0) m_ac--;
					LOG("Noritake VFD: Back Space\n");
					break;

				case 0x0E: // Special Clear (other emulator uses this)
					memset(m_ddram, 0x20, sizeof(m_ddram));
					m_ac = 0;
					LOG("Noritake VFD: Clear screen (0x0E)\n");
					set_busy_flag(DELAY_CLEAR);
					return;

				case CTL_HT: // Horizontal Tab
					m_ac = (m_ac | 7) + 1;
					if (m_ac >= m_chars * m_num_line) m_ac = m_chars * m_num_line - 1;
					LOG("Noritake VFD: Horizontal Tab\n");
					break;

				case CTL_LF: // Line Feed
					if (m_mode == 1 && m_ac >= m_chars * (m_num_line - 1)) // DC2 scroll mode
					{
						// Scroll up
						memmove(m_ddram, m_ddram + m_chars, m_chars * (m_num_line - 1));
						memset(m_ddram + m_chars * (m_num_line - 1), 0x20, m_chars);
					}
					else
					{
						m_ac += m_chars;
						if (m_ac >= m_chars * m_num_line) m_ac %= (m_chars * m_num_line);
					}
					LOG("Noritake VFD: Line Feed\n");
					break;

				case CTL_CR: // Carriage Return
					m_ac -= (m_ac % m_chars);
					LOG("Noritake VFD: Carriage Return\n");
					break;

				case CTL_FF: // Form Feed - Clear display / Home
					m_ac = 0;
					LOG("Noritake VFD: Form Feed (Home)\n");
					break;

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

				case CTL_DC5: // Cursor visible (0x15)
					m_cursor_on = true;
					LOG("Noritake VFD: Cursor visible (0x15)\n");
					break;

				case CTL_DC6: // Cursor invisible (0x16)
					m_cursor_on = false;
					LOG("Noritake VFD: Cursor invisible (0x16)\n");
					break;

				case CTL_DC7: // Blink off
					m_blink_on = false;
					m_blink_pos[0] = m_blink_pos[1] = 0xff;
					LOG("Noritake VFD: Blink off (DC7)\n");
					break;

				case 0x18: // Font select English
				case 0x19: // Font select International
					LOG("Noritake VFD: Font select 0x%02X\n", data);
					break;

				case CTL_SUB: // Brightness control (2-byte command, 0x1A)
					m_2byte_cmd_pending = true;
					m_2byte_cmd = CTL_SUB;
					LOG("Noritake VFD: Brightness control - awaiting level byte\n");
					return;

				case CTL_ESC: // Escape - start custom char definition
					m_esc_mode = true;
					m_esc_step = 1;
					m_esc_data[0] = 0x1B;
					LOG("Noritake VFD: ESC - start escape sequence\n");
					return;

				default:
					LOG("Noritake VFD: Unknown control code 0x%02X\n", data);
					break;
			}

		set_busy_flag(DELAY_MODE);
		return;
	}

	// Regular character write
	LOG("Noritake VFD: Write char '%c' (0x%02X) at AC %d\n", isprint(data) ? data : '.', data, m_ac);

	if (m_ac >= 0 && m_ac < m_chars * m_num_line)
	{
		m_ddram[m_ac] = data;
	}

	if (m_mode == 1 && m_ac == m_chars * m_num_line - 1) // DC2 scroll mode
	{
		// Scroll up
		memmove(m_ddram, m_ddram + m_chars, m_chars * (m_num_line - 1));
		memset(m_ddram + m_chars * (m_num_line - 1), 0x20, m_chars);
		m_ac = m_chars * (m_num_line - 1);
	}
	else
	{
		m_ac++;
		if (m_ac >= m_chars * m_num_line) m_ac = 0;
	}

	set_busy_flag(DELAY_NORMAL);
}

u8 noritake_vfd_device::data_read()
{
	u8 data = 0;

	if (m_ac >= 0 && m_ac < m_chars * m_num_line)
		data = m_ddram[m_ac];

	LOG("Noritake VFD: Read 0x%02X from DDRAM at %d\n", data, m_ac);

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

u8 noritake_vfd_device::control_read()
{
	// Return busy flag and cursor position
	u8 result = (m_busy_flag ? 0x80 : 0) | (m_ac & 0x7f);
	return result;
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

void noritake_vfd_device::vfd_w(u8 data)
{
	vfd_bit_bang_w(BIT(data, 0), BIT(data, 1), BIT(data, 2), BIT(data, 3), (data >> 4) & 0x03);
	m_p40_prev = data;
}

void noritake_vfd_device::vfd_bit_bang_w(int data, int clock, int cs, int latch, int addr)
{
	// Rising edge on clock latches data bit (LSB first, data inverted)
	if (!BIT(m_p40_prev, 1) && clock)
	{
		m_vfd_data = (m_vfd_data >> 1) | (data ? 0x00 : 0x80);
		m_vfd_bits++;
		if (m_vfd_bits == 8)
		{
			if (addr == 0) // Data
				data_write(m_vfd_data);
			else if (addr == 2) // Cmd
				control_write(m_vfd_data);
			
			m_vfd_bits = 0;
			m_vfd_data = 0;
		}
	}
}

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
