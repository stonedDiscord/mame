// license:BSD-3-Clause
// copyright-holders:stonedDiscord

/*

Elektronische Steuereinheit
8085 based hardware
Lots of lamps and 8 7-segment LEDs

Main components:
Siemens SAB 8085AH-2-P (CPU)
Sharp LH5164D-10L or Sony CXK5816PN-12L (SRAM)
Siemens SAB 8256 A 2 P (MUART)
NEC D8279C-2 (keyboard & display interface)
AMI or Micrel S50240 (sound)

At least 4 different boards exist:
4040-000-101 (6 ROM slots, TC5514 RAM) used in excellent
4087-000-101 (3 ROM slots, RTC HD146818) used in doppelpot
4109-000-101 (2 ROM slots, RTC 62421A) used in kniffi
4382-000-101 (2 ROM slots, RTC 62421A) used in dicemstr

Dice Master reference: https://www.youtube.com/watch?v=NlB06dMxjME
Merkur Disc reference: https://www.youtube.com/watch?v=1NjJPkzg9Mk
Nova Kniffi reference: https://www.youtube.com/watch?v=YBq2Z1irXek

At first boot the machine requires initialization by pressing Up 1,- and Initialize at the same time.
Opening the door puts the machine into service mode where the keys on the service keyboard actually do something.
*/


#include "emu.h"

#include "cpu/i8085/i8085.h"
#include "machine/i8255.h"
#include "machine/i8256.h"
#include "machine/i8279.h"
#include "machine/mc146818.h"
#include "machine/msm6242.h"
#include "machine/steppers.h"
#include "sound/beep.h"

#include "bus/rs232/rs232.h"

#include "speaker.h"

#include "adpservice.lh"
#include "disc2000.lh"

#define VERBOSE 1
#include "logmacro.h"


namespace {

class stella8085_state : public driver_device
{
public:
	stella8085_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_ppi(*this, "ppi"),
		m_uart(*this, "muart"),
		m_rs232(*this, "rs232"),
		m_kdc(*this, "kdc"),
		m_motor(*this, "motor%u", 0U),
		m_tz(*this, "TZ%u", 0U),
		m_dsw(*this, "DSW"),
		m_digits(*this, "digit%u", 0U),
		m_lamps(*this, "lamp%u%u", 0U, 0U),
		m_beep(*this, "beeper")
	{ }

	void dicemstr(machine_config &config) ATTR_COLD;
	void doppelpot(machine_config &config) ATTR_COLD;
	void excellent(machine_config &config) ATTR_COLD;

protected:
	void machine_start() override ATTR_COLD;

private:
	uint8_t m_digit = 0U;
	uint8_t m_kbd_sl = 0x00;
	bool m_kbd_bd = false;
	uint8_t m_optic = 0x00;

	// coin acceptor sequencer: an IPT_COIN press is replayed as the light-barrier
	// sequence the firmware validates (one LIM denomination pulse, then LIG).
	uint8_t m_coin_seq = 0;       // 0 = idle, otherwise the current step
	uint8_t m_coin_lim = 0;       // LIM denomination bit for the coin being inserted
	// Physical RL levels: lines idle HIGH, a coin/eject pulses them LOW (the i8279
	// inverts for the firmware). These hold which line is currently pulled low.
	uint8_t m_coin_lim_out = 0;   // LIM bit (TZ1 0-3) currently pulsed low (0 = none)
	uint8_t m_coin_lig = 0;       // LIG (TZ1 bit7): 1 = pulsed low, 0 = idle high
	uint8_t m_coin_ruem = 0;      // RUEM (TZ2 bit4): physical rest = LOW (firmware reads it high)
	uint8_t m_coin_zem = 0;       // ZEM1 (TZ2 bit6 Fadenfoul): 1 = pulsed low, 0 = idle high
	uint8_t m_coin_keys = 0;      // previous IPT_COIN key state, for edge detection

	uint8_t m_lia_out = 0;        // LIA bits (TZ2 0-3) currently blocked low (0 = none, all high)
	uint8_t m_aw_state = 0x0f;    // previous AW1-4 state for edge detection (idle high)
	uint8_t m_lia_seq = 0;        // LIA block sequence step
	uint8_t m_lia_bit = 0;        // LIA bits (channels) whose ejected coin is in transit

	required_device<i8085a_cpu_device> m_maincpu;
	required_device<i8255_device> m_ppi;
	required_device<i8256_device> m_uart;
	required_device<rs232_port_device> m_rs232;
	required_device<i8279_device> m_kdc;
	optional_device_array<stepper_device, 5> m_motor;
	required_ioport_array<8> m_tz;
	required_ioport m_dsw;
	output_finder<16> m_digits;
	output_finder<8, 8> m_lamps;
	required_device<beep_device> m_beep;
	emu_timer *m_sound_timer;
	emu_timer *m_coin_timer;
	emu_timer *m_lia_timer;
	emu_timer *m_rst55_timer;     // 556+4040+4051 chain -> CD4013 -> RST5.5 note-advance clock
	emu_timer *m_rst55_clear_timer; // clears RST5.5 just after the ISR acknowledges it
	uint8_t m_snd_chan = 0;       // 4051 channel = sound byte bits 4,5 = note-advance rate

	void large_program_map(address_map &map) ATTR_COLD;
	void program_map(address_map &map) ATTR_COLD;
	void program_4040_map(address_map &map) ATTR_COLD;
	void io_map(address_map &map) ATTR_COLD;
	void io_4040_map(address_map &map) ATTR_COLD;

	// I8256 ports
	uint8_t lw_r(); //P1.0-P1.3
	void machine1_w(uint8_t data);
	void machine2_w(uint8_t data);
	void update_optics();

	// coin acceptor
	TIMER_CALLBACK_MEMBER(coin_seq_tick);
	TIMER_CALLBACK_MEMBER(lia_tick);

	// I8279 Interface
	uint8_t kbd_rl_r();
	void kbd_sl_w(uint8_t data);
	void kbd_bd_w(uint8_t data);
	void disp_w(uint8_t data);
	void output_digit(uint8_t i, uint8_t data);

	void io00(uint8_t data) ATTR_COLD;
	void io70(uint8_t data) ATTR_COLD;
	void io71(uint8_t data) ATTR_COLD;
	void sounddev(uint8_t data) ATTR_COLD;
	uint8_t io9r() ATTR_COLD;
	void io9w(uint8_t data) ATTR_COLD;

