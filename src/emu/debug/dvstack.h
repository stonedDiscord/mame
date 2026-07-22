// license:BSD-3-Clause
// copyright-holders:MAMEdev Team
#ifndef MAME_EMU_DEBUG_DVSTACK_H
#define MAME_EMU_DEBUG_DVSTACK_H

#pragma once

#include "debugvw.h"

class debug_disasm_buffer;

class debug_view_stack_source : public debug_view_source
{
	friend class debug_view_stack;

public:
	debug_view_stack_source(std::string &&name, device_t &device, device_state_entry const &stack);

private:
	device_state_entry const &m_stack;
	device_memory_interface &m_memory;
	int const m_stack_space;
};

class debug_view_stack : public debug_view
{
	friend class debug_view_manager;

	debug_view_stack(running_machine &machine, debug_view_osd_update_func osdupdate, void *osdprivate);

protected:
	virtual void view_update() override;
	virtual void view_notify(debug_view_notification type) override;

private:
	void enumerate_sources();
	std::string describe_value(debug_view_stack_source const &source, debug_disasm_buffer &buffer, u64 value);
};

#endif // MAME_EMU_DEBUG_DVSTACK_H
