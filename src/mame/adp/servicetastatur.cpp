// license:BSD-3-Clause
// copyright-holders:stonedDiscord
// ADP Profitech 3000 service keyboard standalone test configuration.

#include "emu.h"
#include "bus/adp/bus.h"

namespace {

class servicet_state : public driver_device
{
public:
	servicet_state(machine_config const &mconfig, device_type type, char const *tag) :
		driver_device(mconfig, type, tag)
	{ }

	void servicet(machine_config &config) ATTR_COLD;
};

void servicet_state::servicet(machine_config &config)
{
	ADP_BUS(config, "service", adp_bus_devices, "keyboard");
}

static INPUT_PORTS_START(servicet)
INPUT_PORTS_END

ROM_START(servicet)
ROM_END

} // anonymous namespace

GAME( 1992, servicet, 0, servicet, servicet, servicet_state, empty_init, ROT0, "ADP", u8"Merkur Service Testgerät", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