	void makesound(uint8_t tone, uint8_t octave, uint8_t length);
	int soundfreq(uint8_t channel, uint8_t octave);
	TIMER_CALLBACK_MEMBER(sound_stop);
	TIMER_CALLBACK_MEMBER(rst55_tick);
	TIMER_CALLBACK_MEMBER(rst55_clear);
	IRQ_CALLBACK_MEMBER(sound_irq_ack);
};

// Note-advance rate for the RST5.5 sound interrupt. On the PCB a 556 oscillator
// feeds a 4040 ripple counter whose taps are selected by a 4051 mux driven by the
// sound byte's D4/D5; the selected clock toggles a CD4013 flip-flop wired to RST5.5.
// So the *previous* note's D4/D5 bits set how long until the next note is fetched.
// These four periods are estimates (the exact 556 RC / 4040 taps are unverified) -
// tune them to match the real tempo.
static constexpr int SND_PERIOD_US[4] = { 30000, 60000, 120000, 240000 };

// S50240 (ICG9) top-octave-synthesizer master clock, at the *highest* octave.
// On the PCB the 74LS290 (ICG8-1) derives SOUND_CLOCK from BUS_CLK; the 4040
// (ICG8-2) divides it and the 4051 (ICF8, selected by SOUND_D6/D7) taps Q(octave)
// to feed the S50240 clock through TR84. Folding the octave-0 divider in here, the
// per-octave pitch is simply SOUND_CLOCK >> octave. ~2 MHz is the MK50240 nominal
// for equal temperament (÷239 = C9, ÷478 = C8); adjust to match the real board.
static constexpr int SOUND_CLOCK = (6.144_MHz_XTAL / 8).value();

void stella8085_state::machine_start()
{
	m_sound_timer = timer_alloc(FUNC(stella8085_state::sound_stop), this);
	m_coin_timer = timer_alloc(FUNC(stella8085_state::coin_seq_tick), this);
	m_lia_timer = timer_alloc(FUNC(stella8085_state::lia_tick), this);
	m_rst55_timer = timer_alloc(FUNC(stella8085_state::rst55_tick), this);
	m_rst55_clear_timer = timer_alloc(FUNC(stella8085_state::rst55_clear), this);
	// 556 free-runs continuously; start the note-advance clock at the default rate
	m_rst55_timer->adjust(attotime::from_usec(SND_PERIOD_US[0]), 0, attotime::from_usec(SND_PERIOD_US[0]));

	save_item(NAME(m_digit));
	save_item(NAME(m_snd_chan));
	save_item(NAME(m_optic));
	save_item(NAME(m_coin_seq));
	save_item(NAME(m_coin_lim));
	save_item(NAME(m_coin_lim_out));
	save_item(NAME(m_coin_lig));
	save_item(NAME(m_coin_ruem));
	save_item(NAME(m_coin_zem));
	save_item(NAME(m_coin_keys));
	save_item(NAME(m_lia_out));
	save_item(NAME(m_aw_state));
	save_item(NAME(m_lia_seq));
	save_item(NAME(m_lia_bit));
}

void stella8085_state::large_program_map(address_map &map)
{
	map(0x0000, 0x7fff).rom(); // ICE6
	map(0x8000, 0x9fff).ram(); // ICC6
	map(0xa000, 0xffff).rom(); // ICD6
}

void stella8085_state::program_map(address_map &map)
{
	map(0x0000, 0x8fff).rom(); // ICE6, ICD6, ICC5
	map(0x9000, 0x933f).rw("rtc", FUNC(mc146818_device::read_direct), FUNC(mc146818_device::write_direct));
	map(0xa000, 0xafff).ram(); // ??
	map(0xc000, 0xc7ff).ram(); // ICC6
}

void stella8085_state::program_4040_map(address_map &map)
{
	map(0x0000, 0x4fff).rom();
	map(0x5000, 0x5fff).ram();
	map(0x6000, 0x633f).rw("rtc", FUNC(mc146818_device::read_direct), FUNC(mc146818_device::write_direct));
	map(0x7000, 0x7fff).rom();
}

void stella8085_state::io_map(address_map &map)
{
	map(0x00, 0x00).w(FUNC(stella8085_state::io00));
	map(0x50, 0x51).rw(m_kdc, FUNC(i8279_device::read), FUNC(i8279_device::write));
	map(0x60, 0x6f).rw(m_uart, FUNC(i8256_device::read), FUNC(i8256_device::write));
	map(0x70, 0x73).rw(m_ppi, FUNC(i8255_device::read), FUNC(i8255_device::write));
	// map(0x80, 0x8f) //Y8 ICC5 empty socket
	map(0x90, 0x9f).rw(FUNC(stella8085_state::io9r),FUNC(stella8085_state::io9w)); //Y9 wired to rtc circuits but somehow memory mapped in hardware
}

void stella8085_state::io_4040_map(address_map &map)
{
	map(0x00, 0x00).w(FUNC(stella8085_state::io00));
	map(0x70, 0x73).rw(m_ppi, FUNC(i8255_device::read), FUNC(i8255_device::write));
	map(0x80, 0x81).rw(m_kdc, FUNC(i8279_device::read), FUNC(i8279_device::write));
	map(0x90, 0x9f).rw(m_uart, FUNC(i8256_device::read), FUNC(i8256_device::write));
}

/*********************************************
*      I8256 Ports controlling the wheels    *
*                                            *
*********************************************/

// Each wheel is spun by a 2-phase stepper and read back by a light barrier on
// P1.0-P1.3. The wheels are coded optical discs: a ring of slots whose widths
// and spacing encode the symbol positions, with one wheel-specific reference.
// The init code (homing one wheel at a time) slows the motor near home and reads
// the slot it stops on; the per-wheel disc is generated in update_optics() from
// DISC_PATTERN, captured from real hardware (see the comment there).
uint8_t stella8085_state::lw_r()
{
	// wheel light sensors

	// P1.0 LIW1 - wheel 1 index optic (active high)
	// P1.1 LIW2 - wheel 2 index optic
	// P1.2 LIW3 - wheel 3 index optic
	// P1.3 LIW4 - wheel 4 index optic
	// P1.4 M5A out
	// P1.5 M5B out
	// P1.6 is always low
	// P1.7 LIW5

	return 0xb0 | (m_optic & 0x0f);
}

// Coded optical disc per wheel, captured from real disc2000 hardware .
// The left (wheel 1) and right (wheel 2) wheels are physically identical discs.
// #...#...#...#..###..#...#...#...#...#...#...#...
static constexpr uint64_t DISC_LR =
	(1ULL<<0)|(1ULL<<4)|(1ULL<<8)|(1ULL<<12)|(1ULL<<15)|(1ULL<<16)|(1ULL<<17)|
	(1ULL<<20)|(1ULL<<24)|(1ULL<<28)|(1ULL<<32)|(1ULL<<36)|(1ULL<<40)|(1ULL<<44);

// The firmware homes each wheel against this disc and parks symbol 0 one optic
// step *before* an index mark (verified: it stops in the gap just ahead of a mark,
// at the same get_position the visible reel is drawn at). The disc is mounted with
// that one-step angular offset relative to the stepper's electrical home, so the
// optic the firmware samples is the disc rotated back by one step. Without it the
// wheel motor self-test (Foul service screen, FUN_ram_0f71) reads the index optic
// dark, steps the motor +1 step, still reads dark and reports all three motors
// faulty (service code 00000007). With it the step lands on the next mark, the
// optic toggles and the test passes, matching real hardware.
static constexpr int DISC_OPTIC_OFFSET = 47; // -1 (mod 48)

static constexpr uint64_t DISC_PATTERN[5] =
{
	DISC_LR, // wheel 1 (left)
	DISC_LR, // wheel 2 (right)
	DISC_LR,
	DISC_LR,
	DISC_LR
};

void stella8085_state::update_optics()
{
	// each wheel's light barrier (P1.0-P1.3) follows its coded disc as it turns;
	// the reel position (0..95 half-steps) maps to the 48-step disc table
	for (unsigned n = 0; n < 4; n++)
	{
		const int step = ((m_motor[n]->get_position() >> 1) + DISC_OPTIC_OFFSET) % 48;
		if (BIT(DISC_PATTERN[n], step))
			m_optic |= (1 << n);
		else
			m_optic &= ~(1 << n);
	}
}

void stella8085_state::machine1_w(uint8_t data)
{
	// each wheel is a 2-phase stepper motor driven by two coils:
	// P2.0/P2.1 -> motor 1, P2.2/P2.3 -> motor 2, P2.4/P2.5 -> motor 3, P2.6/P2.7 -> motor 4
	m_motor[0]->update( data       & 0x03);
	m_motor[1]->update((data >> 2) & 0x03);
	m_motor[2]->update((data >> 4) & 0x03);
	m_motor[3]->update((data >> 6) & 0x03);

	update_optics();

	// refresh the reel position/scroll outputs so the layout discs animate
	for (auto &motor : m_motor)
		motor->draw();
}

void stella8085_state::machine2_w(uint8_t data)
{
	m_motor[4]->update((data >> 4) & 0x03);

	update_optics();

	// refresh the reel position/scroll outputs so the layout discs animate
	for (auto &motor : m_motor)
		motor->draw();
}

/*********************************************
*      I8279 Keyboard-Disply Interface       *
*                                            *
*********************************************/

void stella8085_state::kbd_sl_w(uint8_t data)
{
	m_kbd_sl = data;

	// SL3 connected through CD4093 NAND to DIP switch connected to RST75
	if (BIT(m_dsw->read(), 0))
		m_maincpu->set_input_line(I8085_RST75_LINE, BIT(data,3) ? CLEAR_LINE : ASSERT_LINE);
	else
		m_maincpu->set_input_line(I8085_RST75_LINE, CLEAR_LINE);
}

void stella8085_state::kbd_bd_w(uint8_t data)
{
	m_kbd_bd = data;
}

uint8_t stella8085_state::kbd_rl_r()
{
	// The 8279 is in 16-character display mode, so its scan counter runs 0-15,
	// but only 8 sensor rows (TZ0-TZ7) exist - mask the scan line to 8.
	const uint8_t row = m_kbd_sl & 7;
	uint8_t data = m_tz[row]->read();

	// kbd_rl_r returns the PHYSICAL RL pin levels; the i8279 inverts them into its
	// sensor RAM (rl = in_rl ^ 0xff, see i8279.cpp) and the firmware reads that.
	// So the ioports hold the real electrical levels, and what the firmware reads
	// is their complement (matching the real-HW test-ROM dump FC 00 10 33 00..).

	// Row 1 carries the coin acceptor (LIM1-4 = bits 0-3, LIG = bit 7). The raw
	// IPT_COIN keys only trigger the sequencer; the actual line levels are replayed
	// (see coin_seq_tick). Physically every line idles HIGH (pulled up); a coin
	// grounds one LIM line and the common LIG barrier (drives them LOW).
	// The barrier self-test signal (D6 = c01b bit6) drives every coin light barrier
	// blocked (low): the firmware clears D6 and checks the barriers read high, then sets
	// D6 and checks they read low. It toggles D6 in RAM without re-latching port 0x70
	// during the (interrupt-masked) check, so read the intent straight from c01b.
	const bool short_test = BIT(m_maincpu->space(AS_PROGRAM).read_byte(0xc01b), 6);

	if (row == 1)
	{
		// edge-detect the IPT_COIN keys (active high on the LIM bits) and kick off
		// the acceptor sequence for one denomination, then build the physical levels.
		const uint8_t keys = data & 0x0f;
		const uint8_t pressed = keys & ~m_coin_keys;
		m_coin_keys = keys;
		if (pressed && m_coin_seq == 0)
		{
			m_coin_lim = pressed & (~pressed + 1); // a single denomination
			m_coin_seq = 1;
			m_coin_timer->adjust(attotime::zero);
		}
		// bits 4-6 (NC, MK) idle high; LIM low nibble idles high, coin pulses one low;
		// bit7 = LIG (gemeinsame Münzlichtschranke / common coin barrier), idles high,
		// pulses low on a coin and reads low during the D6 barrier self-test.
		data = 0x70 | (0x0f & ~m_coin_lim_out) | ((m_coin_lig || short_test) ? 0x00 : 0x80);
	}
	else if (row == 2)
	{
		// TZ2 carries no operator input - every bit is an internal coin-mechanism light
		// barrier. Physical rest (real-HW dump TZ2=0x10 => 0xEF): LIA(0-3), LÜ(5), ZEM1(6),
		// ZEM2(7) idle HIGH; RUEM(4) idles LOW; a coin pulses ZEM1 low; an ejected coin
		// pulses its LIA line low. During the D6 self-test every checked barrier
		// (LIA 0-3, LÜ 5, ZEM1 6) reads blocked (low); ZEM2 (7) is not tested.
		if (short_test)
			data = 0x80; // ZEM2 high; LIA / LÜ / ZEM1 / RUEM all low (blocked)
		else
			data = 0xa0 | (0x0f & ~m_lia_out) | (m_coin_ruem ? 0x10 : 0x00) | (m_coin_zem ? 0x00 : 0x40);
	}

	return data;
}

// The firmware (FUN_ram_1f0e/34a4 in disc2001) latches the LIM denomination while
// RUEM (the common line, simultaneous with the coin pulse per the pinout) is low,
// so replay: LIM_n high + RUEM low together for one pulse, then release.

TIMER_CALLBACK_MEMBER(stella8085_state::coin_seq_tick)
{
	// Physical pulse: lines drop LOW as the coin passes. (exact sequence/timing vs
	// the firmware is still TODO - coin crediting is deferred.)
	switch (m_coin_seq)
	{
	case 1: // coin dropping past the denomination + Fadenfoul: LIM_n + ZEM1 low
		m_coin_lim_out = m_coin_lim;
		m_coin_zem = 1;
		m_coin_lig = 1;
		m_coin_seq = 2;
		m_coin_timer->adjust(attotime::from_msec(40));
		break;
	case 2: // coin past the denomination, still at the common barrier: LIM high, LIG low
		m_coin_lim_out = 0;
		m_coin_zem = 1;
		m_coin_lig = 1;
		m_coin_seq = 3;
		m_coin_timer->adjust(attotime::from_msec(80));
		break;
	default: // coin has fully passed - back to idle (all lines high)
		m_coin_lim_out = 0;
		m_coin_zem = 0;
		m_coin_lig = 0;
		m_coin_seq = 0;
		break;
	}
}

TIMER_CALLBACK_MEMBER(stella8085_state::lia_tick)
{
	switch (m_lia_seq)
	{
	case 1: // coin now passing the barrier(s): pull the ejected channels' LIA low
		m_lia_out = m_lia_bit;
		m_lia_seq = 2;
		m_lia_timer->adjust(attotime::from_msec(40)); // time for the coin to clear into the tray
		break;
	default: // coin has landed in the tray: barriers back to idle (all high)
		m_lia_out = 0;
		m_lia_bit = 0;
		m_lia_seq = 0;
		break;
	}
}

void stella8085_state::disp_w(uint8_t data)
{
	if (m_kbd_sl < 8)
	{
		for (int i = 0; i < 8; i++)
		{
			bool lamp_value = BIT(data, i);
			m_lamps[m_kbd_sl][i] = lamp_value;
		}
	}
	else
	{
		output_digit(m_kbd_sl, data);
	}
}

void stella8085_state::output_digit(uint8_t i, uint8_t data)
{
	// Seven-segment encoding for digits 0-9 (abcdefg, no decimal point)
	static const uint8_t bcd_to_7seg[16] =
	{
		0x3f, // 0: 0b00111111
		0x06, // 1: 0b00000110
		0x5b, // 2: 0b01011011
		0x4f, // 3: 0b01001111
		0x66, // 4: 0b01100110
		0x6d, // 5: 0b01101101
		0x7d, // 6: 0b01111101
		0x07, // 7: 0b00000111
		0x7f, // 8: 0b01111111
		0x6f, // 9: 0b01101111

		0x77, // A: 0b01110111
		0x7c, // B: 0b01111100
		0x39, // C: 0b00111001
		0x5e, // D: 0b01011110
		0x79, // E: 0b01111001
		0x71  // F: 0b01110001
	};

	if (i > 7)
	{
		uint8_t debug = data & 0x0f;
		uint8_t cash = data >> 4;

		m_digits[i - 8] = bcd_to_7seg[debug];
		m_digits[i] = bcd_to_7seg[cash];
	}
}

TIMER_CALLBACK_MEMBER(stella8085_state::sound_stop)
{
	m_beep->set_state(0);
}

void stella8085_state::io00(uint8_t data)
{
	//old boards
}

uint8_t stella8085_state::io9r()
{
	return 0xff; //old boards
}

void stella8085_state::io9w(uint8_t data)
{
	//old boards
}

void stella8085_state::io70(uint8_t data)
{
	// AW1-4 (the coin-eject outputs) idle HIGH and are pulled LOW to eject a coin,
	// so an eject is a falling edge (high->low) on a line.
	const uint8_t aw = data & 0x0f;
	const uint8_t ejected = m_aw_state & ~aw; // lines that just went high->low
	m_aw_state = aw;

	const bool AW1 = !BIT(data,0); // active low: true while ejecting this channel
	const bool AW2 = !BIT(data,1);
	const bool AW3 = !BIT(data,2);
	const bool AW4 = !BIT(data,3);
	const bool MP = BIT(data,4);
	const bool SZ = BIT(data,5);
	const bool D6 = BIT(data,6); // high on startup
	const bool PA7 = BIT(data,7);

	if (ejected)
		popmessage("eject %d%d%d%d MP %d SZ %d D6 %d PA7 %d\n", AW4,AW3,AW2,AW1,MP,SZ,D6,PA7);

	machine().bookkeeping().coin_lockout_global_w(MP); // coin magnet
	machine().bookkeeping().coin_counter_w(0,AW1); // coin eject (active low)
	machine().bookkeeping().coin_counter_w(1,AW2);
	machine().bookkeeping().coin_counter_w(2,AW3);
	machine().bookkeeping().coin_counter_w(3,AW4);
	machine().bookkeeping().coin_counter_w(5,SZ); // game counter

	// Each channel pulsed low ejects one coin. A short moment later the coin drops
	// through that channel's LIA light barrier (pulling the line low), then falls into
	// the payout tray and the barrier returns to idle.
	if (ejected)
	{
		m_lia_bit |= ejected; // every channel that just started ejecting
		if (m_lia_seq == 0)
		{
			m_lia_seq = 1;
			m_lia_timer->adjust(attotime::from_msec(40)); // coin travels to the barrier
		}
	}

	// D6 ("Short test", c01b bit6) drives the LIA barrier self-test; the barriers
	// follow it directly off c01b in kbd_rl_r (the firmware doesn't re-latch it here).

	if (PA7)
		LOG("PA7 high\n");
}

void stella8085_state::io71(uint8_t data)
{
	const bool RS = BIT(data,0);
	//const bool GONG = BIT(data,1);
	const bool DG = BIT(data,2);
	const bool UG = BIT(data,3);
	const bool DS = BIT(data,4);
	const bool US = BIT(data,5);
	const bool DM = BIT(data,6);
	const bool UM = BIT(data,7);

	// IO71 D0 is the CD4013 flip-flop reset (DIP-gated to RST5.5). The note-advance
	// interrupt is *asserted* by the 556/4040/4051 timer chain (see rst55_tick); the
	// RST5.5 ISR pulses D0 high to acknowledge/clear it. D0 low just re-arms the FF.
	// D0 high resets the CD4013 -> clears RST5.5 (the ISR's note acknowledge). On real HW
	// this happens here; in MAME clearing the CPU's own interrupt line from its running
	// I/O write is not applied in time and RST5.5 re-fires back-to-back, so the actual
	// clear is driven from the interrupt-acknowledge instead (see sound_irq_ack).
	(void)RS;

	// DG (OUT 0x71 bit2, ICJ6 Q6) drives a hardware muting circuit
	// that suppresses speaker crackle
	m_beep->set_output_gain(ALL_OUTPUTS,!DG);

	/*
	if (GONG)
		popmessage("GONG");*/
	if (US)
		LOG("activating US\n");
	if (UG || DS || DM || UM)
		LOG("UG %d DS %d DM %d UM %d\n", UG,DS,DM,UM);
}

void stella8085_state::sounddev(uint8_t data)
{
	LOG("sound %02X\n", data);
	// OUT 0x72 is latched by ICH6 into SOUND_D0..D7: D0-D3 = note (4067 channel),
	// D4-D5 = note-advance tap, D6-D7 = octave (S50240 clock divider).
	uint8_t tone = data & 0x0f;
	uint8_t octave = (data >> 6) & 0x03;
	// D4/D5 pick the 4051 channel = the 556/4040 tap that times the next RST5.5, i.e. how
	// long this note sounds. Re-arm the note-advance clock at that rate.
	m_snd_chan = (data >> 4) & 0x03;
	const attotime period = attotime::from_usec(SND_PERIOD_US[m_snd_chan]);
	m_rst55_timer->adjust(period, 0, period);
	makesound(tone, octave, SND_PERIOD_US[m_snd_chan] / 1000);
}

void stella8085_state::makesound(uint8_t tone, uint8_t octave, uint8_t length)
{
	// Only 4067 channels 1-12 are wired to a divider (the 12 semitones). Channel 0 is a
	// real rest (the tune's note gaps) -> silence. Channels 13-15 are not notes: the
	// firmware constantly sprays 0xFF (all bits set) to OUT 0x72 to silence the chip
	// between notes - if we acted on those we would either play a constant tone over the
	// melody or cut every note dead. Ignore them so the current note rings for its length.
	if (tone > 12)
		return;
	if (tone == 0) // rest
	{
		m_beep->set_state(0);
		return;
	}
	int sfrq = soundfreq(tone, octave);
	LOG("sound freq %d Hz for %d ms\n", sfrq, length);
	m_beep->set_clock(sfrq);
	m_beep->set_state(1);
	// hold the note until the next one arrives; stop if the tune ended (no refresh)
	m_sound_timer->adjust(attotime::from_msec(length), 0);
}

TIMER_CALLBACK_MEMBER(stella8085_state::rst55_tick)
{
	// A 556 oscillator -> 4040 divider -> 4051 (selected by the last sound byte's
	// D4/D5) clocks the CD4013 flip-flop wired to RST5.5. The flip-flop asserts RST5.5
	// (held) until the RST5.5 ISR resets it via io71 D0. DIP SW1:1 gates it to the CPU.
	// (A fine scheduling quantum makes that self-clear take effect before the ISR re-
	// enables interrupts, otherwise it would re-fire and the whole tune plays at once.)
	if (BIT(m_dsw->read(), 3))
		m_maincpu->set_input_line(I8085_RST55_LINE, ASSERT_LINE);
}

IRQ_CALLBACK_MEMBER(stella8085_state::sound_irq_ack)
{
	// When the CPU actually takes the RST5.5 (sound) interrupt, schedule the CD4013
	// reset shortly after - this lands inside the ISR (interrupts disabled) so RST5.5
	// is cleared before the ISR re-enables them, modelling the io71 D0 acknowledge
	// without the unreliable "CPU clears its own line mid-instruction" behaviour.
	if (irqline == I8085_RST55_LINE)
		m_rst55_clear_timer->adjust(attotime::from_usec(1));
	return 0;
}

TIMER_CALLBACK_MEMBER(stella8085_state::rst55_clear)
{
	m_maincpu->set_input_line(I8085_RST55_LINE, CLEAR_LINE);
}

int stella8085_state::soundfreq(uint8_t channel, uint8_t octave)
{
	// MK50240 top-octave divider ratios. The S50240 emits 13 outputs (one octave of
	// 12 equal-tempered semitones) as SOUND_CLOCK / ratio. The 4067 (ICH8) routes one
	// of them to the speaker, addressed by the tone nibble SOUND_S0..S3:
	//   ch 1=C9 2=B8 3=A8# 4=A8 5=G8# 6=G8 7=F8# 8=F8 9=E8 10=D8# 11=D8 12=C8#
	// ch 0 (÷478=C8) and ch 13..15 are not wired to the mux -> silence.
	static const int ratio[13] = { 478, 239, 253, 268, 284, 301, 319, 338, 358, 379, 402, 426, 451 };
	if (channel < 1 || channel > 12)
		return 0;
	// octave (SOUND_D6/D7) taps the 4040/4051 clock divider feeding the S50240, so a
	// larger octave field divides the master clock further -> lower pitch.
	return (SOUND_CLOCK >> octave) / ratio[channel];
}

static INPUT_PORTS_START( stella8085_service )
	PORT_START("TZ6") // TASTATUR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Reset")          // TS7
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Continuous run") //     Dauerlauf
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Game counter")   //     Spielzähler
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Coin counter")   //     Münzspeicher
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Hardware-Test")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Payout quote")   //     Auszahlquote
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Foul")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Win")            // TS0 Gewinn

	PORT_START("TZ7") // TASTATUR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Up 1,-")         // TS7 Hoch 1,-
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Down 1,-")       //     Runter 1,-
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Up Series")      //     Hoch Serie
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Down Series")    //     Runter Serie
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Up 0,10")        //     Hoch 0,10
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Down 0,10")      //     Runter 0,10
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Coinage")        //     Münzung
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Initialize")     // TS0 Initialisieren
INPUT_PORTS_END

