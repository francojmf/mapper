/*
 *    Copyright 2012 Thomas Schöps
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


#include "symbol_combined.h"

#include <QtGui>
#include <QIODevice>

#include "map.h"
#include "symbol_setting_dialog.h"
#include "symbol_properties_widget.h"

CombinedSymbol::CombinedSymbol() : Symbol(Symbol::Combined)
{
	parts.resize(2);
	parts[0] = NULL;
	parts[1] = NULL;
}

CombinedSymbol::~CombinedSymbol()
{
}

Symbol* CombinedSymbol::duplicate(const QHash<MapColor*, MapColor*>* color_map) const
{
	CombinedSymbol* new_symbol = new CombinedSymbol();
	new_symbol->duplicateImplCommon(this);
	new_symbol->parts = parts;
	return new_symbol;
}

void CombinedSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output)
{
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i])
			parts[i]->createRenderables(object, flags, coords, output);
	}
}

void CombinedSymbol::colorDeleted(MapColor* color)
{
	if (containsColor(color))
		resetIcon();
}

bool CombinedSymbol::containsColor(MapColor* color)
{
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i]->containsColor(color))
		{
			return true;
		}
	}
	return false;
}

bool CombinedSymbol::symbolChanged(Symbol* old_symbol, Symbol* new_symbol)
{
	bool have_symbol = false;
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i] == old_symbol)
		{
			have_symbol = true;
			parts[i] = new_symbol;
		}
	}
	
	// always invalidate the icon, since the parts might have changed.
	delete icon; 
	icon = NULL;
	
	return have_symbol;
}

bool CombinedSymbol::containsSymbol(const Symbol* symbol) const
{
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i] == symbol)
			return true;
		if (parts[i]->getType() == Symbol::Combined)	// TODO: see TODO in SymbolDropDown constructor.
		{
			CombinedSymbol* combined_symbol = reinterpret_cast<CombinedSymbol*>(parts[i]);
			if (combined_symbol->containsSymbol(symbol))
				return true;
		}
	}
	return false;
}

void CombinedSymbol::scale(double factor)
{
	resetIcon();
}

Symbol::Type CombinedSymbol::getContainedTypes() const
{
	int type = (int)getType();
	
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i])
			type |= parts[i]->getContainedTypes();
	}
	
	return (Type)type;
}

void CombinedSymbol::saveImpl(QIODevice* file, Map* map)
{
    int size = (int)parts.size();
	file->write((const char*)&size, sizeof(int));
	
	for (int i = 0; i < size; ++i)
	{
		int temp = (parts[i] == NULL) ? -1 : map->findSymbolIndex(parts[i]);
		file->write((const char*)&temp, sizeof(int));
	}
}

bool CombinedSymbol::loadImpl(QIODevice* file, int version, Map* map)
{
	int size;
	file->read((char*)&size, sizeof(int));
	temp_part_indices.resize(size);
	
	for (int i = 0; i < size; ++i)
	{
		int temp;
		file->read((char*)&temp, sizeof(int));
		temp_part_indices[i] = temp;
	}
	return true;
}

bool CombinedSymbol::equalsImpl(Symbol* other, Qt::CaseSensitivity case_sensitivity)
{
	CombinedSymbol* combination = static_cast<CombinedSymbol*>(other);
	if (parts.size() != combination->parts.size())
		return false;
	// TODO: parts are only compared in order
	for (size_t i = 0, end = parts.size(); i < end; ++i)
	{
		if ((parts[i] == NULL && combination->parts[i] != NULL) ||
			(parts[i] != NULL && combination->parts[i] == NULL))
			return false;
		if (parts[i] && !parts[i]->equals(combination->parts[i], case_sensitivity))
			return false;
	}
	return true;
}

bool CombinedSymbol::loadFinished(Map* map)
{
	int size = (int)temp_part_indices.size();
	if (size == 0)
		return true;
	parts.resize(size);
	for (int i = 0; i < size; ++i)
	{
		int index = temp_part_indices[i];
		if (index < 0 || index >= map->getNumSymbols())
			return false;
		parts[i] = map->getSymbol(index);
	}
	temp_part_indices.clear();
	return true;
}

SymbolPropertiesWidget* CombinedSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new CombinedSymbolSettings(this, dialog);
}

// ### CombinedSymbolSettings ###

const int CombinedSymbolSettings::max_count = 5;

CombinedSymbolSettings::CombinedSymbolSettings(CombinedSymbol* symbol, SymbolSettingDialog* dialog) 
 : SymbolPropertiesWidget(symbol, dialog), symbol(symbol)
{
	const CombinedSymbol* source_symbol = static_cast<const CombinedSymbol*>(dialog->getUnmodifiedSymbol());
	Map* source_map = dialog->getSourceMap();
	
	QFormLayout* layout = new QFormLayout();
	
	number_edit = new QSpinBox();
	number_edit->setRange(2, qMax<int>(max_count, symbol->getNumParts()));
	number_edit->setValue(symbol->getNumParts());
	connect(number_edit, SIGNAL(valueChanged(int)), this, SLOT(numberChanged(int)));
	layout->addRow(tr("&Number of parts:"), number_edit);
	
	symbol_labels = new QLabel*[max_count];
	symbol_edits = new SymbolDropDown*[max_count];
	for (int i = 0; i < max_count; ++i)
	{
		symbol_labels[i] = new QLabel(tr("Symbol %1:").arg(i+1));
		symbol_edits[i] = new SymbolDropDown(source_map, Symbol::Line | Symbol::Area | Symbol::Combined, ((int)symbol->parts.size() > i) ? symbol->parts[i] : NULL, source_symbol);
		connect(symbol_edits[i], SIGNAL(currentIndexChanged(int)), this, SLOT(symbolChanged(int)));
		layout->addRow(symbol_labels[i], symbol_edits[i]);
		
		if (i >= symbol->getNumParts())
		{
			symbol_labels[i]->hide();
			symbol_edits[i]->hide();
		}
	}
	
	QWidget* widget = new QWidget;
	widget->setLayout(layout);
	addPropertiesGroup(tr("Combination settings"), widget);
}

CombinedSymbolSettings::~CombinedSymbolSettings()
{
	delete[] symbol_labels;
	delete[] symbol_edits;
}

void CombinedSymbolSettings::numberChanged(int value)
{
	int old_num_items = symbol->getNumParts();
	if (old_num_items == value)
		return;
	
	int num_items = value;
	symbol->setNumParts(num_items);
	for (int i = 0; i < max_count; ++i)
	{
		symbol_labels[i]->setVisible(i < num_items);
		symbol_edits[i]->setVisible(i < num_items);
		
		if (i >= old_num_items && i < num_items)
		{
			// This item appears now
			symbol->setPart(i, NULL);
			symbol_edits[i]->blockSignals(true);
			symbol_edits[i]->setSymbol(NULL);
			symbol_edits[i]->blockSignals(false);
		}
	}
	emit propertiesModified();
}

void CombinedSymbolSettings::symbolChanged(int index)
{
	for (int i = 0; i < symbol->getNumParts(); ++i)
		symbol->setPart(i, symbol_edits[i]->symbol());
	emit propertiesModified();
}

void CombinedSymbolSettings::reset(Symbol* symbol)
{
	assert(symbol->getType() == Symbol::Combined);
	
	SymbolPropertiesWidget::reset(symbol);
	
	this->symbol = reinterpret_cast<CombinedSymbol*>(symbol);
	updateContents();
}

void CombinedSymbolSettings::updateContents()
{
	int num_parts = symbol->getNumParts();
	for (int i = 0; i < max_count; ++i)
	{
		symbol_edits[i]->blockSignals(true);
		if (i < num_parts)
		{
			symbol_edits[i]->setSymbol(symbol->parts[i]);
			symbol_edits[i]->show();
			symbol_labels[i]->show();
		}
		else
		{
			symbol_edits[i]->setSymbol(NULL);
			symbol_edits[i]->hide();
			symbol_labels[i]->hide();
		}
		symbol_edits[i]->blockSignals(false);
	}
	
	number_edit->blockSignals(true);
	number_edit->setValue(num_parts);
	number_edit->blockSignals(false);
}