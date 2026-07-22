// license:BSD-3-Clause
// copyright-holders:MAMEdev Team
#ifndef MAME_DEBUGGER_QT_STACKWINDOW_H
#define MAME_DEBUGGER_QT_STACKWINDOW_H

#pragma once

#include "debuggerview.h"
#include "windowqt.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace osd::debugger::qt {

class StackWindow : public WindowQt
{
public:
	StackWindow(DebuggerQt &debugger, QWidget *parent = nullptr);

	virtual void restoreConfiguration(util::xml::data_node const &node) override;

protected:
	virtual void saveConfigurationToNode(util::xml::data_node &node) override;

private:
	QComboBox *m_source;
	DebuggerView *m_view;
};

} // namespace osd::debugger::qt

#endif // MAME_DEBUGGER_QT_STACKWINDOW_H