static INPUT_PORTS_START( stella8085_dip )
	PORT_START("DSW")
	PORT_DIPNAME(0x01, 0x01, "8085 RST75") PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(0x00, DEF_STR(Off))
	PORT_DIPSETTING(0x01, DEF_STR(On))
	PORT_DIPNAME(0x02, 0x00, "8085 HOLD")  PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(0x00, DEF_STR(Off))
	PORT_DIPSETTING(0x02, DEF_STR(On))
	PORT_DIPNAME(0x04, 0x00, "8085 Reset") PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(0x00, DEF_STR(Off))
	PORT_DIPSETTING(0x04, DEF_STR(On))
	PORT_DIPNAME(0x08, 0x08, "8085 RST55") PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(0x00, DEF_STR(Off))
	PORT_DIPSETTING(0x08, DEF_STR(On))
INPUT_PORTS_END

static INPUT_PORTS_START( servicem )
	PORT_INCLUDE(stella8085_dip)

	PORT_START("TZ0") //TASTEN
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("TZ1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_NAME("DM 0.10")  //LIM1 COIN II
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_NAME("DM 1.00")  //LIM2 COIN II
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_NAME("DM 2.00")  //LIM3 COIN II
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_NAME("DM 5.00")  //LIM4 COIN II

	PORT_START("TZ2")
	// TZ2 carries no operator inputs - every bit is an internal coin-mechanism
	// light barrier / feedback signal (LIA1-4 payout barriers, RüM, Lü, ZEM1/2).
	// The whole row is built from the emulator in kbd_rl_r, so this port is just a
	// placeholder to satisfy the TZ ioport array and is never read.
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("TZ3") //ZUSATZ-EINGAENGE
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("TZ4") //MATRIX-EINGAENGE
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("TZ5") //MATRIX-EINGAENGE
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_INCLUDE(stella8085_service)
INPUT_PORTS_END

static INPUT_PORTS_START( disc )
	PORT_INCLUDE(stella8085_dip)

	PORT_START("TZ0") //TASTEN
	// These are PHYSICAL RL levels; the i8279 inverts them for the firmware. Real-HW
	// rest dump (door open) = firmware reads TZ0 0xFC => physical 0x03: door(bit0) and
	// bit1 idle high, bits 2-7 idle low. Door physically reads HIGH when open, LOW
	// when closed (firmware then reads bit0=1 closed = the working play state).
	PORT_CONFNAME( 0x01, 0x00, "Door (Türschalter)" ) // TS
	PORT_CONFSETTING(    0x00, "Closed" )
	PORT_CONFSETTING(    0x01, "Open" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT ) // SK Schlagkontakt / Read data button
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // ZE2
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_GAMBLE_PAYOUT ) // Return
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SLOT_STOP3 ) // STR
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START ) // NF

	PORT_START("TZ1")
	// LIM = Münzeingang (per denomination). These coin keys only trigger the
	// acceptor sequencer (coin_inserted); the actual LIM/LIG matrix levels are
	// replayed into this row by kbd_rl_r, so a single press credits one coin.
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 ) PORT_NAME("DM 0.10") //LIM1
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_NAME("DM 1.00") //LIM2
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_NAME("DM 2.00") //LIM3
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_NAME("DM 5.00") //LIM4
	// Only the COIN bits (0-3) of this port are used - kbd_rl_r edge-detects them to
	// trigger the acceptor sequencer and then synthesises the whole physical row
	// (bits 4-7 = NC/MK/LIG are built in kbd_rl_r, not read from here).
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // NC
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // NC
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) //MK Münzeinheitenkennung
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) //LIG

	PORT_START("TZ2")
	// TZ2 carries no operator inputs - every bit is an internal coin-mechanism
	// light barrier / feedback signal (LIA1-4 payout barriers, RüM, Lü, ZEM1/2).
	// The whole row is built from the emulator in kbd_rl_r, so this port is just a
	// placeholder to satisfy the TZ ioport array and is never read.
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("TZ3") //ZUSATZ-EINGAENGE
	// Physical RL (the i8279 inverts these for the firmware). Zusatzeingaenge per the
	// pinout: each input is +12V (active) or 0V. ZE5 is the common key return line
	// (gemeinsame Rueckfuehrungsltg.): 0V at rest, +12V when Rueckgabe / Teilgewinn-
	// Annahme / Spiel-starten is pressed. The Foul self-test row-3 check (FUN_1f3d:
	// RL & 0x2b == 0x20) only passes when bit5 is high and bits 0,1,3 are low at rest;
	// the old real-HW dump (reads 0x33 => physical 0xCC) was a FAULTED unit (stop2 +
	// return line flagged), so a healthy machine idles bit5 high (and bit3 low).
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_GAMBLE_LOW ) // ZE0 Risikotaste Leiter 1
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_GAMBLE_HIGH ) // ZE1 Risikotaste Leiter 2
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN) // ZE2
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SLOT_STOP2 ) // ZE3 Stop-Scheibe 2
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_GAMBLE_BOOK ) // ZE4 Serienuebernahme
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) // ZE5 gemeinsame Rueckfuehrungsltg der Tasten
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) // NC
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) // NC

	PORT_START("TZ4") //MATRIX-EINGAENGE
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN ) // physical idle high (firmware reads 0x00)

	PORT_START("TZ5") //MATRIX-EINGAENGE
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN ) // physical idle high (firmware reads 0x00)

	PORT_INCLUDE(stella8085_service)
