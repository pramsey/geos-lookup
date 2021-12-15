# Spatial Lookup Web Service

This [GEOS](https://libgeos.org) example program demonstrates the use of the STRtree index and PreparedGeometry to create a high-performance in-memory reverse geocoder.

A generic reverse geocoder takes in coordinates and returns addresses. This simple application works with collections of polygons. It takes in a query coordinate and returns data from the polygons that the query intersects with.


## How It Works

In order to efficiently search a large collection of polygons, the server applies two indexes to the collection:

* Each polygon is internally indexed by creating a [PreparedGeometry](https://libgeos.org/doxygen/classgeos_1_1geom_1_1prep_1_1PreparedGeometry.html) from the geometry.
* The geometries are then all added to a [TemplateSTRtree](https://libgeos.org/doxygen/classgeos_1_1index_1_1strtree_1_1TemplateSTRtreeImpl.html).

When a coordinate comes in, the query is executed by:

* First querying the STRtree, to find all geometries that have bounding boxes that intersect the coordinate.
* Second for each geometry found, running the prepared geometry [intersects()](https://libgeos.org/doxygen/classgeos_1_1geom_1_1prep_1_1PreparedGeometry.html#a05f062f6af7ad3f600069500aaacde93) method to determine if the point is actually inside the geometry.

Basically the two indexes act at different levels. The STRtree indexes the bounds of all the polygons. The PreparedGeometry indexes the segments of each separate geometry.


## Implementation Details

The lookup service is contained within the `SpatialLookup` class. This class holds the parsed GeoJSON objects, which in turn hold the properties list for each object. It also holds the STRtree index, and a vector of `LookupEntry` objects for each polygonal `GeoJSONFeature`.

Within the `SpatialLookup` class the `LookupEntry` class encapsulates the `PreparedGeometry` and `GeoJSONFeature` into a single object that can be added to the STRtree. This reduces the lookup code to the bare minimum:

* find all entries in the STRtree that intersect the query,
* check each entry for intersection,
* read the desired property from the GeoJSON properties of each intersecting entry.

The HTTP interface is provided here by [cpp-httplib](https://github.com/yhirose/cpp-httplib), but it could be provided by any HTTP library you choose.


## Building and Running

To build the code, create a clean build directory and then build. Using a separate build directory makes cleaning up build artifacts very easy.

```
mkdir _build
cd _build
cmake ..
make
```

To run the application, ensure you have a GeoJSON FeatureCollection file of polygons handy. (Use the example below, or download a [state zip code file](https://github.com/OpenDataDE/State-zip-code-GeoJSON) perhaps.)

Then run the executable, supplying the GeoJSON file name and the name of the property you want returned when your query point hits a polygon. For example, using a Maryland zipcode file:

```
# ./spatial_lookup md_maryland_zip_codes_geo.min.json ZCTA5CE10

./spatial_lookup loaded and indexed md_maryland_zip_codes_geo.min.json
./spatial_lookup listening on localhost:8080

```

Querying the service then looks like this:

```
curl "http://localhost:8080/lookup?x=-78.40&y=39.69"

["21766"]
```

## Example GeoJSON File

Use "name" as your property.

```
./spatial_lookup geojson.json name
```

There two polygons surrounding the cell at 0,0.

```
curl http://localhost:8080/lookup?x=0.5&y=0.5
```

Example data for `geojson.json` file.

```json
{ "type": "FeatureCollection",
  "features": [
    { "type": "Feature",
      "geometry": {
        "type": "Polygon",
        "coordinates": [
          [ [0.0, 0.0], [1.0, 0.0], [1.0, 1.0],
             [0.0, 1.0], [0.0, 0.0] ]
          ]
      },
      "properties": {
         "name": "Peter",
         "age": 50.8
      }
    },
    { "type": "Feature",
      "geometry": {
        "type": "Polygon",
        "coordinates": [
          [ [0.0, 0.0], [1.0, 0.0], [1.0, 1.0],
             [0.0, 1.0], [0.0, 0.0] ]
          ]
      },
      "properties": {
         "name": "Paul",
         "age": 50.8
      }
    },
    { "type": "Feature",
      "geometry": {
        "type": "Polygon",
        "coordinates": [
          [ [10.0, 10.0], [11.0, 10.0], [11.0, 11.0],
             [10.0, 11.0], [10.0, 10.0] ]
          ]
      },
      "properties": {
         "name": "Mary",
         "age": 50.8
      }
    }
  ]
}
```
