// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/*

    Intel 8256(AH) Multifunction microprocessor support controller emulation

*/

#include "emu.h"
#include "i8256.h"

#define VERBOSE 1
#include "logmacro.h"


namespace {

enum // MUART REGISTERS
{
	I8256_REG_CMD1,
	I8256_REG_CMD2,
	I8256_REG_CMD3,
	I8256_REG_MODE,
	I8256_REG_PORT1C,
	I8256_REG_INTEN,
	I8256_REG_INTAD,
	I8256_REG_BUFFER,
	I8256_REG_PORT1,
	I8256_REG_PORT2,
	I8256_REG_TIMER1,
	I8256_REG_TIMER2,
	I8256_REG_TIMER3,
	I8256_REG_TIMER4,
	I8256_REG_TIMER5,
	I8256_REG_STATUS,
};

enum
{
	I8256_CMD1_FRQ,
	I8256_CMD1_8086,
	I8256_CMD1_BITI,
	I8256_CMD1_BRKI,
	I8256_CMD1_S0,
	I8256_CMD1_S1,
	I8256_CMD1_L0,
	I8256_CMD1_L1
};

enum
{
	I8256_STOP_1,
	I8256_STOP_15,
	I8256_STOP_2,
	I8256_STOP_075
};

// Character length encoding: L1:L0 = 0:0 -> 8, 0:1 -> 7, 1:0 -> 6, 1:1 -> 5
constexpr int CHAR_LEN[4] = { 8, 7, 6, 5 };

enum
{
	I8256_CMD2_B0,
	I8256_CMD2_B1,
	I8256_CMD2_B2,
	I8256_CMD2_B3,
	I8256_CMD2_C0,
	I8256_CMD2_C1,
	I8256_CMD2_EVEN_PARITY,
	I8256_CMD2_PARITY_ENABLE
};

constexpr int BAUD_RATES[16] = { 0, 0, 0, 19200, 9600, 4800, 2400, 1200, 600, 300, 200, 150, 110, 100, 75, 50 };

constexpr int SYS_CLOCK_DIVIDER[4] = {5,3,2,1};

enum
{
	I8256_CMD3_RST,
	I8256_CMD3_TBRK,
	I8256_CMD3_SBRK,
	I8256_CMD3_END,
	I8256_CMD3_NIE,
	I8256_CMD3_IAE,
	I8256_CMD3_RxE,
	I8256_CMD3_SET
};

enum
{
	I8256_INT_TIMER1,
	I8256_INT_TIMER2,
	I8256_INT_EXTINT,
	I8256_INT_TIMER3,
	I8256_INT_RX,
	I8256_INT_TX,
	I8256_INT_TIMER4,
	I8256_INT_TIMER5
};

const char timer_interrupt[5] = {I8256_INT_TIMER1, I8256_INT_TIMER2, I8256_INT_TIMER3, I8256_INT_TIMER4, I8256_INT_TIMER5};

enum
{
	I8256_MODE_P2C0,
	I8256_MODE_P2C1,
	I8256_MODE_P2C2,
	I8256_MODE_CT2,
	I8256_MODE_CT3,
	I8256_MODE_T5C,
	I8256_MODE_T24,
	I8256_MODE_T35
};

enum // Upper / Lower
{
	I8256_PORT2C_II,
	I8256_PORT2C_IO,
	I8256_PORT2C_OI,
	I8256_PORT2C_OO,
	I8256_PORT2C_HI,
	I8256_PORT2C_HO,
	I8256_PORT2C_DNU,
	I8256_PORT2C_TEST
};

enum
{
	I8256_STATUS_FRAMING_ERROR,
	I8256_STATUS_OVERRUN_ERROR,
	I8256_STATUS_PARITY_ERROR,
	I8256_STATUS_BREAK,
	I8256_STATUS_TR_EMPTY,
	I8256_STATUS_TB_EMPTY,
	I8256_STATUS_RB_FULL,
	I8256_STATUS_INT
};

enum
{
	I8256_MOD_DSC,
	I8256_MOD_TME,
	I8256_MOD_RS0,
	I8256_MOD_RS1,
	I8256_MOD_RS2,
	I8256_MOD_RS3,
	I8256_MOD_RS4,
	I8256_MOD_0
};
} // anonymous namespace