INPUT_PORTS_END

void stella8085_state::dicemstr(machine_config &config)
{
	I8085A(config, m_maincpu, 10.240_MHz_XTAL / 2); // divider not verified
	m_maincpu->set_addrmap(AS_PROGRAM, &stella8085_state::large_program_map);
	m_maincpu->set_addrmap(AS_IO, &stella8085_state::io_map);
	m_maincpu->set_irq_acknowledge_callback(FUNC(stella8085_state::sound_irq_ack));

	I8255(config, m_ppi);
	m_ppi->out_pa_callback().set(FUNC(stella8085_state::io70));
	m_ppi->out_pb_callback().set(FUNC(stella8085_state::io71));
	m_ppi->out_pc_callback().set(FUNC(stella8085_state::sounddev));

	I8256(config, m_uart, 10.240_MHz_XTAL / 2); // divider not verified
	m_uart->int_callback().set_inputline(m_maincpu, I8085_INTR_LINE);

	RS232_PORT(config, m_rs232, default_rs232_devices, nullptr);
	m_uart->txd_handler().set(m_rs232, FUNC(rs232_port_device::write_txd));
	m_rs232->rxd_handler().set(m_uart, FUNC(i8256_device::write_rxd));
	// CTS is tied active on the board (the firmware drives the serial port with no
	// hardware flow control). Do NOT route it to the rs232 slot: an empty slot resets
	// CTS deasserted (high), which gates the i8256 transmitter so write_buffer never
	// drains and the firmware spins forever in its wait-for-TBE loop. Leave the i8256
	// CTS at its asserted (low) default so transmission works standalone like real HW.
	//m_rs232->cts_handler().set(m_uart, FUNC(i8256_device::write_cts));

	I8279(config, m_kdc, 10.240_MHz_XTAL / 4); // divider not verified
	m_kdc->out_sl_callback().set(FUNC(stella8085_state::kbd_sl_w));
	m_kdc->out_bd_callback().set(FUNC(stella8085_state::kbd_bd_w));
	m_kdc->out_disp_callback().set(FUNC(stella8085_state::disp_w));
	m_kdc->in_rl_callback().set(FUNC(stella8085_state::kbd_rl_r));
	m_kdc->out_irq_callback().set_inputline(m_maincpu, I8085_RST65_LINE);

	RTC62421(config, "rtc", 32.768_kHz_XTAL);

	SPEAKER(config, "mono").front_center();
	BEEP(config, m_beep)
		.add_route(ALL_OUTPUTS, "mono", 0.50);
}

