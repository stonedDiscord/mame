// license:BSD-3-Clause
// copyright-holders:stonedDiscord
#ifndef MAME_BUS_ADP_BUS_H
#define MAME_BUS_ADP_BUS_H

#pragma once

class device_adp_bus_interface;

// ADP serial peripheral connector: data out, clock, enable and data in.
// Each connector carries one channel; framing belongs to its peripheral.
class adp_bus_device : public device_t, public device_single_card_slot_interface<device_adp_bus_interface>
{
public:
	template <typename T>
	adp_bus_device(machine_config const &mconfig, char const *tag, device_t *owner, T &&options, char const *default_option)
		: adp_bus_device(mconfig, tag, owner, 0)
	{
		set_options(std::forward<T>(options), default_option, false);
	}

	adp_bus_device(machine_config const &mconfig, char const *tag, device_t *owner, uint32_t clock = 0);

	void data_w(int state);
	void clock_w(int state);
	void enable_w(int state);
	int data_r() const { return m_data; }
	void return_data_w(int state) { m_data = state; }

protected:
	virtual void device_start() override ATTR_COLD;

private:
	device_adp_bus_interface *m_card;
	int m_data;
};

class device_adp_bus_interface : public device_interface
{
public:
	virtual void data_w(int state) = 0;
	virtual void clock_w(int state) = 0;
	virtual void enable_w(int state) = 0;

protected:
	device_adp_bus_interface(machine_config const &mconfig, device_t &device);
	void output_data(int state);

private:
	adp_bus_device *const m_port;
};

DECLARE_DEVICE_TYPE(ADP_BUS, adp_bus_device)

void adp_bus_devices(device_slot_interface &device);

#endif // MAME_BUS_ADP_BUS_H