DEFINE_DEVICE_TYPE(I8256, i8256_device, "intel_8256", "Intel 8256AH Multifunction microprocessor support controller")

i8256_device::i8256_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, I8256, tag, owner, clock), // removed device_serial_interface
	m_in_inta_cb(*this, 0),
	m_out_int_cb(*this),
	m_in_extint_cb(*this, 0),
	m_txd_handler(*this),
	m_rts_handler(*this),
	m_dtr_handler(*this),
	m_rxc_handler(*this),
	m_txc_handler(*this),
	m_in_p2_cb(*this, 0),
	m_out_p2_cb(*this),
	m_in_p1_cb(*this, 0),
	m_out_p1_cb(*this),
	m_rxc(0),
	m_rxd(1),
	m_cts(0),  // Initialize to 0 (ready to send) - CTS is active low
	m_txc(0),
	m_timer(nullptr),
	m_txd(-1)  // Initialize to impossible value so first output_txd call triggers
{
}

void i8256_device::device_start()
{
	// internal register
	save_item(NAME(m_command1));
	save_item(NAME(m_command2));
	save_item(NAME(m_command3));
	save_item(NAME(m_mode));
	save_item(NAME(m_port1_control));
	save_item(NAME(m_interrupts));
	save_item(NAME(m_current_interrupt_level));
	save_item(NAME(m_rx_buffer));
	save_item(NAME(m_tx_buffer));
	save_item(NAME(m_tx_buffer_full));
	save_item(NAME(m_port1_int));
	save_item(NAME(m_port2_int));
	save_item(NAME(m_timers));
	save_item(NAME(m_status));
	save_item(NAME(m_modification));

	save_item(NAME(m_parity));
	save_item(NAME(m_stop_bits_mode));
	save_item(NAME(m_br_factor));
	save_item(NAME(m_baud_sel));
	save_item(NAME(m_data_bits));
	save_item(NAME(m_bit_accumulator));

	save_item(NAME(m_tx_shift));
	save_item(NAME(m_tx_state));
	save_item(NAME(m_tx_bits));
	save_item(NAME(m_tx_parity));
	save_item(NAME(m_tx_counter));
	save_item(NAME(m_txd));
	save_item(NAME(m_cts));

	save_item(NAME(m_rx_shift));
	save_item(NAME(m_rx_state));
	save_item(NAME(m_rx_bits));
	save_item(NAME(m_rx_parity));
	save_item(NAME(m_rx_counter));
	save_item(NAME(m_rxd));
	save_item(NAME(m_rxc));

	save_item(NAME(m_internal_txc));

	m_timer = timer_alloc(FUNC(i8256_device::timer_check), this);

	memset(m_timers, 0, sizeof(m_timers));
}

void i8256_device::device_reset()
{
	m_command1 = 0;
	m_command2 = 0;
	m_command3 = 0;
	m_mode = 0;
	m_port1_control = 0;
	m_interrupts = 0;
	m_modification = 0;

	m_tx_buffer = 0;
	m_tx_buffer_full = false;
	m_rx_buffer = 0;
	m_port1_int = 0;
	m_port2_int = 0;

	m_current_interrupt_level = 0;
	m_out_int_cb(CLEAR_LINE);

	m_status = 0x30; // TRE and TBE

	m_tx_shift = 0;
	m_tx_state = STATE_IDLE;
	m_tx_bits = 0;
	m_tx_parity = 0;
	m_tx_counter = 0;
	m_internal_txc = 0;
	output_txd(1);

	m_rx_shift = 0;
	m_rx_state = STATE_IDLE;
	m_rx_bits = 0;
	m_rx_parity = 0;
	m_rx_counter = 0;

	m_baud_sel = 0;
	m_br_factor = 1;
	m_data_bits = 8;
	m_stop_bits_mode = I8256_STOP_1;
	m_parity = PARITY_NONE;
	m_bit_accumulator = 0;
	m_rx_accumulator = 0;

	m_txd = -1;  // Force TxD update on next call
	output_txd(1);  // TxD idle = high

	// Assert RTS and DTR to indicate device is ready for communication
	m_rts_handler(1);
	m_dtr_handler(1);

	reset_timer();
}