void stella8085_state::doppelpot(machine_config &config)
{
	I8085A(config, m_maincpu, 6.144_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &stella8085_state::program_map);
	m_maincpu->set_addrmap(AS_IO, &stella8085_state::io_map);
	m_maincpu->set_irq_acknowledge_callback(FUNC(stella8085_state::sound_irq_ack));

	I8255(config, m_ppi);
	m_ppi->out_pa_callback().set(FUNC(stella8085_state::io70));
	m_ppi->out_pb_callback().set(FUNC(stella8085_state::io71));
	m_ppi->out_pc_callback().set(FUNC(stella8085_state::sounddev));

	I8256(config, m_uart, 6.144_MHz_XTAL / 2);
	m_uart->int_callback().set_inputline(m_maincpu, I8085_INTR_LINE);
	m_maincpu->in_inta_func().set(m_uart, FUNC(i8256_device::inta_r));
	m_uart->out_p2_callback().set(FUNC(stella8085_state::machine1_w)); //M1-4
	m_uart->in_p1_callback().set(FUNC(stella8085_state::lw_r));
	m_uart->out_p1_callback().set(FUNC(stella8085_state::machine2_w));

	// wheel stepper motors, each driven by a 2-bit coil pattern. The REEL only
	// provides gray-code decoding/position here; the light barrier is generated
	// in update_optics() from the per-wheel coded disc (DISC_PATTERN).
	REEL(config, m_motor[0], MPU3_48STEP_REEL, 96, 2, 0x00, 2);
	REEL(config, m_motor[1], MPU3_48STEP_REEL, 96, 2, 0x00, 2);
	REEL(config, m_motor[2], MPU3_48STEP_REEL, 96, 2, 0x00, 2);
	REEL(config, m_motor[3], MPU3_48STEP_REEL, 96, 2, 0x00, 2);
	REEL(config, m_motor[4], MPU3_48STEP_REEL, 96, 2, 0x00, 2);

	RS232_PORT(config, m_rs232, default_rs232_devices, nullptr);
	m_uart->txd_handler().set(m_rs232, FUNC(rs232_port_device::write_txd));
	m_rs232->rxd_handler().set(m_uart, FUNC(i8256_device::write_rxd));
	// CTS is tied active on the board (the firmware drives the serial port with no
	// hardware flow control). Do NOT route it to the rs232 slot: an empty slot resets
	// CTS deasserted (high), which gates the i8256 transmitter so write_buffer never
	// drains and the firmware spins forever in its wait-for-TBE loop. Leave the i8256
	// CTS at its asserted (low) default so transmission works standalone like real HW.
	//m_rs232->cts_handler().set(m_uart, FUNC(i8256_device::write_cts));

	I8279(config, m_kdc, 6.144_MHz_XTAL / 2);
	m_kdc->out_sl_callback().set(FUNC(stella8085_state::kbd_sl_w));
	m_kdc->out_bd_callback().set(FUNC(stella8085_state::kbd_bd_w));
	m_kdc->out_disp_callback().set(FUNC(stella8085_state::disp_w));
	m_kdc->in_rl_callback().set(FUNC(stella8085_state::kbd_rl_r));
	m_kdc->out_irq_callback().set_inputline(m_maincpu, I8085_RST65_LINE);

	config.set_default_layout(layout_adpservice);

	MC146818(config, "rtc", 32.768_kHz_XTAL);

	SPEAKER(config, "mono").front_center();
	BEEP(config, m_beep)
		.add_route(ALL_OUTPUTS, "mono", 0.50);
}

void stella8085_state::excellent(machine_config &config)
{
	doppelpot(config);
	m_maincpu->set_addrmap(AS_PROGRAM, &stella8085_state::program_4040_map);
	m_maincpu->set_addrmap(AS_IO, &stella8085_state::io_4040_map);
}

ROM_START( bahia )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "bahia_pr1", 0x0000, 0x1000, CRC(41e7f89c) SHA1(933334e2f78a91e24ec0132b8e7757da5a9d2e02) )
	ROM_LOAD( "bahia_pr2", 0x1000, 0x1000, CRC(ab06262a) SHA1(435f16002054f010e1349f2dbc998a2e5eb50c70) )
	ROM_LOAD( "bahia_pr3", 0x2000, 0x1000, CRC(6aecba71) SHA1(5a522329b0aeb7707014f3879adfbaf963aed27d) )
	ROM_LOAD( "bahia_pr4", 0x3000, 0x1000, CRC(bf6a989f) SHA1(bdd24b82f6f60ac42f83e5c4b68607c14929028f) )
	ROM_LOAD( "bahia_pr5", 0x4000, 0x1000, CRC(70622047) SHA1(a4e33bfd56c862ca2b130a96828d9d15eae7a5b2) )
	ROM_LOAD( "bahia_pr6", 0x7000, 0x1000, CRC(0373bee7) SHA1(4c0a31feab21872fee7ecd6a04933c0df050b99f) )
