// license:BSD-3-Clause
// copyright-holders:MAMEdev Team

#include "emu.h"
#include "stackwindow.h"

#include "debugger.h"
#include "debug/debugcon.h"
#include "util/xmlfile.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QVBoxLayout>

namespace osd::debugger::qt {

StackWindow::StackWindow(DebuggerQt &debugger, QWidget *parent)
	: WindowQt(debugger, nullptr)
{
	setWindowTitle("Debug: Stack");
	if (parent)
	{
		QPoint const pos = parent->pos();
		setGeometry(pos.x() + 100, pos.y() + 100, 900, 500);
	}

	QWidget *const frame = new QWidget(this);
	m_source = new QComboBox(frame);
	m_view = new DebuggerView(DVT_STACK, m_machine, frame);
	for (auto const &source : m_view->view()->source_list())
		m_source->addItem(source->name());
	connect(m_source, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this] (int index)
	{
		if ((0 <= index) && (index < m_view->view()->source_count()))
			m_view->view()->set_source(*m_view->view()->source(index));
	});

	device_t *const cpu = m_machine.debugger().console().get_visible_cpu();
	if (cpu)
	{
		debug_view_source const *const source = m_view->view()->source_for_device(cpu);
		if (source)
			m_source->setCurrentIndex(m_view->view()->source_index(*source));
	}

	QVBoxLayout *const layout = new QVBoxLayout(frame);
	layout->setContentsMargins(2, 2, 2, 2);
	layout->addWidget(m_source);
	layout->addWidget(m_view);
	setCentralWidget(frame);
}

void StackWindow::restoreConfiguration(util::xml::data_node const &node)
{
	WindowQt::restoreConfiguration(node);
	int const source = node.get_attribute_int(ATTR_WINDOW_MEMORY_REGION, m_source->currentIndex());
	if ((0 <= source) && (source < m_source->count()))
		m_source->setCurrentIndex(source);
	m_view->restoreConfigurationFromNode(node);
}

void StackWindow::saveConfigurationToNode(util::xml::data_node &node)
{
	WindowQt::saveConfigurationToNode(node);
	node.set_attribute_int(ATTR_WINDOW_TYPE, WINDOW_TYPE_STACK_VIEWER);
	node.set_attribute_int(ATTR_WINDOW_MEMORY_REGION, m_source->currentIndex());
	m_view->saveConfigurationToNode(node);
}

} // namespace osd::debugger::qt
