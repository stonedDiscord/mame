// license:BSD-3-Clause
// copyright-holders:MAMEdev Team

#include "emu.h"
#include "dvstack.h"

#include "debugbuf.h"
#include "debugcpu.h"

#include <algorithm>
#include <cctype>
#include <locale>

namespace {

bool stack_symbol(std::string symbol)
{
	std::transform(symbol.begin(), symbol.end(), symbol.begin(), [] (unsigned char ch) { return std::tolower(ch); });
	return (symbol == "sp") || (symbol == "rsp") || (symbol == "esp");
}

u8 pointer_bytes(address_space_config const &config)
{
	int const bits = config.logaddr_width();
	return bits <= 8 ? 1 : bits <= 16 ? 2 : bits <= 32 ? 4 : 8;
}

} // anonymous namespace

debug_view_stack_source::debug_view_stack_source(std::string &&name, device_t &device, device_state_entry const &stack)
	: debug_view_source(std::move(name), &device)
	, m_stack(stack)
	, m_memory(device.memory())
	, m_stack_space(m_memory.has_space(AS_DATA) ? AS_DATA : AS_PROGRAM)
{
}

debug_view_stack::debug_view_stack(running_machine &machine, debug_view_osd_update_func osdupdate, void *osdprivate)
	: debug_view(machine, DVT_STACK, osdupdate, osdprivate)
{
	enumerate_sources();
	if (m_source_list.empty())
		throw std::bad_alloc();
}

void debug_view_stack::enumerate_sources()
{
	for (device_state_interface &state : state_interface_enumerator(machine().root_device()))
	{
		device_t &device = state.device();
		device_memory_interface *memory;
		device_disasm_interface *disasm;
		if (!device.debug() || !device.interface(memory) || !device.interface(disasm) || !memory->has_space(AS_PROGRAM))
			continue;

		for (auto const &entry : state.state_entries())
		{
			if (!entry->divider() && !entry->is_float() && stack_symbol(entry->symbol()))
			{
				m_source_list.emplace_back(std::make_unique<debug_view_stack_source>(
						util::string_format(std::locale::classic(), "%s '%s' (%s)", device.name(), device.tag(), entry->symbol()), device, *entry));
				break;
			}
		}
	}

	if (!m_source_list.empty())
		set_source(*m_source_list[0]);
}

void debug_view_stack::view_notify(debug_view_notification type)
{
	if (type == VIEW_NOTIFY_SOURCE_CHANGED)
	{
		m_topleft = debug_view_xy(0, 0);
		m_recompute = true;
	}
}

std::string debug_view_stack::describe_value(debug_view_stack_source const &source, debug_disasm_buffer &buffer, u64 value)
{
	device_memory_interface &memory = source.m_memory;
	address_space_config const *const config = memory.logical_space_config(AS_PROGRAM);
	offs_t const logical = value & config->logaddrmask();
	offs_t translated = logical;
	address_space *space;
	if (!memory.translate(AS_PROGRAM, device_memory_interface::TR_READ, translated, space))
		return std::string();
	std::string const handler = space->get_handler_string(read_or_write::READ, translated);
	if ((handler.find("unmapped") != std::string::npos) || (handler.find("nop") != std::string::npos))
		return std::string();

	std::string text;
	{
		offs_t const string_address = translated;
		auto disable = machine().disable_side_effects();
		for (unsigned i = 0; i < 32; ++i)
		{
			u8 const ch = space->read_byte(string_address + i);
			if (!ch)
				break;
			if ((ch < 0x20) || (ch > 0x7e))
			{
				text.clear();
				break;
			}
			text.push_back(char(ch));
		}
	}
	if (text.size() >= 4)
		return util::string_format("%s: \"%s\"", handler, text);

	translated = logical;
	if (!memory.translate(AS_PROGRAM, device_memory_interface::TR_FETCH, translated, space))
		return handler;
	std::string instruction;
	offs_t next, size;
	u32 info;
	buffer.disassemble(logical, instruction, next, size, info);
	return util::string_format("%s: %s", handler, instruction);
}

void debug_view_stack::view_update()
{
	debug_view_stack_source const &source = downcast<debug_view_stack_source const &>(*m_source);
	address_space_config const *const stackconfig = source.m_memory.logical_space_config(source.m_stack_space);
	address_space_config const *const programconfig = source.m_memory.logical_space_config(AS_PROGRAM);
	u8 const bytes = pointer_bytes(*programconfig);
	offs_t const step = (std::max<offs_t>)(stackconfig->byte2addr(bytes), 1);
	u64 const stack = source.m_stack.value() & stackconfig->logaddrmask();
	int const chars = stackconfig->logaddrchars();
	int const valuechars = bytes * 2;
	int const valuecolumn = chars + 2;
	int const descriptioncolumn = valuecolumn + valuechars + 2;

	m_total = debug_view_xy((std::max)(descriptioncolumn + 48, 80), 256);
	debug_disasm_buffer buffer(*source.device());
	auto disable = machine().disable_side_effects();
	debug_view_char *dest = m_viewdata.data();

	for (int row = 0; row < m_visible.y; ++row)
	{
		int const index = m_topleft.y + row;
		std::string line;
		u8 attrib = DCA_NORMAL;
		if (index == 0)
		{
			line = util::string_format("%-*s  %-*s  Interpretation", chars, "Address", valuechars, "Value");
			attrib = DCA_ANCILLARY;
		}
		else
		{
			offs_t const address = (stack + offs_t(index - 1) * step) & stackconfig->logaddrmask();
			u64 const value = source.device()->debug()->symtable().read_memory(source.m_memory.space(source.m_stack_space), address, bytes, true);
			line = util::string_format("%0*X  %0*X  %s", chars, address, valuechars, value, describe_value(source, buffer, value));
			if (index == 1)
				attrib |= DCA_CURRENT;
		}

		for (int col = 0; col < m_visible.x; ++col, ++dest)
		{
			int const sourcecol = m_topleft.x + col;
			dest->byte = sourcecol < line.size() ? line[sourcecol] : ' ';
			dest->attrib = attrib;
		}
	}

	m_recompute = false;
}
