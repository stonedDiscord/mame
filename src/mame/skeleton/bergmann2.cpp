// license:BSD-3-Clause
// copyright-holders:stonedDiscord

/*
CPU Z0840004PSC
RAM MB8416-20L
PIO 2x Z0842004PSC
CTC 2x Z0843004PSC

*/


#include "emu.h"

#include "cpu/z80/z80.h"
#include "machine/watchdog.h"
#include "machine/z80daisy.h"
#include "machine/z80pio.h"
#include "machine/z80ctc.h"

#include "crown.lh"

#define VERBOSE 1
#include "logmacro.h"

namespace {

class bergmann2_state : public driver_device
{
public:
    bergmann2_state(const machine_config &mconfig, device_type type, const char *tag)
        : driver_device(mconfig, type, tag)
        , m_maincpu(*this, "maincpu")
        , m_pio1(*this, "pio1")
        , m_pio2(*this, "pio2")
        , m_ctc1(*this, "ctc1")
        , m_ctc2(*this, "ctc2")
        , m_watchdog(*this, "watchdog")
        , m_led(*this, "led_error")
    {
    }

    void bergmann2(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;

private:
    required_device<z80_device> m_maincpu;

    required_device<z80pio_device> m_pio1;
    required_device<z80pio_device> m_pio2;

    required_device<z80ctc_device> m_ctc1;
    required_device<z80ctc_device> m_ctc2;

    required_device<watchdog_timer_device> m_watchdog;

    output_finder<> m_led;

    bool m_battery = false;

    void mem_map(address_map &map);
    void io_map(address_map &map);

    uint8_t pio1_pa_r();
    uint8_t pio1_pb_r();
    void pio1_pb_w(uint8_t data);
    uint8_t pio2_pb_r();
    void pio2_pb_w(uint8_t data);
    uint8_t pio2_pa_r();
    void pio2_pa_w(uint8_t data);
    void ctc1_zc0_w(int state);
    void ctc1_zc1_w(int state);
    void ctc1_zc2_w(int state);
    void ctc2_zc0_w(int state);
};

void bergmann2_state::machine_start()
{
	m_led.resolve();
}

void bergmann2_state::mem_map(address_map &map)
{
    map.global_mask(0x7fff);
    map(0x0000, 0x3fff).rom().region("maincpu", 0);
    map(0x4000, 0x47ff).ram();
    map(0x6000, 0x6000).w(m_watchdog, FUNC(watchdog_timer_device::reset_w));
}

void bergmann2_state::io_map(address_map &map)
{
    map.global_mask(0x1f);
    map(0x00, 0x03).rw(m_ctc2, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
    map(0x04, 0x07).rw(m_ctc1, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
    map(0x08, 0x0b).rw(m_pio2, FUNC(z80pio_device::read), FUNC(z80pio_device::write));
    map(0x0c, 0x0f).rw(m_pio1, FUNC(z80pio_device::read_alt), FUNC(z80pio_device::write_alt));
    map(0x10, 0x13).noprw(); //74C373/3
    map(0x14, 0x17).noprw(); //74C373/2
    map(0x18, 0x1b).noprw(); //74C373/1

}

//PIO1
uint8_t bergmann2_state::pio1_pa_r()
{
    // Steckerleiste 15
    return ioport("COIN")->read();
}

uint8_t bergmann2_state::pio1_pb_r()
{
    // Steckerleiste 15
    uint8_t data = 0xbf;

    data |= ioport("RETURN")->read();
    return data;
}

void bergmann2_state::pio1_pb_w(uint8_t data)
{
    // Steckerleiste 15
    //coins out
	machine().bookkeeping().coin_counter_w(3,BIT(data,0)); // 0.10DM
	machine().bookkeeping().coin_counter_w(2,BIT(data,1)); // 1DM
	machine().bookkeeping().coin_counter_w(1,BIT(data,2)); // 2DM
	machine().bookkeeping().coin_counter_w(0,BIT(data,3)); // 5DM

    machine().bookkeeping().coin_lockout_global_w(BIT(data,4)); // coin magnet
}


//PIO2
uint8_t bergmann2_state::pio2_pb_r()
{
    // Steckerleiste 16
    return 0xff;
}

void bergmann2_state::pio2_pb_w(uint8_t data)
{
    // Steckerleiste 16
}

uint8_t bergmann2_state::pio2_pa_r()
{
    // Steckerleiste 17
    uint8_t data = 0xbf;
    data |= m_battery << 7;
    //Bit 6+7 battery
    return data;
}

void bergmann2_state::pio2_pa_w(uint8_t data)
{
    // Steckerleiste 17
    LOG("PIO2 PA: %02x\n", data);
    m_led = BIT(data, 0);
    // 1 NC
    // alarm_l = BIT(data, 2);
    // alarm_r = BIT(data, 3);
    // sound_r = BIT(data, 4);
    // sound_l = BIT(data, 5);
    m_battery = BIT(data, 6);
    
}

void bergmann2_state::ctc1_zc0_w(int state)
{
    LOG("CTC1 ZC0: %d\n", state);
}

void bergmann2_state::ctc1_zc1_w(int state)
{
    LOG("CTC1 ZC1: %d\n", state);
}

void bergmann2_state::ctc1_zc2_w(int state)
{
    LOG("CTC1 ZC2: %d\n", state);
}

void bergmann2_state::ctc2_zc0_w(int state)
{
    m_ctc2->trg1(state);
    m_ctc2->trg2(state);
    m_ctc2->trg3(state);
}

static INPUT_PORTS_START( bergmann2 )
    PORT_START("RETURN")
    PORT_BIT( 0x4f, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_GAMBLE_PAYOUT ) PORT_NAME("Return")

    PORT_START("COIN")
    PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 ) // 5DM
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 ) // 2DM
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN3 ) // 1DM
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN4 ) // 0.10 DM
INPUT_PORTS_END

