/*
 *    Copyright 2012, 2013 Thomas Schöps
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util_task_dialog.h"

#include <QCommandLinkButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QSignalMapper>


TaskDialog::TaskDialog(QWidget* parent, const QString& title, const QString& text, QDialogButtonBox::StandardButtons buttons)
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(title);
	
	QLabel* text_label = NULL;
	if (!text.isEmpty())
		text_label = new QLabel(text);
	
	button_box = NULL;
	if (buttons != QDialogButtonBox::NoButton)
		button_box = new QDialogButtonBox(buttons);
	
	layout = new QVBoxLayout();
	if (text_label)
		layout->addWidget(text_label);
	if (button_box)
		layout->addWidget(button_box);
	setLayout(layout);
	
	signal_mapper = new QSignalMapper(this);
	connect(signal_mapper, SIGNAL(mapped(QWidget*)), this, SLOT(buttonClicked(QWidget*)));
	connect(button_box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

QCommandLinkButton* TaskDialog::addCommandButton(const QString& text, const QString& description)
{
	QCommandLinkButton* button = new QCommandLinkButton(text, description);
	signal_mapper->setMapping(button, button);
	connect(button, SIGNAL(clicked()), signal_mapper, SLOT(map()));
	
	layout->insertWidget(layout->count() - (button_box ? 1 : 0), button);
	return button;
}

void TaskDialog::buttonClicked(QWidget* button)
{
	buttonClicked(static_cast<QAbstractButton*>(button));
}
void TaskDialog::buttonClicked(QAbstractButton* button)
{
	clicked_button = button;
	if (button_box->buttonRole(clicked_button) == QDialogButtonBox::RejectRole)
		reject();
	else
		accept();
}
