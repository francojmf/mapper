/*
 *    Copyright 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_GPS_DISPLAY_H_
#define _OPENORIENTEERING_GPS_DISPLAY_H_

#include <QObject>
#if defined(ENABLE_POSITIONING)
	#include <QtPositioning/QGeoPositionInfo>
	#include <QtPositioning/QGeoPositionInfoSource>
#endif

#include "map_coord.h"

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE
class MapWidget;
class Georeferencing;
class QGeoPositionInfo;
class QGeoPositionInfoSource;


/**
 * Retrieves the GPS position and displays a marker at this position on a MapWidget.
 */
class GPSDisplay : public QObject
{
Q_OBJECT
public:
	/// Creates a GPS display for the given map widget and georeferencing.
	GPSDisplay(MapWidget* widget, const Georeferencing& georeferencing);
	/// Destructor, removes the GPS display from the map widget.
	~GPSDisplay();
	
	/// Starts regular position updates. This will issue redraws of the map widget.
	void startUpdates();
	/// Stops regular position updates.
	void stopUpdates();
	
	/// Sets GPS marker visibility (true by default)
	void setVisible(bool visible);
	/// Returns GPS marker visibility
	inline bool isVisible() const {return visible;}
	
	/// Sets whether distance rings are drawn
	void enableDistanceRings(bool enable);
	/// Sets whether the current heading from the Compass is used to draw a heading indicator.
	void enableHeadingIndicator(bool enable);
	
	/// This is called from the MapWidget drawing code to draw the GPS position marker.
	void paint(QPainter* painter);
	
	/// Returns if a valid position was received since the last call to startUpdates().
	inline bool hasValidPosition() const {return has_valid_position;}
	/// Returns the latest received GPS coord. Check hasValidPosition() beforehand!
	const MapCoordF& getLatestGPSCoord() const {return latest_gps_coord;}
	/// Returns the accuracy of the latest received GPS coord, or -1 if unknown. Check hasValidPosition() beforehand!
	float getLatestGPSCoordAccuracy() const {return latest_gps_coord_accuracy;}
	
signals:
	/// Is emitted whenever a new position update happens.
	/// If the accuracy is unknown, -1 will be given.
	void mapPositionUpdated(MapCoordF coord, float accuracy);
	
	/// Like mapPositionUpdated(), but gives the values as
	/// latitude / longitude in degrees and also gives altitude
	/// (meters above sea level; -9999 is unknown)
	void latLonUpdated(double latitude, double longitude, double altitude, float accuracy);
	
	/// Is emitted when updates are interrupted after previously being active,
	/// due to loss of satellite reception or another error such as the user
	/// turning off the GPS receiver.
	void positionUpdatesInterrupted();
	
private slots:
#if defined(ENABLE_POSITIONING)
    void positionUpdated(const QGeoPositionInfo& info);
	void error(QGeoPositionInfoSource::Error positioningError);
	void updateTimeout();
#endif
	void debugPositionUpdate();
	
private:
	MapCoordF calcLatestGPSCoord(bool& ok);
	void updateMapWidget();
	
	bool gps_updated;
#if defined(ENABLE_POSITIONING)
	QGeoPositionInfo latest_pos_info;
#endif
	MapCoordF latest_gps_coord;
	float latest_gps_coord_accuracy;
	bool tracking_lost;
	bool has_valid_position;
	
	bool visible;
	bool distance_rings_enabled;
	bool heading_indicator_enabled;
	
	QGeoPositionInfoSource* source;
	MapWidget* widget;
	const Georeferencing& georeferencing;
};

#endif