uint8_t i8256_device::acknowledge()
{
	LOG("i8256_device::acknowledge %d\n", m_current_interrupt_level);
	if (BIT(m_command3,I8256_CMD3_IAE) == 0)
		return 0x00;

	const uint8_t vector = m_current_interrupt_level;

	if (!machine().side_effects_disabled())
	{
		m_out_int_cb(CLEAR_LINE);
		m_status &= ~(1 << I8256_STATUS_INT);
		m_current_interrupt_level = 0;
	}

	if (BIT(m_command1,I8256_CMD1_8086)) // 8086 mode, TODO: only on second INTA
		return 0x40 | vector;
	else
		return 0xc7 | (vector << 3); // 8085 mode c7 = rst 0, ff = rst 7
}

void i8256_device::reset_timer()
{
	int divider = 64; // Use 16kHz for serial (default)
	// For internal baud rate generator (serial), always use 16kHz regardless of FRQ
	// FRQ bit only affects timer countdown operations, not serial baud rate
	
	const attotime TIME = attotime::from_hz((clock() / SYS_CLOCK_DIVIDER[(m_command2 & 0x30) >> 4]) / divider);
	LOG("i8256 Timer frequency: %d Hz (clock=%d, divider=%d)\n", (clock() / SYS_CLOCK_DIVIDER[(m_command2 & 0x30) >> 4]) / divider, clock(), divider);
	m_timer->adjust(TIME, 0, TIME);
}

void i8256_device::gen_interrupt(uint8_t level)
{
	LOG("i8256_device::gen_interrupt %d\n", level);
	if (BIT(m_interrupts, level))
	{
		m_current_interrupt_level = level;
		m_out_int_cb(ASSERT_LINE);
		m_status |= (1 << I8256_STATUS_INT);
	}
}

TIMER_CALLBACK_MEMBER(i8256_device::timer_check)
{
	bool t24 = BIT(m_mode, I8256_MODE_T24);
	bool t35 = BIT(m_mode, I8256_MODE_T35);
	for (int i = 0; i < 5; ++i)
	{
		if ((i == 1 && t24) || (i == 2 && t35))
		{
			// cascaded low timer
			int high_index = (i == 1) ? 3 : 4;
			int int_level = (i == 1) ? I8256_INT_TIMER4 : I8256_INT_TIMER5;
			if (m_timers[i] > 0)
			{
				m_timers[i]--;
				if (m_timers[i] == 0)
				{
					if (m_timers[high_index] > 0)
					{
						m_timers[high_index]--;
						m_timers[i] = 255;
					}
					if (m_timers[high_index] == 0 ) 
						gen_interrupt(int_level);
				}
			}
		}
		else if (!((i == 3 && t24) || (i == 4 && t35)))
		{
			// normal timer, not cascaded high
			if (m_timers[i] > 0)
			{
				m_timers[i]--;
				if (m_timers[i] == 0 && BIT(m_interrupts, timer_interrupt[i]))
				{
					// For Timer2, only trigger if BITI=0
					if (i == I8256_INT_TIMER2 && BIT(m_command1, I8256_CMD1_BITI))
						continue;

					gen_interrupt(timer_interrupt[i]);
				}
			}
		}
	}

	// internal baud rate generator tick when baud_sel >= 3
	if (m_baud_sel >= 3 && BAUD_RATES[m_baud_sel] > 0)
	{
		// Timer runs at 16 kHz for serial operations
		
		// TX: Use fractional bit timing accumulator for precise output
		m_bit_accumulator += 16000;  // Add timer frequency
		while (m_bit_accumulator >= BAUD_RATES[m_baud_sel])
		{
			m_bit_accumulator -= BAUD_RATES[m_baud_sel];
			transmit_clock();
		}
		
		// RX: Call at 16 kHz and let counter divide down to sample rate
		// This keeps sampling synchronized with the 16 kHz timer, not clustering
		receive_clock();
		
		// Always pulse the clock outputs for external devices
		m_rxc_handler(0); m_rxc_handler(1);
		m_txc_handler(0); m_txc_handler(1);
	}
}