ROM_END

ROM_START( dicemstr ) // curiously hand-written stickers say F3 but strings in ROM are F2
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "stella_dice_master_f3_i.ice6",  0x0000, 0x8000, CRC(9897fb87) SHA1(bfb18c1370d9bd12ec61622c0ebbad5c0138e1d8) )
	ROM_LOAD( "stella_dice_master_f3_ii.icd6", 0x8000, 0x8000, CRC(9484cf3b) SHA1(e1104882eaba860ab984c1a37e2f97d4bed08829) ) // 0x0000 - 0x1fff is 0xff filled
ROM_END

ROM_START( dpplpot )
	ROM_REGION( 0x9000, "maincpu", 0 )
	ROM_LOAD( "doppelpot.ice6", 0x0000, 0x4000, CRC(b01d3307) SHA1(8364506e8169432ddec275ef5b53660c01dc209e) )
	ROM_LOAD( "doppelpot.icd6", 0x4000, 0x4000, CRC(153708cb) SHA1(3d15b115ec39c1df42d4437226e83413f495c4d9) )
	ROM_LOAD( "doppelpot.icc5", 0x8000, 0x1000, CRC(135dac6b) SHA1(10873ee64579245eac7069bf84d61550684e67de) )
ROM_END

ROM_START( dpplstrt )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "doppel_start_eprom1_2732.bin", 0x0000, 0x1000, CRC(0a90cc49) SHA1(87a2aaa85ecf0525473d02a2121d13c6615b7188) )
	ROM_LOAD( "doppel_start_eprom2_2732.bin", 0x1000, 0x1000, CRC(720c4262) SHA1(da7f6a399093e4596d84798423201e1445ac38a1) )
	ROM_LOAD( "doppel_start_eprom3_2732.bin", 0x2000, 0x1000, CRC(1d26e43a) SHA1(389a5398536097c3dc3e084f4635908aad17c62d) )
	ROM_LOAD( "doppel_start_eprom4_2732.bin", 0x3000, 0x1000, CRC(d0fa1fdd) SHA1(c56df831c0c112762636675a60a76a0f00732ab2) )
	ROM_LOAD( "doppel_start_eprom5_2732.bin", 0x4000, 0x1000, CRC(40079325) SHA1(9e9f5e3853b3b75c89cd9086814f8a762cc3643b) )
	ROM_LOAD( "doppel_start_eprom6_2732.bin", 0x7000, 0x1000, CRC(1b988121) SHA1(7886ad67d62db61588640f95efc679bc26220691) )
ROM_END

ROM_START( disc )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "disc_eprom1_2732.bin", 0x0000, 0x1000, CRC(b9d1f518) SHA1(3a49b248eeb77767e8274a8be523678d7b4aa7d0) )
	ROM_LOAD( "disc_eprom2_2732.bin", 0x1000, 0x1000, CRC(f55fba7c) SHA1(941f2653cb48836bb46f0903f64c3e9d32e67f46) )
	ROM_LOAD( "disc_eprom3_2732.bin", 0x2000, 0x1000, CRC(bd05e77a) SHA1(9e2b5ad6de3eb36cb1f588906a2a10e512b79ce8) )
	ROM_LOAD( "disc_eprom4_2732.bin", 0x3000, 0x1000, CRC(fa8dfac6) SHA1(7e8ba772218f4344070c4fa7e7bc5606b004ddc7) )
	ROM_LOAD( "disc_eprom5_2732.bin", 0x4000, 0x1000, CRC(d036733c) SHA1(f2912f9090b3737ddd1c0702f30a6817fd36ec2c) )
	ROM_LOAD( "disc_eprom6_2732.bin", 0x7000, 0x1000, CRC(d94d5f6e) SHA1(a27df116478b776c549c392297ffa4fdbb073514) )
ROM_END

ROM_START( disc2000 )
	ROM_REGION( 0x9000, "maincpu", 0 )
	ROM_LOAD( "disc2000.ice6", 0x0000, 0x4000, CRC(53a66005) SHA1(a5bb63abe8eb631a0fb09496ef6e0ee6c713985c) )
	ROM_LOAD( "disc2000.icd6", 0x4000, 0x4000, CRC(787b6708) SHA1(be990f95b6d04cbe0b9832603204f2a81b0ace3f) )
ROM_END

ROM_START( disc2001 )
	ROM_REGION( 0x9000, "maincpu", 0 )
	ROM_LOAD( "disc2001.ice6", 0x0000, 0x4000, CRC(4d128fe1) SHA1(2b9b0a1296ff77b281173fb0fcf667ed3e3ece2b) )
	ROM_LOAD( "disc2001.icd6", 0x4000, 0x4000, CRC(72f6560a) SHA1(3fdc3aaafcc2c185a19a27ccd511d8522fbe0c2e) )
ROM_END

ROM_START( disc3000 )
	ROM_REGION( 0x9000, "maincpu", 0 )
	ROM_LOAD( "disc3000.ice6", 0x0000, 0x4000, CRC(6e024e72) SHA1(7198c0cd844d4bc080b2d8654d32d53a04ce8bb4) )
	ROM_LOAD( "disc3000.icd6", 0x4000, 0x4000, CRC(ad88715a) SHA1(660f4044e8f24ad59767ce025966475f9fd56885) )
ROM_END

ROM_START( disciip )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "disc_ii_plus_1-f7m27256f1.ice6", 0x0000, 0x8000, CRC(b2d999f2) SHA1(cb961dfa7d6eec84e742261d6cf66a3e95715101) )
	ROM_LOAD( "disc_ii_plus_2-f7m27256f1.icd6", 0x8000, 0x8000, CRC(c87dd5ce) SHA1(721293fd9ba19bb58b657d6eabd3cd1c7dd74aac) )
ROM_END

ROM_START( discoly )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "olympia_pr1", 0x0000, 0x1000, CRC(531deb63) SHA1(6fddfc5791465c3bcdb554f207de391905085df9) )
	ROM_LOAD( "olympia_pr2", 0x1000, 0x1000, CRC(81e119b2) SHA1(a28eca9394e88b862b15e7bc117b0c4d01d4cf38) )
	ROM_LOAD( "olympia_pr3", 0x2000, 0x1000, CRC(2e156cef) SHA1(e5f145f3e4b7515b949fa7b570ca312c5dd12311) )
	ROM_LOAD( "olympia_pr4", 0x3000, 0x1000, CRC(8f51c072) SHA1(f23fd7d683a5d0765469de8910bb24d0db1b42e2) )
	ROM_LOAD( "olympia_pr5", 0x4000, 0x1000, CRC(f486c0da) SHA1(ebec6f66bffa1057f5fa9b4ab53cedd533036f0a) )
	ROM_LOAD( "olympia_pr6", 0x7000, 0x1000, CRC(e6830d26) SHA1(dbe7a39f24a1dcee298dae3ec1b2d6249a914262) )
ROM_END

ROM_START( discryl )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "disc_royal_1_m27256.ice6", 0x0000, 0x8000, CRC(b286c166) SHA1(08fecc3bf21013f8dbcc08fef3755757c7ff8053) )
	ROM_LOAD( "disc_royal_2_m27256.icd6", 0x8000, 0x8000, CRC(be2a96c2) SHA1(07efc914832fe549b69a2ec0de5fd5725502ee86) )
ROM_END

ROM_START( discrylb )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "disc_royal_dob._i.ice6", 0x0000, 0x8000, CRC(eafe92ca) SHA1(5dc172d7cd4efca7a49ac5884ff30fea7be02a30) )
	ROM_LOAD( "disc_royal_dob.ii.icd6", 0x8000, 0x8000, CRC(ad58476d) SHA1(4565156cac372f45058bce20006692e9afa53ebe) )
ROM_END

ROM_START( elitdisc )
	ROM_REGION( 0x9000, "maincpu", 0 )
	ROM_LOAD( "elitedisc.ice6", 0x0000, 0x4000, CRC(7f7a2f30) SHA1(01e3ce5fce2c9d51d3f4b8aab7dd67ed4b26d8f4) )
	ROM_LOAD( "elitedisc.icd6", 0x4000, 0x4000, CRC(e56f2360) SHA1(691a6762578daca6ce4581418761dcc07c291fab) )
ROM_END

