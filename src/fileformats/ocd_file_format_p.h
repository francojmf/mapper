/*
 *    Copyright 2013 Kai Pastor
 *
 *    Some parts taken from file_format_oc*d8{.h,_p.h,cpp} which are
 *    Copyright 2012 Pete Curtis
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

#ifndef _OPENORIENTEERING_OCD_FILE_FORMAT_P_
#define _OPENORIENTEERING_OCD_FILE_FORMAT_P_

#include <QTextCodec>

#include "ocd_types.h"
#include "../file_import_export.h"
#include "../object.h"
#include "../object_text.h"
#include "../symbol.h"
#include "../symbol_area.h"
#include "../symbol_line.h"
#include "../symbol_point.h"
#include "../symbol_text.h"

class Georeferencing;
class MapColor;
class MapPart;
class OCAD8FileImport;
class Template;

/**
 * An map file importer for OC*D files.
 */
class OcdFileImport : public Importer
{
Q_OBJECT
protected:
	/// Information about an OC*D rectangle symbol
	struct RectangleInfo
	{
		LineSymbol* border_line;
		double corner_radius;
		bool has_grid;
		
		// Only valid if has_grid is true
		LineSymbol* inner_line;
		TextSymbol* text;
		bool number_from_bottom;
		double cell_width;
		double cell_height;
		int unnumbered_cells;
		QString unnumbered_text;
	};
	
	// Helper classes that provide to core classes' protected members
	
	class OcdImportedAreaSymbol : public AreaSymbol
	{
		friend class OcdFileImport;
	};
	
	class OcdImportedLineSymbol : public LineSymbol
	{
		friend class OcdFileImport;
	};
	
	class OcdImportedPointSymbol : public PointSymbol
	{
		friend class OcdFileImport;
	};
	
	class OcdImportedTextSymbol : public TextSymbol
	{
		friend class OcdFileImport;
	};
	
	class OcdImportedPathObject : public PathObject
	{
		friend class OcdFileImport;
		
	public:
		OcdImportedPathObject(Symbol* symbol = NULL) : PathObject(symbol) { }
	};
	
public:
	OcdFileImport(QIODevice* stream, Map *map, MapView *view);
	virtual ~OcdFileImport();
	
	void setCustom8BitEncoding(const char *encoding);
	
	template< std::size_t N >
	QString convertOcdString(const Ocd::PascalString<N>& src) const;
	
	template< std::size_t N >
	QString convertOcdString(const Ocd::Utf8PascalString< N >& src) const;
	
	template< class E >
	QString convertOcdString(const char* src, std::size_t len) const;
	
	template< class E >
	QString convertOcdString(const QByteArray& data) const;
	
	QString convertOcdString(const QChar* src) const;
	
	MapCoord convertOcdPoint(const Ocd::OcdPoint32& ocd_point) const;
	
	float convertAngle(int ocd_angle) const;
	
	template< class T >
	qint64 convertLength(T ocd_length) const;
	
	MapColor* convertColor(int ocd_color);
	
	void addSymbolWarning(LineSymbol* symbol, const QString& warning);
	
	void addSymbolWarning(TextSymbol* symbol, const QString& warning);
	
	virtual void finishImport() throw (FileFormatException);
	
protected:
	virtual void import(bool load_symbols_only) throw (FileFormatException);
	
	template< class F >
	void importImplementation(bool load_symbols_only) throw (FileFormatException);
	
	template< class F >
	void importGeoreferencing(const OcdFile< F >& file) throw (FileFormatException);
	
	void importGeoreferencing(const QString& param_string);
	
	template< class F >
	void importColors(const OcdFile< F >& file) throw (FileFormatException);
	
	MapColor* importColor(const QString& param_string);
	
	template< class F >
	void importSymbols(const OcdFile< F >& file) throw (FileFormatException);
	
	template< class F >
	void importObjects(const OcdFile< F >& file) throw (FileFormatException);
	
	template< class F >
	void importTemplates(const OcdFile< F >& file) throw (FileFormatException);
	
	Template* importTemplate(const QString& param_string, const int ocd_version);
	
	template< class F >
	void importExtras(const OcdFile< F >& file) throw (FileFormatException);
	
	template< class F >
	void importView(const OcdFile< F >& file) throw (FileFormatException);
	
	void importView(const QString& param_string);
	
	// Symbol import
	
	template< class S >
	PointSymbol* importPointSymbol(const S& ocd_symbol);
	
	template< class S >
	Symbol* importLineSymbol(const S& ocd_symbol);
	
	template< class S >
	AreaSymbol* importAreaSymbol(const S& ocd_symbol, int ocd_version);
	
	template< class S >
	TextSymbol* importTextSymbol(const S& ocd_symbol);
	
	template< class S >
	TextSymbol* importLineTextSymbol(const S& ocd_symbol);
	
	template< class S >
	LineSymbol* importRectangleSymbol(const S& ocd_symbol);
	
	template< class S >
	void setupBaseSymbol(Symbol* symbol, const S& ocd_symbol);
	
	template< class E >
	void setupPointSymbolPattern(PointSymbol* symbol, std::size_t data_size, const E* elements);
	
	// Object import
	
	template< class O >
	Object* importObject(const O& ocd_object, MapPart* part);
	
	template< class O >
	QString getObjectText(const O& ocd_object) const;
	
