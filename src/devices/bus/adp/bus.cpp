// license:BSD-3-Clause
// copyright-holders:stonedDiscord
#include "emu.h"
#include "bus.h"
#include "servicet.h"

DEFINE_DEVICE_TYPE(ADP_BUS, adp_bus_device, "adp_bus", "ADP Serial Peripheral Bus")

adp_bus_device::adp_bus_device(machine_config const &mconfig, char const *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, ADP_BUS, tag, owner, clock)
	, device_single_card_slot_interface<device_adp_bus_interface>(mconfig, *this)
	, m_card(nullptr)
	, m_data(0)
{
}

void adp_bus_device::device_start()
{
	m_card = get_card_device();
	save_item(NAME(m_data));
}

void adp_bus_device::data_w(int state)
{
	if (m_card)
		m_card->data_w(state);
}

void adp_bus_device::clock_w(int state)
{
	if (m_card)
		m_card->clock_w(state);
}

void adp_bus_device::enable_w(int state)
{
	if (m_card)
		m_card->enable_w(state);
}

device_adp_bus_interface::device_adp_bus_interface(machine_config const &mconfig, device_t &device)
	: device_interface(device, "adpbus")
	, m_port(dynamic_cast<adp_bus_device *>(device.owner()))
{
}

void device_adp_bus_interface::output_data(int state)
{
	if (m_port)
		m_port->return_data_w(state);
}

void adp_bus_devices(device_slot_interface &device)
{
	device.option_add("keyboard", ADP_SERVICE_KEYBOARD);
}
