﻿/*
 *    Copyright 2012 Kai Pastor
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


#include "georeferencing.h"

#include <cassert>

#include <QLocale>
#include <QDebug>

#include <proj_api.h>

const QString Georeferencing::geographic_crs_spec("+proj=latlong +datum=WGS84");

Georeferencing::Georeferencing()
: scale_denominator(0),
  declination(0.0),
  grivation(0.0),
  map_ref_point(0, 0),
  projected_ref_point(0, 0),
  geographic_ref_point(0, 0)
{
	updateTransformation();
	
	projected_crs_id = tr("Local coordinates");
	projected_crs  = NULL;
	geographic_crs = pj_init_plus(geographic_crs_spec.toAscii());
	assert(geographic_crs != NULL);
}

Georeferencing::Georeferencing(const Georeferencing& other)
: scale_denominator(other.scale_denominator),
  declination(other.declination),
  grivation(other.grivation),
  map_ref_point(other.map_ref_point),
  projected_ref_point(other.projected_ref_point),
  projected_crs_id(other.projected_crs_id),
  projected_crs_spec(other.projected_crs_spec),
  geographic_ref_point(other.geographic_ref_point)
{
	updateTransformation();
	
	projected_crs  = pj_init_plus(projected_crs_spec.toAscii());
	geographic_crs = pj_init_plus(geographic_crs_spec.toAscii());
	assert(geographic_crs != NULL);
}

Georeferencing::~Georeferencing()
{
	if (projected_crs != NULL)
		pj_free(projected_crs);
	if (geographic_crs != NULL)
		pj_free(geographic_crs);
}

Georeferencing& Georeferencing::operator=(const Georeferencing& other)
{
	scale_denominator   = other.scale_denominator;
	declination         = other.declination;
	grivation           = other.grivation;
	map_ref_point       = other.map_ref_point;
	projected_ref_point = other.projected_ref_point;
	from_projected      = other.from_projected;
	to_projected        = other.to_projected;
	projected_ref_point = other.projected_ref_point;
	projected_crs_id    = other.projected_crs_id;
	projected_crs_spec  = other.projected_crs_spec;
	geographic_ref_point= other.geographic_ref_point;
	
	updateTransformation();
	
	if (projected_crs != NULL)
		pj_free(projected_crs);
	projected_crs       = pj_init_plus(projected_crs_spec.toAscii());
	emit projectionChanged();
	
	return *this;
}

void Georeferencing::setScaleDenominator(int value)
{
	scale_denominator = value;
	updateTransformation();
}

void Georeferencing::setDeclination(double value)
{
	grivation += value - declination;
	declination = value;
	updateTransformation();
}

void Georeferencing::setGrivation(double value)
{
	declination += value - grivation;
	grivation = value;
	updateTransformation();
}

void Georeferencing::setMapRefPoint(MapCoord point)
{
	map_ref_point = point;
	updateTransformation();
}

void Georeferencing::setProjectedRefPoint(QPointF point)
{
	projected_ref_point = point;
	bool ok;
	LatLon new_geo_ref = toGeographicCoords(point, &ok);
	if (ok)
		geographic_ref_point = new_geo_ref;
	updateGrivation();
	updateTransformation();
}

double Georeferencing::getConvergence() const
{
	if (isLocal())
		return 0.0;
	
	const double delta_phi = M_PI / 20000.0;  // roughly 1 km, TODO: replace by literal constant.
	LatLon geographic_other = geographic_ref_point;
	geographic_other.latitude += (geographic_other.latitude < 0.0) ? delta_phi : -delta_phi; // 2nd point on the same meridian
	QPointF projected_other = toProjectedCoords(geographic_other);
	
	double denominator = projected_other.y() - projected_ref_point.y();
	if (fabs(denominator) < 0.00000000001)
		return 0.0;
	
	return RAD_TO_DEG * atan((projected_ref_point.x() - projected_other.x()) / denominator);
}

void Georeferencing::setGeographicRefPoint(LatLon lat_lon)
{
	bool ok;
	QPointF new_projected_ref = toProjectedCoords(lat_lon, &ok);
	if (ok)
		projected_ref_point = new_projected_ref;
	geographic_ref_point = lat_lon;
	if (ok)
	{
		updateGrivation();
		updateTransformation();
	}
}

void Georeferencing::updateTransformation()
{
	const QTransform old(to_projected);
	to_projected.reset();
	double scale = double(scale_denominator) / 1000.0;
	to_projected.translate(projected_ref_point.x(), projected_ref_point.y());
	to_projected.rotate(-grivation);
	to_projected.scale(scale, -scale);
	to_projected.translate(-map_ref_point.xd(), -map_ref_point.yd());
	
	if (old != to_projected)
	{
		from_projected = to_projected.inverted();
		emit transformationChanged();
	}
}

bool Georeferencing::updateGrivation()
{
	const double old_value = grivation;
	grivation = declination - getConvergence();
	return (old_value != grivation);
}

void Georeferencing::initDeclination()
{
	if (isLocal())
	{
		// Maybe not yet initialized
		projected_crs = pj_init_plus(projected_crs_spec.toAscii());
		if (projected_crs != NULL)
			emit projectionChanged();
	}

	declination = grivation + getConvergence();
}

bool Georeferencing::setProjectedCRS(const QString& id, const QString& spec)
{
	if (projected_crs != NULL)
		pj_free(projected_crs);
	
	this->projected_crs_id = id;
	this->projected_crs_spec = spec;
	projected_crs = pj_init_plus(projected_crs_spec.toAscii());
	if (updateGrivation())
		updateTransformation();
	
	emit projectionChanged();
	return projected_crs != NULL;
}

QPointF Georeferencing::toProjectedCoords(const MapCoord& map_coords) const
{
	return to_projected.map(map_coords.toQPointF());
}

QPointF Georeferencing::toProjectedCoords(const MapCoordF& map_coords) const
{
	return to_projected.map(map_coords.toQPointF());
}

MapCoord Georeferencing::toMapCoords(const QPointF& projected_coords) const
{
	return MapCoordF(from_projected.map(projected_coords)).toMapCoord();
}

MapCoordF Georeferencing::toMapCoordF(const QPointF& projected_coords) const
{
	return MapCoordF(from_projected.map(projected_coords));
}

LatLon Georeferencing::toGeographicCoords(const MapCoordF& map_coords, bool* ok) const
{
	return toGeographicCoords(toProjectedCoords(map_coords), ok);
}

LatLon Georeferencing::toGeographicCoords(const QPointF& projected_coords, bool* ok) const
{
	if (ok != NULL)
		*ok = false;

	double easting = projected_coords.x(), northing = projected_coords.y();
	if (projected_crs && geographic_crs) {
		int ret = pj_transform(projected_crs, geographic_crs, 1, 1, &easting, &northing, NULL);
		if (ok != NULL) 
			*ok = (ret == 0);
	}
	return LatLon(northing, easting);
}

QPointF Georeferencing::toProjectedCoords(const LatLon& lat_lon, bool* ok) const
{
	if (ok != NULL)
		*ok = false;
	
	double easting = lat_lon.longitude, northing = lat_lon.latitude;
	if (projected_crs && geographic_crs) {
		int ret = pj_transform(geographic_crs, projected_crs, 1, 1, &easting, &northing, NULL);
		if (ok != NULL) 
			*ok = (ret == 0);
	}
	return QPointF(easting, northing);
}

MapCoord Georeferencing::toMapCoords(const LatLon& lat_lon, bool* ok) const
{
	return toMapCoords(toProjectedCoords(lat_lon, ok));
}

MapCoordF Georeferencing::toMapCoordF(const LatLon& lat_lon, bool* ok) const
{
	return toMapCoordF(toProjectedCoords(lat_lon, ok));
}

QString Georeferencing::getErrorText() const
{
	int err_no = *pj_get_errno_ref();
	return (err_no == 0) ? "" : pj_strerrno(err_no);
}

double Georeferencing::radToDeg(double val)
{
	return RAD_TO_DEG * val;
}

QString Georeferencing::radToDMS(double val)
{
	qint64 tmp = RAD_TO_DEG * val * 360000;
	int csec = tmp % 6000;
	tmp = tmp / 6000;
	int min = tmp % 60;
	int deg = tmp / 60;
	QString ret = QString::fromUtf8("%1°%2'%3\"").arg(deg).arg(min).arg(QLocale().toString(csec/100.0,'f',2));
	return ret;
}

QDebug operator<<(QDebug dbg, const Georeferencing &georef)
{
	dbg.nospace() 
	  << "Georeferencing(1:" << georef.scale_denominator
	  << " " << georef.declination
	  << " " << georef.grivation
	  << "deg, " << georef.projected_crs_id
	  << " (" << georef.projected_crs_spec
	  << ") " << georef.projected_ref_point.x() << "," << georef.projected_ref_point.y();
	if (georef.isLocal())
		dbg.nospace() << ", local)";
	else
		dbg.nospace() << ", geographic)";
	
	return dbg.space();
}

QDebug operator<<(QDebug dbg, const LatLon& lat_lon)
{
	dbg.space() 
	  << "LatLon" << lat_lon.latitude << lat_lon.longitude
	  << "(" << Georeferencing::radToDeg(lat_lon.latitude)
	  << Georeferencing::radToDeg(lat_lon.longitude) << ")";
	return dbg.space();
}