// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/**********************************************************************

    Intel 8256(AH) Multifunction microprocessor support controller emulation

**********************************************************************
                            _____   _____
                   AD0   1 |*    \_/     | 40  Vcc
                   AD1   2 |             | 39  P10
                   AD2   3 |             | 38  P11
                   AD3   4 |             | 37  P12
                   AD4   5 |             | 36  P13
                   DB5   6 |             | 35  P14
                   DB6   7 |             | 34  P15
                   DB7   8 |             | 33  P16
                   ALE   9 |             | 32  P17
                    RD  10 |    8256     | 31  P20
                    WR  11 |    8256AH   | 30  P21
                 RESET  12 |             | 29  P22
                    CS  13 |             | 28  P23
                  INTA  14 |             | 27  P24
                   INT  15 |             | 26  P25
                EXTINT  16 |             | 25  P26
                   CLK  17 |             | 24  P27
                   RxC  18 |             | 23  TxD
                   RxD  19 |             | 22  TxC
                   GND  20 |_____________| 21  CTS

**********************************************************************/

#ifndef MAME_MACHINE_I8256_H
#define MAME_MACHINE_I8256_H

#pragma once


class i8256_device : public device_t // removed device_serial_interface
{
public:
	i8256_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	auto inta_callback()    { return m_in_inta_cb.bind(); }
	auto int_callback()     { return m_out_int_cb.bind(); }
	auto extint_callback()  { return m_in_extint_cb.bind(); }

	auto txd_handler() { return m_txd_handler.bind(); }
	auto rts_handler() { return m_rts_handler.bind(); } // new: RTS output
	auto dtr_handler() { return m_dtr_handler.bind(); } // new: DTR output
	auto rxc_handler() { return m_rxc_handler.bind(); } // new: RxC output pin
	auto txc_handler() { return m_txc_handler.bind(); } // new: TxC output pin

	auto in_p2_callback()   { return m_in_p2_cb.bind(); }
	auto out_p2_callback()  { return m_out_p2_cb.bind(); }
	auto in_p1_callback()   { return m_in_p1_cb.bind(); }
	auto out_p1_callback()  { return m_out_p1_cb.bind(); }

	void write_rxc(int state);
	void write_rxd(int state);
	void write_cts(int state);
	void write_txc(int state);

	void write(offs_t offset, u8 data);
	uint8_t read(offs_t offset);
	uint8_t acknowledge();
	void gen_interrupt(uint8_t level);

	uint8_t p1_r();
	void    p1_w(uint8_t data);
	uint8_t p2_r();
	void    p2_w(uint8_t data);

protected:
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

private:
	// serial state machine states // new
	enum serial_state // new
	{
		STATE_IDLE, // new
		STATE_START, // new
		STATE_DATA, // new
		STATE_PARITY, // new
		STATE_STOP // new
	};

	// parity types // new
	enum parity_type // new
	{
		PARITY_NONE, // new
		PARITY_ODD, // new
		PARITY_EVEN // new
	};

	devcb_read_line m_in_inta_cb;
	devcb_write_line m_out_int_cb;
	devcb_read_line m_in_extint_cb;

	devcb_write_line m_txd_handler;
	devcb_write_line m_rts_handler; // new
	devcb_write_line m_dtr_handler; // new
	devcb_write_line m_rxc_handler; // new
	devcb_write_line m_txc_handler; // new

	devcb_read8 m_in_p2_cb;
	devcb_write8 m_out_p2_cb;
	devcb_read8 m_in_p1_cb;
	devcb_write8 m_out_p1_cb;

	bool m_rxc;
	bool m_rxd;
	bool m_cts;
	bool m_txc;

	uint8_t m_command1, m_command2, m_command3;
	uint8_t m_data_bits; // new: decoded character length (5-8)
	uint8_t m_parity; // was int, now uses parity_type enum
	uint8_t m_stop_bits_mode; // new: replaces m_stop_bits, holds I8256_STOP_* value
	uint8_t m_baud_sel; // new: lower 4 bits of command2

	uint8_t m_mode;
	uint8_t m_port1_control;
	uint8_t m_interrupts, m_current_interrupt_level;
	uint8_t m_tx_buffer, m_rx_buffer;
	bool m_tx_buffer_full; // new: explicit buffer-full flag
	uint8_t m_port1_int, m_port2_int;
	uint8_t m_timers[5];
	emu_timer *m_timer;

	uint8_t m_status, m_modification;

	uint8_t m_br_factor;
	uint32_t m_bit_accumulator;  // Fractional bit timing accumulator for TX
	uint32_t m_rx_accumulator;   // Fractional bit timing accumulator for RX

	// transmitter shift register state // new
	uint8_t m_tx_shift; // new
	int m_tx_state; // new
	int m_tx_bits; // new
	int m_tx_parity; // new
	int m_tx_counter; // new
	int m_txd; // new: current TxD output value

	// receiver shift register state // new
	uint8_t m_rx_shift; // new
	int m_rx_state; // new
	int m_rx_bits; // new
	int m_rx_parity; // new
	int m_rx_counter; // new

	int m_internal_txc; // new: subdivider counter for internal baud generator

	TIMER_CALLBACK_MEMBER(timer_check);

	void reset_timer();
	void output_txd(int state); // new
	void receive_clock(); // new: replaces rcv_complete
	void transmit_clock(); // new: replaces tra_callback/tra_complete
};

DECLARE_DEVICE_TYPE(I8256, i8256_device)

#endif // MAME_MACHINE_I8256_H