void i8256_device::output_txd(int state)
{
		LOG("i8256 TxD output: %d (handler=%p)\n", state, (void*)&m_txd_handler);
		m_txd = state;
		LOG("i8256 About to call TxD handler with %d\n", m_txd);
		m_txd_handler(m_txd);
		LOG("i8256 TxD handler returned\n");
}

void i8256_device::receive_clock()
{
	if (!BIT(m_command3, I8256_CMD3_RxE))
		return;

	// Use accumulator for precise baud rate timing, called at 16 kHz
	m_rx_accumulator += 16000;

	switch (m_rx_state)
	{
	case STATE_IDLE:
		if (!m_rxd)
		{
			LOG("i8256 RX: Start bit detected\n");
			m_rx_state = STATE_START;
			m_rx_accumulator = 16000;  // Start accumulating from first timer tick
			m_rx_counter = 0;
		}
		break;

	case STATE_START:
		// Sample start bit at middle (after ~half bit period)
		// Baud rate = 4800, half period = 4800/2 = 2400 per bit
		if (m_rx_accumulator >= (BAUD_RATES[m_baud_sel] / 2))
		{
			if (m_rxd && !BIT(m_modification, I8256_MOD_DSC))
			{
				LOG("i8256 RX: False start bit detected, aborting\n");
				m_rx_state = STATE_IDLE;
				m_rx_accumulator = 0;
				break;
			}
			LOG("i8256 RX: Confirmed start bit, begin data\n");
			m_rx_state = STATE_DATA;
			// Don't reset accumulator - continue counting to sample first data bit at correct time
			// We'll sample each data bit centered within the bit period
			m_rx_shift = 0;
			m_rx_parity = 0;
			m_rx_bits = 0;
			m_rx_counter = 0;
		}
		break;

	case STATE_DATA:
		// Sample each data bit (every full bit period)
		if (m_rx_accumulator >= BAUD_RATES[m_baud_sel])
		{
			m_rx_accumulator -= BAUD_RATES[m_baud_sel];  // Keep fractional part, don't reset to 0
			m_rx_shift |= (m_rxd ? 1 : 0) << m_rx_bits;
			m_rx_parity ^= m_rxd;
			m_rx_bits++;
			LOG("i8256 RX DATA: bit %d = %d (acc=%d)\n", m_rx_bits-1, m_rxd, m_rx_accumulator);
			if (m_rx_bits == m_data_bits)
				m_rx_state = (m_parity != PARITY_NONE) ? STATE_PARITY : STATE_STOP;
		}
		break;

	case STATE_PARITY:
		if (m_rx_accumulator >= BAUD_RATES[m_baud_sel])
		{
			m_rx_accumulator -= BAUD_RATES[m_baud_sel];
			int expected = (m_parity == PARITY_ODD) ? !m_rx_parity : m_rx_parity;
			if (m_rxd != expected)
			{
				m_status |= (1 << I8256_STATUS_PARITY_ERROR);
				LOG("i8256 RX PARITY: ERROR (got %d, expected %d)\n", m_rxd, expected);
			}
			else
			{
				m_status &= ~(1 << I8256_STATUS_PARITY_ERROR);
				LOG("i8256 RX PARITY: OK\n");
			}
			m_rx_state = STATE_STOP;
			m_rx_bits = 0;
		}
		break;

	case STATE_STOP:
		if (m_rx_accumulator >= BAUD_RATES[m_baud_sel])
		{
			m_rx_accumulator -= BAUD_RATES[m_baud_sel];
			m_rx_bits++;
			int stop_count = (m_stop_bits_mode == I8256_STOP_2) ? 2 : 1;
			if (m_rx_bits >= stop_count)
			{
				if (!BIT(m_modification, I8256_MOD_TME))
				{
					if (!m_rxd)
						m_status |= (1 << I8256_STATUS_FRAMING_ERROR);
					else
						m_status &= ~(1 << I8256_STATUS_FRAMING_ERROR);
				}
				if (m_status & (1 << I8256_STATUS_RB_FULL))
				{
					m_status |= (1 << I8256_STATUS_OVERRUN_ERROR);
					LOG("i8256 RX: OVERRUN ERROR\n");
				}
				else
				{
					m_rx_buffer = m_rx_shift & ((1 << m_data_bits) - 1);
					m_status |= (1 << I8256_STATUS_RB_FULL);
					LOG("i8256 RX: Received 0x%02x, generating RX interrupt\n", m_rx_buffer);
					gen_interrupt(I8256_INT_RX);
				}
				m_rx_state = STATE_IDLE;
				m_rx_accumulator = 0;
			}
		}
		break;
	}
}

