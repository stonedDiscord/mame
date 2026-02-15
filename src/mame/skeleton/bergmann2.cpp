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
    {
    }

    void bergmann2(machine_config &config);

private:
    required_device<z80_device> m_maincpu;

    required_device<z80pio_device> m_pio1;
    required_device<z80pio_device> m_pio2;

    required_device<z80ctc_device> m_ctc1;
    required_device<z80ctc_device> m_ctc2;

    void mem_map(address_map &map);
    void io_map(address_map &map);
};

void bergmann2_state::mem_map(address_map &map)
{
    map.global_mask(0x7fff);
    map(0x0000, 0x3fff).rom().region("maincpu", 0);
    map(0x4000, 0x47ff).ram();
}

void bergmann2_state::io_map(address_map &map)
{
    map.global_mask(0xff);
    map(0x40, 0x43).rw(m_ctc1, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
    map(0x44, 0x47).rw(m_ctc2, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
    map(0x48, 0x4B).rw(m_pio1, FUNC(z80pio_device::read_alt), FUNC(z80pio_device::write_alt));
    map(0x4C, 0x4F).rw(m_pio2, FUNC(z80pio_device::read_alt), FUNC(z80pio_device::write_alt));
}

static INPUT_PORTS_START( bergmann2 )
INPUT_PORTS_END

void bergmann2_state::bergmann2(machine_config &config)
{
    Z80(config, m_maincpu, 4_MHz_XTAL); // Z0840004PSC, divider not verified
    m_maincpu->set_addrmap(AS_PROGRAM, &bergmann2_state::mem_map);
    m_maincpu->set_addrmap(AS_IO, &bergmann2_state::io_map);

    Z80PIO(config, m_pio1, 4_MHz_XTAL/2);
    Z80PIO(config, m_pio2, 4_MHz_XTAL/2);
    Z80CTC(config, m_ctc1, 4_MHz_XTAL/2);
    Z80CTC(config, m_ctc2, 4_MHz_XTAL/2);

}

ROM_START( corsar )
    ROM_REGION(0x10000, "maincpu", 0)
    ROM_LOAD( "crown_corsar_a.bin", 0x0000, 0x2000, CRC(cf907cab) SHA1(66e22b8e1f8e3645dd4b5f1504517b7e734b20f2) )
    ROM_LOAD( "crown_corsar_b.bin", 0x2000, 0x2000, CRC(c1a69ccd) SHA1(97561620110ed991dcd2b17fbd0de5d0bbc282fe) )
ROM_END


} // anonymous namespace

GAME( 198?, corsar, 0, bergmann2, bergmann2, bergmann2_state, empty_init, ROT0, "Crown", "Corsar", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
