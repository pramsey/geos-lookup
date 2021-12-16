/*
*  SpatialLookup.cpp
*
*  Copyright (c) 2021 Paul Ramsey. All rights reserved.
*  MIT License
*/

// App headers
#include "SpatialLookup.h"

/*************************************************************************
 * SpatialLookup::LookupEntry
 */

const Envelope*
SpatialLookup::LookupEntry::getEnvelopeInternal() const
{
    return m_prepgeom->getGeometry().getEnvelopeInternal();
}

bool
SpatialLookup::LookupEntry::intersects(const Coordinate& coord) const
{
    const GeometryFactory* gf = m_prepgeom->getGeometry().getFactory();
    std::unique_ptr<Point> pt(gf->createPoint(coord));
    return m_prepgeom->intersects(pt.get());
}

const GeoJSONFeature&
SpatialLookup::LookupEntry::getFeature() const
{
    return m_feature;
}


/*************************************************************************
 * SpatialLookup
 */

bool
SpatialLookup::readGeoJsonFile()
{
    // Load the filename into an in-memory string
    std::ifstream ifs(m_filename);

    // File is not readable / does not exist
    if(!ifs) {
        std::cerr << "spatial_lookup: unable to load file '" << m_filename << "'" << std::endl;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs) ),
                        (std::istreambuf_iterator<char>()    ));

    try {
        // Parse the GeoJSON string into a feature collection
        GeoJSONReader reader;
        GeoJSONFeatureCollection fc = reader.readFeatures(content);

        // Just stop if there are no features
        if (fc.getFeatures().empty()) {
            std::cerr << "spatial_lookup: no features in file '" << m_filename << "'" << std::endl;
            return false;
        }

        // We can't easily hold a reference to the collection
        // so we copy out what we care about, the vector of features
        for (auto& feature: fc.getFeatures()) {
            const Geometry* geom = feature.getGeometry();
            if (geom && geom->isPolygonal()) {
                m_lookups.emplace_back(feature);
            }
        }        
    }
    catch (std::exception& e) {
        std::string what(e.what());
        std::cerr << "spatial_lookup: failed to parse file '" << m_filename << "'" << std::endl;
        std::cerr << "spatial_lookup: " << what << std::endl;
        return false;
    }

    return true;
}


bool
SpatialLookup::createIndex()
{
    // Set up a new empty spatial index. Pre-reserving the 
    // size will make building a tiny bit faster.
    m_index.reset(new TemplateSTRtree<LookupEntry*, EnvelopeTraits>(m_lookups.size()));

    // Populate the index with LookupEntry* pointers. Because
    // they have a getEnvelopeInternal() method, the index 
    // can handle them directly.
    for (auto& entry: m_lookups) {
        m_index->insert(&entry);
    }
    return true;
}


std::vector<std::string>
SpatialLookup::lookup(const Coordinate& coord) const
{
    // Return value
    std::vector<std::string> properties;
    // In unfortunate case we're running without data, just return
    if (!m_dataready)
        return properties;

    // Lambda for the STRtree index search. If we've got a hit that 
    // intersects with the underlying polygon, then look into
    // the feature and read back the desired property.
    auto visitor = [&properties, &coord, this](const LookupEntry* e) {
        if (e->intersects(coord)) {
            const GeoJSONFeature& feature = e->getFeature();
            auto& props = feature.getProperties();
            GeoJSONValue v = props.at(m_property);
            properties.push_back(v.getString());
        }
    };

    // Run the query with the callback.
    Envelope qe(coord.x, coord.x, coord.y, coord.y);
    m_index->query(qe, visitor);
    return properties;
}


/*************************************************************************
 * main
 */

/*
 * HTTP library is from 
 * https://github.com/yhirose/cpp-httplib 
 */
#include "vend/httplib.h"
using namespace httplib;

/**
 * Convert a vector of strings into a JSON array.
 * Must be a nicer way to do this.
 */
static std::string
hits_to_json(std::vector<std::string>& hits)
{
    std::stringstream ss;
    ss << "[";
    std::size_t count = 0;
    std::size_t length = std::distance(hits.begin(), hits.end());
    for (auto it = hits.begin(); it != hits.end(); ++it, ++count) {
        ss << '"' << *it << '"';
        if (count < length - 1)
            ss << ",";
    }
    ss << "]" << std::endl;
    return ss.str();
}

/**
 * Run it!
 */
int
main(int argc, char* argv[])
{
    // Default listen address
    unsigned int port = 8080;
    const std::string host = "localhost";

    // Two commandline arguments are required:
    // The geojson file, and the property to respond with.
    if (argc != 3) {
        std::cerr << "Usage: spatial_lookup geojson.json property" << std::endl;
        exit(1);
    }

    // Load the file and build the indexes
    SpatialLookup splu(argv[1], argv[2]);
    if (!splu.ready()) {
        std::cerr << "spatial_lookup: data load failed" << std::endl;
        return 1;
    }
    std::cerr << "spatial_lookup: loaded and indexed " << argv[1] << std::endl;

    // Set up HTTP end point, read the 'x' and 'y' HTTP request
    // parameters
    Server svr;
    svr.Get("/lookup", [&splu](const Request& req, Response& res) {
        // Only respond to a request with both parameters
        if(req.has_param("x") && req.has_param("y")) {
            std::string sX = req.get_param_value("x");
            std::string sY = req.get_param_value("y");
            // Unparsable numbers result in 0.0 so Null Island
            // may be a common query request
            double x = std::atof(sX.c_str());
            double y = std::atof(sY.c_str());
            // Indexed lookup of the coordinate against the data!
            std::vector<std::string> hits = splu.lookup(Coordinate(x, y));
            res.set_content(hits_to_json(hits), "application/json");
        }
    });

    // Start the server
    std::cerr << "spatial_lookup: listening on " << host << ":" << port << std::endl;
    svr.listen("localhost", 8080);
    return 0;
}