void i8256_device::transmit_clock()
{
	switch (m_tx_state)
	{
	case STATE_IDLE:
		if (m_tx_buffer_full && !m_cts)
		{
			LOG("i8256 TX: Starting transmission of 0x%02x (CTS=%d)\n", m_tx_buffer, m_cts);
			m_tx_shift = m_tx_buffer;
			m_tx_buffer_full = false;
			m_status |= (1 << I8256_STATUS_TB_EMPTY);
			m_status &= ~(1 << I8256_STATUS_TR_EMPTY);
			gen_interrupt(I8256_INT_TX); // TBE interrupt
			m_tx_bits = 0;
			m_tx_parity = 0;
			output_txd(0); // start bit
			m_tx_state = STATE_DATA;
		}
		else
		{
			if (m_tx_buffer_full && m_cts)
				LOG("i8256 TX: Blocked - CTS=%d, buffer_full=%d\n", m_cts, m_tx_buffer_full);
			output_txd(1);
		}
		break;

	case STATE_DATA:
		if (m_tx_bits < m_data_bits)
		{
			int bit = (m_tx_shift >> m_tx_bits) & 1;
			output_txd(bit);
			m_tx_parity ^= bit;
			m_tx_bits++;
			LOG("i8256 TX DATA: bit %d = %d\n", m_tx_bits-1, bit);
		}
		else if (m_parity != PARITY_NONE)
		{
			int parity_bit = (m_parity == PARITY_ODD) ? !m_tx_parity : m_tx_parity;
			output_txd(parity_bit);
			LOG("i8256 TX PARITY: %d\n", parity_bit);
			m_tx_state = STATE_STOP;
			m_tx_bits = 0;
		}
		else
		{
			output_txd(1); // first stop bit
			LOG("i8256 TX STOP: 1\n");
			m_tx_state = STATE_STOP;
			m_tx_bits = 0;
		}
		break;

	case STATE_STOP:
		m_tx_bits++;
		const int stop_count = (m_stop_bits_mode == I8256_STOP_2) ? 2 : 1; // 1.5 and 0.75 treated as 1 here
		if (m_tx_bits >= stop_count)
		{
			LOG("i8256 TX: Transmission complete\n");
			m_status |= (1 << I8256_STATUS_TR_EMPTY);
			gen_interrupt(I8256_INT_TX); // TRE interrupt
			m_tx_state = STATE_IDLE;
			m_tx_bits = 0;
		}
		break;
	}
}

