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
#include "machine/z80daisy.h"
#include "machine/z80pio.h"
#include "machine/z80ctc.h"

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
        , m_led(*this, "led")
    {
    }

    void bergmann2(machine_config &config);

private:
    required_device<z80_device> m_maincpu;

    required_device<z80pio_device> m_pio1;
    required_device<z80pio_device> m_pio2;

    required_device<z80ctc_device> m_ctc1;
    required_device<z80ctc_device> m_ctc2;

    output_finder<> m_led;

    void mem_map(address_map &map);
    void io_map(address_map &map);

    uint8_t pio1_pa_r();
    uint8_t pio1_pb_r();
    void pio1_pb_w(uint8_t data);
    uint8_t pio2_pb_r();
    void pio2_pb_w(uint8_t data);
    uint8_t pio2_pa_r();
    void pio2_pa_w(uint8_t data);

};

void bergmann2_state::mem_map(address_map &map)
{
    map.global_mask(0x7fff);
    map(0x0000, 0x3fff).rom().region("maincpu", 0);
    map(0x4000, 0x47ff).ram();
}

void bergmann2_state::io_map(address_map &map)
{
    map.global_mask(0x1f);
    map(0x00, 0x03).rw(m_ctc2, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
    map(0x04, 0x07).rw(m_ctc1, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
    map(0x08, 0x0b).rw(m_pio2, FUNC(z80pio_device::read_alt), FUNC(z80pio_device::write_alt));
    map(0x0c, 0x0f).rw(m_pio1, FUNC(z80pio_device::read_alt), FUNC(z80pio_device::write_alt));

    // Steckerleiste 13
    //map(0x10, 0x13).noprw(); //74C373/3
    //map(0x14, 0x17).noprw(); //74C373/3
    //map(0x18, 0x1b).noprw(); //74C373/3

}

//PIO1
uint8_t bergmann2_state::pio1_pa_r()
{
    // Steckerleiste 15
    return 0xff;
}

uint8_t bergmann2_state::pio1_pb_r()
{
    // Steckerleiste 15
    return 0xff;
}

void bergmann2_state::pio1_pb_w(uint8_t data)
{
    // Steckerleiste 15
    //coins out
	machine().bookkeeping().coin_counter_w(0,BIT(data,0)); // coin eject
	machine().bookkeeping().coin_counter_w(1,BIT(data,1));
	machine().bookkeeping().coin_counter_w(2,BIT(data,2));
	machine().bookkeeping().coin_counter_w(3,BIT(data,3));

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
    return 0xff;
}

void bergmann2_state::pio2_pa_w(uint8_t data)
{
    // Steckerleiste 17
    m_led = BIT(data, 0);
}


static INPUT_PORTS_START( bergmann2 )
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
    Z80(config, m_maincpu, 4_MHz_XTAL); // Z0840004PSC, divider not verified
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
    Z80CTC(config, m_ctc2, 4_MHz_XTAL/2);
    m_ctc2->intr_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);

}

ROM_START( corsar )
    ROM_REGION(0x10000, "maincpu", 0)
    ROM_LOAD( "crown_corsar_a.bin", 0x0000, 0x2000, CRC(cf907cab) SHA1(66e22b8e1f8e3645dd4b5f1504517b7e734b20f2) )
    ROM_LOAD( "crown_corsar_b.bin", 0x2000, 0x2000, CRC(c1a69ccd) SHA1(97561620110ed991dcd2b17fbd0de5d0bbc282fe) )
ROM_END


} // anonymous namespace

GAME( 198?, corsar, 0, bergmann2, bergmann2, bergmann2_state, empty_init, ROT0, "Crown", "Corsar", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