static const z80_daisy_config daisy_chain[] =
{
	{ "ctc1" },
	{ "pio1" },
	{ "pio2" },
    { "ctc2" },
	{ nullptr }
};

void bergmann2_state::bergmann2(machine_config &config)
{
    Z80(config, m_maincpu, 4_MHz_XTAL/2);
    m_maincpu->set_addrmap(AS_PROGRAM, &bergmann2_state::mem_map);
    m_maincpu->set_addrmap(AS_IO, &bergmann2_state::io_map);
    m_maincpu->set_daisy_config(daisy_chain);

    Z80PIO(config, m_pio1, 4_MHz_XTAL/2);
    m_pio1->in_pa_callback().set( FUNC(bergmann2_state::pio1_pa_r));
    m_pio1->out_pb_callback().set(FUNC(bergmann2_state::pio1_pb_w));
    m_pio1->in_pb_callback().set( FUNC(bergmann2_state::pio1_pb_r));
    m_pio1->out_int_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
    Z80PIO(config, m_pio2, 4_MHz_XTAL/2);
    m_pio2->out_pa_callback().set(FUNC(bergmann2_state::pio2_pa_w));
    m_pio2->in_pa_callback().set( FUNC(bergmann2_state::pio2_pa_r));
    m_pio2->out_pb_callback().set(FUNC(bergmann2_state::pio2_pb_w));
    m_pio2->in_pb_callback().set( FUNC(bergmann2_state::pio2_pb_r));
    m_pio2->out_int_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
    Z80CTC(config, m_ctc1, 4_MHz_XTAL/2);
    m_ctc1->intr_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
    m_ctc1->zc_callback<0>().set(FUNC(bergmann2_state::ctc1_zc0_w));
    m_ctc1->zc_callback<1>().set(FUNC(bergmann2_state::ctc1_zc1_w));
    m_ctc1->zc_callback<2>().set(FUNC(bergmann2_state::ctc1_zc2_w));
    Z80CTC(config, m_ctc2, 4_MHz_XTAL/2);
    m_ctc2->zc_callback<0>().set(FUNC(bergmann2_state::ctc2_zc0_w));
    m_ctc2->intr_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);

    WATCHDOG_TIMER(config, m_watchdog).set_time(attotime::from_usec(341)); // 47uF x 22k x 0,33

    config.set_default_layout(layout_crown);

}

ROM_START( corsar )
    ROM_REGION(0x4000, "maincpu", ROMREGION_ERASEFF)
    ROM_LOAD( "crown_corsar_a.bin", 0x0000, 0x2000, CRC(cf907cab) SHA1(66e22b8e1f8e3645dd4b5f1504517b7e734b20f2) )
    ROM_LOAD( "crown_corsar_b.bin", 0x2000, 0x2000, CRC(c1a69ccd) SHA1(97561620110ed991dcd2b17fbd0de5d0bbc282fe) )
ROM_END

ROM_START( jubilees )
    ROM_REGION(0x4000, "maincpu", ROMREGION_ERASEFF)
    ROM_LOAD( "jubilee_super_a.bin", 0x0000, 0x2000, CRC(32b8ad57) SHA1(8b95f23b5cb22261f7db62e3879b475722cfe0f2) )
    ROM_LOAD( "jubilee_super_b.bin", 0x2000, 0x1000, CRC(60b7a6d3) SHA1(a98375c970176927ad4f0ca6b62b1a22cc07df69) )
ROM_END

} // anonymous namespace

GAMEL( 1984, corsar,   0, bergmann2, bergmann2, bergmann2_state, empty_init, ROT0, "Crown", "Corsar",        MACHINE_NOT_WORKING | MACHINE_NO_SOUND, layout_crown )
GAMEL( 1984, jubilees, 0, bergmann2, bergmann2, bergmann2_state, empty_init, ROT0, "Crown", "Jubilee Super", MACHINE_NOT_WORKING | MACHINE_NO_SOUND, layout_crown )