	template< class O >
	Object* importRectangleObject(const O& ocd_object, MapPart* part, const OcdFileImport::RectangleInfo& rect);
	
	// Some helper functions that are used in multiple places
	
	void setPointFlags(OcdImportedPathObject* object, quint16 pos, bool is_area, const Ocd::OcdPoint32& ocd_point);
	
	void setPathHolePoint(OcdFileImport::OcdImportedPathObject* object, int i);
	
	void fillPathCoords(OcdFileImport::OcdImportedPathObject* object, bool is_area, quint16 num_points, const Ocd::OcdPoint32* ocd_points);
	
	bool fillTextPathCoords(TextObject* object, TextSymbol* symbol, quint16 npts, const Ocd::OcdPoint32* ocd_points);
	
protected:
	/// The locale is used for number formatting.
	QLocale locale;
	
	QByteArray buffer;
	
	QScopedPointer< OCAD8FileImport > delegate;
	
	/// Character encoding to use for 1-byte (narrow) strings
	QTextCodec *custom_8bit_encoding;
	
	/// maps OCD color number to oo-mapper color object
	QHash<int, MapColor *> color_index;
	
	/// maps OCD symbol number to oo-mapper symbol object
	QHash<int, Symbol *> symbol_index;
	
	/// maps OO Mapper text symbol pointer to OCD defined horizontal alignment (stored in objects instead of symbols in OO Mapper)
	QHash<Symbol*, TextObject::HorizontalAlignment> text_halign_map;
	
	/// maps OO Mapper text symbol pointer to OCD defined vertical alignment (stored in objects instead of symbols in OO Mapper)
	QHash<Symbol*, TextObject::VerticalAlignment> text_valign_map;
	
	/// maps OCD symbol number to rectangle information struct
	QHash<int, RectangleInfo> rectangle_info;
};



// ### OcdFileImport inline code ###

template< std::size_t N >
inline
QString OcdFileImport::convertOcdString(const Ocd::PascalString<N>& src) const
{
	return custom_8bit_encoding->toUnicode(src.data, src.length);
}

template< std::size_t N >
inline
QString OcdFileImport::convertOcdString(const Ocd::Utf8PascalString<N>& src) const
{
	return QString::fromUtf8(src.data, src.length);
}

template< >
inline
QString OcdFileImport::convertOcdString< Ocd::Custom8BitEncoding >(const char* src, std::size_t len) const
{
	len = qstrnlen(src, len);
	return custom_8bit_encoding->toUnicode(src, len);
}

template< >
inline
QString OcdFileImport::convertOcdString< Ocd::Utf8Encoding >(const char* src, std::size_t len) const
{
	len = qstrnlen(src, len);
	return QString::fromUtf8(src, len);
}

template< >
inline
QString OcdFileImport::convertOcdString< Ocd::Custom8BitEncoding >(const QByteArray& data) const
{
	return custom_8bit_encoding->toUnicode(data.constData(), data.length());
}

template< >
inline
QString OcdFileImport::convertOcdString< Ocd::Utf8Encoding >(const QByteArray& data) const
{
	return QString::fromUtf8(data.constData(), data.length());
}

inline
QString OcdFileImport::convertOcdString(const QChar* src) const
{
	return QString(src);
}

inline
MapCoord OcdFileImport::convertOcdPoint(const Ocd::OcdPoint32& ocd_point) const
{
	// Behavior of operator>>() on negative integers is implementation defined.
	// Define SHIFT_OPERATOR_IS_BINARY if the compiler does not implement
	// operator>>() as arithmetic shift (maintaining the sign).
#if defined(SHIFT_OPERATOR_IS_BINARY)
	const qint64 x = (ocd_point.x > 0) ? (ocd_point.x >> 8) : (-1 - ((-1-ocd_point.x) >> 8));
	const qint64 y = (ocd_point.y > 0) ? (ocd_point.y >> 8) : (-1 - ((-1-ocd_point.y) >> 8));
	return MapCoord::fromRaw(x * 10, y * -10);
#else // shift operator is arithmetic
	Q_ASSERT( (-3 >> 1) == -2 );
	return MapCoord::fromRaw(qint64(ocd_point.x >> 8) * 10, qint64(ocd_point.y >> 8) * -10);
#endif
}

inline
float OcdFileImport::convertAngle(int ocd_angle) const
{
	// OC*D uses tenths of a degree, counterclockwise
	// BUG: if sin(rotation) is < 0 for a hatched area pattern, the pattern's createRenderables() will go into an infinite loop.
	// So until that's fixed, we keep a between 0 and PI
	return (M_PI / 1800) * ((ocd_angle + 3600) % 3600);
}

template< class T >
inline
qint64 OcdFileImport::convertLength(T ocd_length) const
{
	// OC*D uses hundredths of a millimeter.
	// oo-mapper uses 1/1000 mm
	return ((qint64)ocd_length) * 10;
}

inline
MapColor *OcdFileImport::convertColor(int ocd_color)
{
	if (!color_index.contains(ocd_color))
	{
		addWarning(tr("Color id not found: %1, ignoring this color").arg(ocd_color));
		return NULL;
	}
	
	return color_index[ocd_color];
}


#endif // _OPENORIENTEERING_OCD_FILE_FORMAT_P_