uint8_t i8256_device::read(offs_t offset)
{
	// In the 8-bit mode, AD0-AD3 are used to select the proper register, while AD1-AD4 are used in the 16-bit mode.
	// AD4 in the 8-bit mote is ignored as an address, while AD0 in the 16-bit mode is used as a second chip select, active low.
	if (BIT(m_command1,I8256_CMD1_8086))
		offset = offset >> 1;

	u8 reg = offset & 0x0f;

	switch (reg)
	{
		case I8256_REG_CMD1:
			return m_command1;
		case I8256_REG_CMD2:
			return m_command2;
		case I8256_REG_CMD3:
			return m_command3 & 0x76; // When command Register 3 is read, bits 0, 3, and 7 will always be zero.
		case I8256_REG_MODE:
		   return m_mode;
		case I8256_REG_PORT1C:
			return m_port1_control;
		case I8256_REG_INTEN:
			return m_interrupts;
		case I8256_REG_INTAD:
			if (!machine().side_effects_disabled())
			{
				m_out_int_cb(CLEAR_LINE);
				m_status &= ~(1 << I8256_STATUS_INT);
			}
			return m_current_interrupt_level*4;
		case I8256_REG_BUFFER:
			if (!machine().side_effects_disabled())
			{
				LOG("I8256 RX BUFFER read: 0x%02x\n", m_rx_buffer);
				m_status &= ~(1 << I8256_STATUS_RB_FULL);
			}
			return m_rx_buffer;
		case I8256_REG_PORT1:
			return m_port1_int;
		case I8256_REG_PORT2:
			return m_port2_int;
		case I8256_REG_TIMER1:
		case I8256_REG_TIMER2:
		case I8256_REG_TIMER3:
		case I8256_REG_TIMER4:
		case I8256_REG_TIMER5:
			return m_timers[reg-10];
		case I8256_REG_STATUS:
			return m_status;
		default:
			logerror("I8256 Read unmapped register: %u\n", reg);
			return 0xff;
	}
}