ROM_START( excellnt )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "excellent.ice5", 0x0800, 0x0800, CRC(b4c573b5) SHA1(5b01b68b8abd48bd293bc9aa507c3285a6e7550f) BAD_DUMP ) // underdumped
	ROM_LOAD( "excellent.ice6", 0x1800, 0x0800, CRC(f1d53581) SHA1(7aef66149f3427b287d3e9d86cc198dc1ed40d7c) BAD_DUMP ) // underdumped
	ROM_LOAD( "excellent.icd5", 0x2800, 0x0800, CRC(912a5f59) SHA1(3df3ca7eaef8de8e13e93f6a1e6975f8da7ed7a1) BAD_DUMP ) // underdumped
	ROM_LOAD( "excellent.icd6", 0x3800, 0x0800, CRC(5a2b95b4) SHA1(b0d17b327664e8680b163c872109769c4ae42039) BAD_DUMP ) // underdumped
	ROM_LOAD( "excellent.icc5", 0x4800, 0x0800, CRC(ae424805) SHA1(14e12ceebd9fbf6eba96c168e8e7b797b34f7ca5) BAD_DUMP ) // underdumped
ROM_END

ROM_START( extrbltt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "extrablatt.ice6", 0x0000, 0x8000, CRC(6885cf89) SHA1(30acd5511fb73cb22ae4230fedcf40f385c0d261) )
	ROM_LOAD( "extrablatt.icd6", 0x8000, 0x8000, CRC(5c0cb9bd) SHA1(673d5f8dec7ccce1c4f39dce6be1e9d1ed699047) )
ROM_END

ROM_START( fullhous )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "merkur_full_house_ic1.ice6", 0x0000, 0x4000, CRC(4f984add) SHA1(5a31c96475fe12c4f19658133d97e6bf0536b776) )
	ROM_LOAD( "merkur_full_house_ic2.icd6", 0x4000, 0x4000, CRC(c0f393a0) SHA1(fa16db49d44e813e68701eb77284d04903cf3ec7) )
ROM_END

ROM_START( herzas )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "herz_as_nr1.ice6", 0x0000, 0x4000, CRC(dd4dbaac) SHA1(7fb3c8ea495d5bf989c4aa807ecbe5601c451a73) )
	ROM_LOAD( "herz_as_nr2.icd6", 0x4000, 0x4000, CRC(f2c6a0c4) SHA1(2dad5f79cb5b21905cbefd56b00db1cce1d0b920) )
	ROM_LOAD( "herz_as_nr3.icc5", 0x8000, 0x1000, CRC(1c8657e8) SHA1(836319901c77037c7f414cf0fddf5ab1bdf90ee5) )
ROM_END

ROM_START( herzasf8 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "herz_as_f8_1.ice6", 0x0000, 0x4000, CRC(830bada0) SHA1(8c1fc7e8433c986687b68f6de7610a624e9ac707) )
	ROM_LOAD( "herz_as_f8_2.icd6", 0x4000, 0x4000, CRC(77f88503) SHA1(671a7e819a0361101a30327179132f2661388b72) )
	ROM_LOAD( "herz_as_f8_3.icc5", 0x8000, 0x1000, CRC(b343bfac) SHA1(3772045fcaeb9a87459e481149f27873fc713ca7) )
ROM_END

ROM_START( herzasf1 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "herz_as_f10_1.ice6", 0x0000, 0x4000, CRC(e8342e8b) SHA1(e32ad013cdd1480d350ba2e6db18a4489f152301) )
	ROM_LOAD( "herz_as_f10_2.icd6", 0x4000, 0x4000, CRC(03ba2d03) SHA1(12434a3c862b30e40b7c3187066b25b2b7c4eaa6) )
	ROM_LOAD( "herz_as_f10_3.icc5", 0x8000, 0x1000, CRC(f67d2492) SHA1(a2daad380376d19cd9ca37f530a23c01b8d3ce5c) )
ROM_END

ROM_START( juwel )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "juwel.ice6", 0x0000, 0x8000, CRC(6fd9fd6a) SHA1(2ff982750d87be1bc7757bde706d9e329ac29785) )
	ROM_LOAD( "juwel.icd6", 0x8000, 0x8000, CRC(a9ec9e36) SHA1(f7a2b5866988116e0bbeb8a120cae9083d651c5b) )
ROM_END

ROM_START( karoas )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "karoas.ice6", 0x0000, 0x8000, CRC(71c4c39d) SHA1(b188896838a788d5bfc7b18f1bb423a06fe5fcc6) )
	ROM_LOAD( "karoas.icd6", 0x8000, 0x8000, CRC(e1b131bd) SHA1(dc2fbfaf86fa5b161d17a563eae2bc8fc4d19395) )
ROM_END

ROM_START( kniffi )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "kniffi.ice6", 0x0000, 0x8000, CRC(57df5d69) SHA1(78bc9cabf0b4bec5f8c2578d55011f0adc034798) )
	ROM_LOAD( "kniffi.icd6", 0x8000, 0x8000, CRC(1c129cec) SHA1(bad22f18b94c16dba36995ff8daf4d48f4d082a2) )
ROM_END

ROM_START( m21point )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD( "21_point_27c256_f3_i.ice6",  0x0000, 0x8000, CRC(c2b7b030) SHA1(affb317da2f892213556937fa8857186dccac58a) )
	ROM_LOAD( "21_point_27c256_f3_ii.icd6", 0x8000, 0x8000, CRC(a466591c) SHA1(c481fc91055b41c9976ff86785f7ee0ce631bd69) )
ROM_END

ROM_START( macao )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mega_macao_f1_1.ice6", 0x0000, 0x8000, CRC(b16c9349) SHA1(f07c3dd215bccab088741f95972489284d6a4db9) )
	ROM_LOAD( "mega_macao_f1_2.icd6", 0x8000, 0x8000, CRC(4df216e6) SHA1(28b3ad213f3af9a472c5e7de1c139399677dd825) )
ROM_END

ROM_START(mas)
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "mega_as_f5_eprom1_27256.ice6", 0x0000, 0x8000, CRC(17e22e95) SHA1(6fbc11c41c99ee4aac3dcad6647cede25b73f3da) )
	ROM_LOAD( "mega_as_f5_eprom2_27256.icd6", 0x8000, 0x8000, CRC(12453d57) SHA1(c6c9fa39bdfc7801471bed57e365e37bb02f50b0) )
ROM_END

ROM_START( mastro )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "merkur_astro_pr1.ice6", 0x0000, 0x8000, CRC(b2d61886) SHA1(12d2aed9315fc311929edeacd23a38bceadb69f8) )
	ROM_LOAD( "merkur_astro_pr2.icd6", 0x8000, 0x8000, CRC(1e0e42d0) SHA1(46b1eec99331f6656f7cb1542207a79091bce9d9) )
ROM_END

ROM_START( mbistro )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "merkur_bistro_f1.ice6", 0x0000, 0x8000, CRC(e497eeef) SHA1(a5f621627ee80c11697ee5aa9fcd99023e7b6479) )
	ROM_LOAD( "merkur_bistro_f1.icd6", 0x8000, 0x8000, NO_DUMP )
ROM_END

ROM_START( mclub )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "merkur_club_f1_i_st_m27256f1_original.bin",  0x0000, 0x8000, CRC(c78b19b2) SHA1(79aeeee6e82bf987e2aa936575e1e1b251b1a425) )
	ROM_LOAD( "merkur_club_f1_ii_st_m27256f1_original.bin", 0x8000, 0x8000, CRC(ad3b7d5f) SHA1(317909be8a7853bf83f4f3a2497b1f38a0d954c9) )
ROM_END

ROM_START( mmax )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "mega_max_f4_i_st_m27256f1_ice6",  0x0000, 0x8000, CRC(91aa91ba) SHA1(f6c3a6e2e2edeaa79cf0bcdb6af01ddd50eb5488) )
	ROM_LOAD( "mega_max_f4_ii_st_m27256f1_icd6", 0x8000, 0x8000, CRC(6120080b) SHA1(16209bfe8e75a165ec1e8a5bf2ec7fa078725380) )
ROM_END

ROM_START( mtrio )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "mega_trio_f1_ic1.ice6", 0x0000, 0x8000, CRC(b8c2fc4c) SHA1(ddecd608286eb1f3efc6fccce8806a74ad7ce4b8))
	ROM_LOAD( "mega_trio_f1_ic2.icd6", 0x8000, 0x8000, CRC(9d97fd8c) SHA1(c398610e14c33985a186ae816b759cfdd2b0c6fa))
ROM_END

ROM_START( rasant )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rasant_pr_1.ice6", 0x0000, 0x8000, CRC(6abef716) SHA1(8ef2999f6c72f7fb134bfa4ad72ab7be7d12af27) )
	ROM_LOAD( "rasant_pr_2.icd6", 0x8000, 0x8000, CRC(c3a95f74) SHA1(87805ca63a93cc9012e7f2ab4d808c48ba93c919) )
ROM_END

ROM_START( sesam )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "sesam_1_27128.ice6", 0x0000, 0x4000, CRC(29a07575) SHA1(d7e3355e32fcb7a064d8d0fd9b4904be860f7eed) )
	ROM_LOAD( "sesam_2_27128.icd6", 0x4000, 0x4000, CRC(dac087d0) SHA1(0776e3db14c9b88140887237b5b3d71396b2e6e9) )
	ROM_LOAD( "sesam_3_2732.icc5",  0x8000, 0x1000, CRC(ec6a2eac) SHA1(6608dd6f477db0df7de3e4262c5b7bcdf1af7ef4) )
