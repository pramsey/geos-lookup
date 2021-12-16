/*
*  SpatialLookup.cpp
*
*  Copyright (c) 2021 Paul Ramsey. All rights reserved.
*  MIT License
*/

#pragma once

// System headers
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cstdlib>

// GEOS headers
#include <geos/geom/Coordinate.h>
#include <geos/geom/Envelope.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Point.h>
#include <geos/geom/prep/PreparedGeometry.h>
#include <geos/geom/prep/PreparedGeometryFactory.h>
#include <geos/index/strtree/TemplateSTRtree.h>
#include <geos/io/GeoJSON.h>
#include <geos/io/GeoJSONReader.h>

// Short names
using geos::geom::GeometryFactory;
using geos::geom::Coordinate;
using geos::geom::Envelope;
using geos::geom::Geometry;
using geos::geom::Point;
using geos::geom::prep::PreparedGeometry;
using geos::geom::prep::PreparedGeometryFactory;
using geos::index::strtree::EnvelopeTraits;
using geos::index::strtree::TemplateSTRtree;
using geos::io::GeoJSONFeature;
using geos::io::GeoJSONFeatureCollection;
using geos::io::GeoJSONReader;
using geos::io::GeoJSONValue;


/**
 * This class wraps up the functionality needed to provide an
 * in-memory reverse geocoder. At start-up, the class reads in
 * a GeoJSON feature collection, identifies the polygonal features,
 * creates PreparedGeometry for each polygon, and places those
 * into an STRtree.
 *
 * This provides a high-performance in-memory lookup: for an
 * input coordinate, the STRtree is first used to find all geometry
 * with a bounding box overlap to the point, then the prepared
 * geometry for those are used for a high performance test of
 * whether the coordinate actually intersects the shape.
 */
class SpatialLookup {

public:

    /**
     * This class is a utility class, which encapsulates the
     * prepared geometry and the GeoJSON feature into one
     * object so that the lookup routine can very efficiently
     * run the prepared intersects test and then lookup
     * the return values from the GeoJSON properties. The
     * class is also outfitted with a getEnvelopeInternal
     * method so that it can be used directly in the STRtree.
     */
    class LookupEntry {

    public:

        // Constructor
        LookupEntry(const GeoJSONFeature& feature)
            : m_feature(feature) // take a copy of feature first, then use it next
            , m_prepgeom(PreparedGeometryFactory::prepare(m_feature.getGeometry()))
            {};

        /**
         * Return the Envelope of the geometry that was used to
         * construct this entry. This method is called by the
         * TemplateSTRtree when inserting new entries into the
         * tree.
         */
        const Envelope* getEnvelopeInternal() const;
        const GeoJSONFeature& getFeature() const;
        bool intersects(const Coordinate& coord) const;

    private:

        // Members
        GeoJSONFeature m_feature;
        std::unique_ptr<PreparedGeometry> m_prepgeom;

    };

    /**
     * The filename is the GeoJSON file to read for the
     * geometries to index. The property is the feature
     * property to return for geometries that hit the
     * test coordinates. For example "name" if the features
     * have a "name" property.
     */
    SpatialLookup(const std::string& filename, const std::string& property)
        : m_filename(filename)
        , m_property(property)
        , m_index(nullptr)
        , m_dataready(false)
    {
        m_dataready = readGeoJsonFile() && createIndex();
    }

    /**
     * Given a coordinate, search the spatial index and
     * return a list of values for the property of interest.
     */
    std::vector<std::string> lookup(const Coordinate& coord) const;
    bool ready(void) const {
        return m_dataready;
    }

private:

    // Members
    const std::string m_filename;
    const std::string m_property;
    std::unique_ptr<TemplateSTRtree<LookupEntry*, EnvelopeTraits>> m_index;
    std::vector<LookupEntry> m_lookups;
    bool m_dataready;

    // Methods
    bool readGeoJsonFile();
    bool createIndex();

};