void i8256_device::write(offs_t offset, u8 data)
{
	u8 reg = offset & 0x0f;

	// In the 8-bit mode, AD0-AD3 are used to select the proper register, while AD1-AD4 are used in the 16-bit mode.
	// AD4 in the 8-bit mote is ignored as an address.

	if (BIT(m_command1,I8256_CMD1_8086))
	{
		if (!BIT(offset,0)) // AD0 in the 16-bit mode is used as a second chip select, active low.
			reg = (offset >> 1) & 0x0f;
		else
			return;
	}

	switch (reg)
	{
		case I8256_REG_CMD1:
			if (m_command1 != data)
			{
				m_command1 = data;

				reset_timer();

				if (BIT(m_command1,I8256_CMD1_8086))
					LOG("I8256 Enabled 8086 mode\n");

				m_data_bits = CHAR_LEN[(m_command1 >> 6) & 0x03];
				m_stop_bits_mode = (m_command1 >> 4) & 0x03;
				LOG("I8256 CMD1=0x%02x: data_bits=%d, stop_bits_mode=%d, 8086=%d, BITI=%d\n", data, m_data_bits, m_stop_bits_mode, BIT(m_command1,I8256_CMD1_8086), BIT(m_command1,I8256_CMD1_BITI));
			}
			break;
		case I8256_REG_CMD2:
			if (m_command2 != data)
			{
				m_command2 = data;

				m_baud_sel = m_command2 & 0x0f;

			// Calculate br_factor: timer is 16 kHz for serial operations
		// br_factor = number of 16 kHz timer ticks per baud rate bit
		if (m_baud_sel >= 3 && BAUD_RATES[m_baud_sel] > 0)
		{
			// Internal clock: br_factor = 16000 / baud_rate (rounded)
			m_br_factor = (16000 + BAUD_RATES[m_baud_sel] / 2) / BAUD_RATES[m_baud_sel];
			LOG("I8256: Internal baud mode, br_factor=%d for %d bps\n", m_br_factor, BAUD_RATES[m_baud_sel]);
		}
		else if (m_baud_sel == 1)
			m_br_factor = 32;
		else if (m_baud_sel == 2)
			m_br_factor = 64;
		else
			m_br_factor = 1;
				if (BIT(m_command2,I8256_CMD2_PARITY_ENABLE))
					m_parity = BIT(m_command2,I8256_CMD2_EVEN_PARITY) ? PARITY_EVEN : PARITY_ODD;
				else
					m_parity = PARITY_NONE;

LOG("I8256 CMD2=0x%02x: Baud sel=%d, br_factor=%d, parity=%d, baud_rate=%d\n", data, m_baud_sel, m_br_factor, m_parity, BAUD_RATES[m_baud_sel]);
			LOG("I8256 Clock Scale: %u\n", SYS_CLOCK_DIVIDER[(m_command2 & 0x30) >> 4]);
			if ((clock() / SYS_CLOCK_DIVIDER[(m_command2 & 0x30) >> 4]) != 1024000)
				logerror("I8256 Internal Clock should be 1024000, calculated: %u\n", (clock() / SYS_CLOCK_DIVIDER[(m_command2 & 0x30) >> 4]));

			if (m_baud_sel >= 3)
			{
				LOG("I8256: Using internal baud rate generator\n");
				m_rx_state = STATE_IDLE;
				m_rx_counter = 0;
				m_tx_state = STATE_IDLE;
				m_tx_counter = 0;
				m_internal_txc = 0;
			}
			else
			{
				LOG("I8256: Expecting external clock on TxC/RxC\n");
				}

				reset_timer();
			}
			break;
		case I8256_REG_CMD3:
			LOG("I8256 CMD3=0x%02x: Set=%d (cmd3 will be 0x%02x)\n", data, BIT(data, I8256_CMD3_SET), BIT(data, I8256_CMD3_SET) ? (m_command3 | data) : (m_command3 & ~data));
			if (BIT(data, I8256_CMD3_SET))
				m_command3 |= (data & 0x7f);
			else
				m_command3 &= ~(data & 0x7f);

			LOG("I8256 CMD3 updated: RxE=%d, TBRK=%d, SBRK=%d, IAE=%d\n", 
				BIT(m_command3,I8256_CMD3_RxE), BIT(m_command3,I8256_CMD3_TBRK), 
				BIT(m_command3,I8256_CMD3_SBRK), BIT(m_command3,I8256_CMD3_IAE));

			if (BIT(m_command3,I8256_CMD3_RST))
			{
				LOG("I8256 Software Reset\n");
				m_interrupts = 0;
				m_status = 0x30;
				m_current_interrupt_level = 0;
				m_out_int_cb(CLEAR_LINE);
				m_rx_state = STATE_IDLE;
				m_rx_counter = 0;
				m_rx_accumulator = 0;
				m_tx_state = STATE_IDLE;
				m_tx_counter = 0;
				m_bit_accumulator = 0;
				output_txd(1);
				m_command3 &= ~(1 << I8256_CMD3_RST);
			}
			break;
		case I8256_REG_MODE:
			m_mode = data;
			break;
		case I8256_REG_PORT1C:
			m_port1_control = data;
			break;
		case I8256_REG_INTEN:
			LOG("I8256 Interrupt enable: 0x%02x (was 0x%02x)\n", data, m_interrupts);
			m_interrupts = m_interrupts | data;
			break;
		case I8256_REG_INTAD: // reset interrupt
			LOG("I8256 Interrupt acknowledge: 0x%02x\n", data);
			m_interrupts = m_interrupts & ~data;
			break;
		case I8256_REG_BUFFER:
			LOG("I8256 TX BUFFER write: 0x%02x (will transmit when ready, current CTS=%d)\n", data, m_cts);
			m_tx_buffer = data;
			m_tx_buffer_full = true;
			m_status &= ~ (1 << I8256_STATUS_TB_EMPTY);
			break;
		case I8256_REG_PORT1:
			m_port1_int = data;
			break;
		case I8256_REG_PORT2:
			m_port2_int = data;
			break;
		case I8256_REG_TIMER1:
		case I8256_REG_TIMER2:
		case I8256_REG_TIMER3:
			m_timers[reg-10] = data;
			break;
		case I8256_REG_TIMER4:
			if (BIT(m_mode, I8256_MODE_T24)) m_timers[1] = 255;
			m_timers[reg-10] = data;
			break;
		case I8256_REG_TIMER5:
			if (BIT(m_mode, I8256_MODE_T35)) m_timers[2] = 255;
			m_timers[reg-10] = data;
			break;
		case I8256_REG_STATUS:
			m_modification = data;
			break;
		default:
			LOG("I8256 Unmapped write %02x to %02x\n", data, reg);
			break;
	}
}