ROM_END

ROM_START( sherzas )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "super_herz_as_1.ice6", 0x0000, 0x8000, CRC(4212cfaa) SHA1(c428a2a59ae73a92abd08e2b9b2f4feb8ae4dc31) )
	ROM_LOAD( "super_herz_as_2.icd6", 0x8000, 0x8000, CRC(c5cab1a1) SHA1(d3425c94d898369ad22e969a00697e2f0a1305f9) )
ROM_END

ROM_START( sjackpot )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "super_jackpot_i.ice6",  0x0000, 0x4000, CRC(3f14364a) SHA1(4711e2d1aa76a08478177ad7b1f5509b11649f9d) )
	ROM_LOAD( "super_jackpot_ii.icd6", 0x4000, 0x4000, CRC(984d4ca1) SHA1(1da5533f06fb7a1ab8f221c5a58c1afafdd5f862) )
ROM_END

ROM_START( sprmlti )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "super_multi_1.ice6", 0x0000, 0x8000, CRC(fcf126ba) SHA1(89dfd10b6529a92b55d2585c0aa3d0c6b1751550) )
	ROM_LOAD( "super_multi_2.icd6", 0x8000, 0x8000, CRC(0b7a8352) SHA1(ac03b226296085f43634ba96e3e390d3e44c1760) )
ROM_END

ROM_START( sprmltib )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "super_multi_dob_pr1.ice6", 0x0000, 0x8000, CRC(2eb21e6b) SHA1(214f0f26f03551ecda88a64d4e1d75a49f376aae) )
	ROM_LOAD( "super_multi_dob_pr2.icd6", 0x8000, 0x8000, CRC(e3f14918) SHA1(8ba7fc80044b5d27005a53ddbf9e928c74c25d48) )
ROM_END

ROM_START( superpro )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "super_pro_f2_1.ice6", 0x0000, 0x8000, CRC(3294f651) SHA1(3c2dcecda4cbebf29246bbc7705430e96dcafbae) )
	ROM_LOAD( "super_pro_f2_2.icd6", 0x8000, 0x8000, CRC(82802b74) SHA1(8e6ebc429d4e1ccfc5ed6a3bb6fb1747a6a7187a) )
ROM_END

ROM_START( v4assef1 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "venus_4_asse_f1_i.ice6",  0x0000, 0x8000, CRC(29fd7f6a) SHA1(84a8f744e189f0645410c4b7ac36b65f30aa1cc9) )
	ROM_LOAD( "venus_4_asse_f1_ii.icd6", 0x8000, 0x8000, CRC(314dc36c) SHA1(d076651910c713326fe5f0c617ae6e74b6c15334) )
ROM_END

ROM_START( v4assef2 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "4asse_f2_1_27256.ice6", 0x0000, 0x8000, CRC(577a1a55) SHA1(b13ac1b761fba6b7e25c18ad3c1edeef8c892089) )
	ROM_LOAD( "4asse_f2_2_27256.icd6", 0x8000, 0x8000, CRC(4f921c1a) SHA1(a6cbca333e29e490306820e2df6c9579a67941c8) )
ROM_END

ROM_START( vmulti )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "venus_multi_1_2732.bin", 0x0000, 0x1000, CRC(3b269798) SHA1(511fb8a86008c124de37d5359681d8379d25891d) )
	ROM_LOAD( "venus_multi_2_2732.bin", 0x1000, 0x1000, CRC(67e22cec) SHA1(8639cb4496012d9f20f2ece89f15290d017ece2e) )
	ROM_LOAD( "venus_multi_3_2732.bin", 0x2000, 0x1000, CRC(64bd9bd8) SHA1(c878bdd147e011f4191b5613455648852d395bf1) )
	ROM_LOAD( "venus_multi_4_2732.bin", 0x3000, 0x1000, CRC(b47e70c1) SHA1(a52cd6568dee16f917c92a41693abd91c4dc2d8c) )
	ROM_LOAD( "venus_multi_5_2732.bin", 0x4000, 0x1000, CRC(c2905422) SHA1(5c8e3f0440671dc16df32b599239b0435f120778) )
	ROM_LOAD( "venus_multi_6_2732.bin", 0x7000, 0x1000, CRC(09dd81e7) SHA1(35e9a96d913678a75851a9bf7e7349f93e337805) )
ROM_END

} // anonymous namespace

GAMEL( 1982, excellnt,        0, excellent, servicem, stella8085_state, empty_init, ROT0, "ADP",    "Excellent",         MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1983, bahia,           0, excellent, servicem, stella8085_state, empty_init, ROT0, "ADP",    "Bahia",             MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1984, disc,            0, excellent, disc,     stella8085_state, empty_init, ROT0, "ADP",    "Disc",              MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1985, dpplstrt,        0, excellent, servicem, stella8085_state, empty_init, ROT0, "Nova",   "Doppelstart",       MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1986, discoly,         0, excellent, disc,     stella8085_state, empty_init, ROT0, "ADP",    "Disc Olympia",      MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1986, dpplpot,         0, doppelpot, disc,     stella8085_state, empty_init, ROT0, "Nova",   "Doppelpot",         MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1986, elitdisc,        0, doppelpot, disc,     stella8085_state, empty_init, ROT0, "ADP",    "Elite Disc",        MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1986, sjackpot,        0, doppelpot, disc,     stella8085_state, empty_init, ROT0, "Nova",   "Super Jackpot",     MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1986, vmulti,          0, excellent, disc,     stella8085_state, empty_init, ROT0, "Venus",  "Multi",             MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1987, disc2000,        0, doppelpot, disc,     stella8085_state, empty_init, ROT0, "ADP",    "Disc 2000",         MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1987, disc2001,        0, doppelpot, disc,     stella8085_state, empty_init, ROT0, "ADP",    "Disc 2001",         MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1987, fullhous,        0, doppelpot, disc,     stella8085_state, empty_init, ROT0, "Merkur", "Full House",        MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1987, herzas,   herzasf1, doppelpot, servicem, stella8085_state, empty_init, ROT0, "ADP",    "Herz As",           MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1987, herzasf8, herzasf1, doppelpot, servicem, stella8085_state, empty_init, ROT0, "ADP",    "Herz As (F8)",      MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1987, herzasf1,        0, doppelpot, servicem, stella8085_state, empty_init, ROT0, "ADP",    "Herz As (F10)",     MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1987, kniffi,          0, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "Nova",   "Kniffi",            MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1987, rasant,          0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "Venus",  "Rasant",            MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1987, sesam,           0, doppelpot, disc,     stella8085_state, empty_init, ROT0, "Merkur", "Sesam",             MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1987, sprmlti,  sprmltib, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "Venus",  "Super Multi",       MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1987, sprmltib,        0, doppelpot, servicem, stella8085_state, empty_init, ROT0, "Venus",  "Super Multi (DOB)", MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1988, extrbltt,        0, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "ADP",    "Extrablatt",        MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1988, juwel,           0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "ADP",    "Juwel",             MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1988, mastro,          0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "ADP",    "Astro (Merkur)",    MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1988, sherzas,         0, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "Merkur", "Super Herz As",     MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1989, disc3000,        0, doppelpot, disc,     stella8085_state, empty_init, ROT0, "ADP",    "Disc 3000",         MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1989, disciip,         0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "ADP",    "Disc II Plus",      MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1989, discryl,  discrylb, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "ADP",    "Disc Royal",        MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1989, discrylb,        0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "ADP",    "Disc Royal (DOB)",  MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_disc2000 )
GAMEL( 1989, mas,             0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "MEGA",   "As",                MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1990, v4assef1, v4assef2, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "Venus",  "4 Asse (F1)",       MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1990, v4assef2,        0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "Venus",  "4 Asse (F2)",       MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1991, macao,           0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "MEGA",   "Macao",             MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1991, mbistro,         0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "MEGA",   "Bistro",            MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1991, mclub,           0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "Merkur", "Club",              MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1991, superpro,        0, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "Merkur", "Super Pro",         MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1992, m21point,        0, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "MEGA",   "21 Point",          MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1992, karoas,          0, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "ADP",    "Karo As",           MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1992, mmax,            0, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "MEGA",   "Max",               MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
GAMEL( 1992, mtrio,           0, dicemstr,  disc,     stella8085_state, empty_init, ROT0, "MEGA",   "Trio",              MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
// 'STELLA DICE MASTER F2' and 'COPYRIGHT BY ADP LUEBBECKE GERMANY 1993' in ROM
GAMEL( 1993, dicemstr,        0, dicemstr,  servicem, stella8085_state, empty_init, ROT0, "Stella", "Dice Master",       MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK, layout_adpservice )