uint8_t i8256_device::p1_r()
{
	uint8_t input = m_in_p1_cb(0);
	// For output bits: use latched value, for input bits: use current input
	return (m_port1_int & m_port1_control) | (input & ~m_port1_control);
}

void i8256_device::p1_w(uint8_t data)
{
	// Check for P17 interrupt if BITI=1
	if (BIT(m_command1, I8256_CMD1_BITI) && !BIT(m_port1_int, 7) && BIT(data, 7))
	{
		gen_interrupt(I8256_INT_TIMER2);
	}

	m_port1_int = (m_port1_int & ~m_port1_control) | (data & m_port1_control);
	m_out_p1_cb(0, m_port1_int & m_port1_control);
}

uint8_t i8256_device::p2_r()
{
	uint8_t p2c = m_mode & 0x07;
	uint8_t result = 0;
	
	switch (p2c)
	{
		case I8256_PORT2C_IO:
			result = m_in_p2_cb(0) & 0x0f;
			result |= m_port2_int & 0xf0;
			break;
		case I8256_PORT2C_OI:
			result = m_port2_int & 0x0f;
			result |= m_in_p2_cb(0) & 0xf0;
			break;
		case I8256_PORT2C_OO:
			result = m_port2_int;
			break;
		case I8256_PORT2C_HI:
			// TODO Handshake
			result = m_in_p2_cb(0);
			break;
		case I8256_PORT2C_HO:
			// TODO Handshake
			result = m_port2_int;
			break;
		case I8256_PORT2C_DNU:
			// Do not use
			result = 0;
			break;
		case I8256_PORT2C_TEST:
			// TODO Test mode
			result = 0;
			break;
		case I8256_PORT2C_II:
		default:
			result = m_in_p2_cb(0);
			break;
	}
	return result;
}

void i8256_device::p2_w(uint8_t data)
{
	uint8_t p2c = m_mode & 0x07;
	m_port2_int = data;
	uint8_t port2_data = 0;
	switch (p2c)
	{
		case I8256_PORT2C_IO: port2_data = m_port2_int & 0x0f; break;
		case I8256_PORT2C_OI: port2_data = m_port2_int & 0xf0; break;
		case I8256_PORT2C_OO: port2_data = m_port2_int; break;
		case I8256_PORT2C_HI: port2_data = 0; break;
		case I8256_PORT2C_HO: port2_data = m_port2_int; break;
		case I8256_PORT2C_DNU: port2_data = 0; break;
		case I8256_PORT2C_TEST: port2_data = 0; break;
		default: port2_data = 0; break;
	}
	if (p2c == I8256_PORT2C_IO || p2c == I8256_PORT2C_OI || p2c == I8256_PORT2C_OO || p2c == I8256_PORT2C_HO)
		m_out_p2_cb(0, port2_data);
}

/*-------------------------------------------------
    serial interface
-------------------------------------------------*/

void i8256_device::write_rxd(int state)
{
	LOG("i8256_device::write_rxd %d\n", state);
	m_rxd = state; // was: device_serial_interface::rx_w(state)
}

void i8256_device::write_txc(int state)
{
	if (m_txc == state)
		return;
	m_txc = state;
	LOG("i8256_device::write_txc %d\n", state);
	if (m_baud_sel <= 2 && !state)
		transmit_clock();
}

void i8256_device::write_rxc(int state)
{
	if (m_rxc == state)
		return;
	m_rxc = state;
	LOG("i8256_device::write_rxc %d\n", state);
	if (m_baud_sel == 0 && state)
		receive_clock();
}

void i8256_device::write_cts(int state)
{
	LOG("i8256_device::write_cts %d (was %d)\n", state, m_cts);
	m_cts = state;
}
