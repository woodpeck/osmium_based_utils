/*

Osmium-based program that displays a couple statistics about the 
date in the input file.

Frederik Ramm <frederik@remote.org>, public domain
Ported 2016 to libosmium 2.9 by Philip Beelmann <beelmann@geofabik.de>

Handling of areas is still missing.

*/

#include <iostream>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/dynamic_handler.hpp>

#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/geom/haversine.hpp>
#include <osmium/visitor.hpp>

#include <geos/geom/Geometry.h>
#include <geos/geom/LineString.h>

/* ================================================== */

class StatisticsHandler : public osmium::handler::Handler
{

private:

    double motorway_trunk_length = 0;
    double primary_secondary_length = 0;
    double other_road_length = 0;
    double residential_road_with_name_length = 0;
    double residential_road_length = 0;
    double path_length = 0;

    double river_length = 0;
    double railway_length = 0;
    double powerline_length = 0;
    double water_area = 0;
    double forest_area = 0;

    int building_count = 0;
    int housenumber_count = 0;
    int place_count = 0;

    int poi_power_count = 0;
    int poi_traffic_count = 0;
    int poi_other_count = 0;
    int poi_public_count = 0;
    int poi_hospitality_count = 0;
    int poi_shop_count = 0;
    int poi_religion_count = 0;

    int landuse_green_count = 0;
    int landuse_blue_count = 0;
    int landuse_zone_count = 0;
    int landuse_agri_count = 0;

public:

    void count_misc(const osmium::TagList& tags)
    {
        const char *t = tags.get_value_by_key("landuse");
        if (t)
        {
            if (!strcmp(t, "forest") || !strcmp(t, "grass") || !strcmp(t, "meadow"))
            {
                landuse_green_count++;
            }
            else if (!strcmp(t, "residential") || !strcmp(t, "industrial") || !strcmp(t, "commercial") || !strcmp(t, "military"))
            {
                landuse_zone_count++;
            }
            else if (!strcmp(t, "farmland") || !strcmp(t, "farm") || !strcmp(t, "farmyard"))
            {
                landuse_agri_count++;
            }
        }
        t = tags.get_value_by_key("amenity");
        if (t)
        {
            if (!strcmp(t, "restaurant") || !strcmp(t, "cafe") || !strcmp(t, "fast_food") || !strcmp(t, "pub") || !strcmp(t, "bar"))
            {
                poi_hospitality_count++;
            }
            else if (!strcmp(t, "fuel") || !strcmp(t, "parking"))
            {
                poi_traffic_count++;
            }
            else if (!strcmp(t, "place_of_worship"))
            {
                poi_religion_count++;
            }
            else if (!strcmp(t, "school") || !strcmp(t, "public_building") || !strcmp(t, "kindergarten") || !strcmp(t, "hospital") || !strcmp(t, "post_office"))
            {
                poi_public_count++;
            }
            else if (!strcmp(t, "atm") || !strcmp(t, "bank"))
            {
                poi_shop_count++;
            }
            else
            {
                poi_other_count++;
            }
        }
        if (tags.get_value_by_key("shop"))  poi_shop_count++;
        t = tags.get_value_by_key("tourism");
        if (t)
        {
            if (!strcmp(t, "hotel") || !strcmp(t, "motel") || !strcmp(t, "camp_site") || !strcmp(t, "hostel"))
            {
                poi_hospitality_count++;
            }
            else if (!strcmp(t, "museum"))
            {
                poi_public_count++;
            }
            else
            {
                poi_other_count++;
            }
        }
        t = tags.get_value_by_key("highway");
        if (t)
        {
            if (!strcmp(t, "bus_stop"))
            {
                poi_traffic_count++;
            }
        }
        t = tags.get_value_by_key("aeroway");
        if (t)
        {
            if (!strcmp(t, "aerodrome"))
            {
                poi_traffic_count++;
            }
        }
        t = tags.get_value_by_key("railway");
        if (t)
        {
            if (!strcmp(t, "station") || !strcmp(t, "halt"))
            {
                poi_traffic_count++;
            }
        }
        t = tags.get_value_by_key("power");
        if (t)
        {
            if (!strcmp(t, "station") || !strcmp(t, "generator") || !strcmp(t, "transformer"))
            {
                poi_power_count++;
            }
        }
        t = tags.get_value_by_key("natural");
        if (t)
        {
            if (!strcmp(t, "wood"))
            {
                landuse_green_count++;
            }
            else if (!strcmp(t, "water"))
            {
                landuse_blue_count++;
            }
        }
    }


    void area(const osmium::Area& area)
    {
        if (area.tags().get_value_by_key("building"))
        {
            building_count++;
        }
        if (area.tags().get_value_by_key("addr_housenumber"))
        {
            housenumber_count++;
        }
        count_misc(area.tags());
    }

    void way(const osmium::Way& way)
    {
        const char *hwy = way.tags().get_value_by_key("highway");
        if (hwy)
        {
            if (!strcmp(hwy, "motorway") || !strcmp(hwy, "trunk"))
            {
                motorway_trunk_length += waylen(way);
            }
            else if (!strcmp(hwy, "primary") || !strcmp(hwy, "secondary"))
            {
                primary_secondary_length += waylen(way);
            }
            else if (!strcmp(hwy, "tertiary") || !strcmp(hwy, "unclassified"))
            {
                other_road_length += waylen(way);
            }
            else if (!strcmp(hwy, "residential"))
            {
                double l = waylen(way);
                residential_road_length += l;
                if (way.tags().get_value_by_key("name")) residential_road_with_name_length += l;
            }
            else if (!strcmp(hwy, "service") || !strcmp(hwy, "path") || !strcmp(hwy, "footway") || !strcmp(hwy, "cycleway") || !strcmp(hwy, "track"))
            {
                path_length += waylen(way);
            }
            return; 
        }
        const char *wwy = way.tags().get_value_by_key("waterway");
        if (wwy)
        {
            if (!strcmp(wwy, "river"))
            {
                river_length += waylen(way);
            }
            return;
        }
        const char *rwy = way.tags().get_value_by_key("railway");
        if (rwy)
        {
            if (!strcmp(rwy, "rail") || !strcmp(rwy, "light_rail"))
            {
                railway_length += waylen(way);
            }
            return;
        }
        const char *pwr = way.tags().get_value_by_key("power");
        if (pwr)
        {
            if (!strcmp(pwr, "line") || !strcmp(pwr, "minor_line"))
            {
                powerline_length += waylen(way);
            }
            return;
        }
        count_misc(way.tags());
    }

    void node(const osmium::Node& node)
    {
        if (node.tags().get_value_by_key("place"))
        {
            if (node.tags().get_value_by_key("name")) place_count++;
        }
        else if (node.tags().get_value_by_key("addr:housenumber"))
        {
            housenumber_count++;
        }
        else 
        {
            count_misc(node.tags());
        }
    }

    void print()
    {
        bool csv = false;
        if (csv)
        {
            std::cout <<
                "motorways and trunk roads km,"
                "primary and secondary roads km,"
                "other connecting roads km,"
                "residential roads km,"
                "residential roads with names km,"
                "tracks/paths km,"
                "rivers km,"
                "railways km,"
                "power lines km,"
                "buildings,"
                "house numbers,"
                "named places,"
                "forest/meadow landcover count,"
                "water area landcover count,"
                "residential/industrial zone count,"
                "agricultural landuse count,"
                "POIs power,"
                "POIs transport,"
                "POIs public,"
                "POIs hospitality,"
                "POIs shop/bank,"
                "POIs religion,"
                "POIs other" << std::endl;

            std::cout <<
                (int) (motorway_trunk_length / 1000) << "," <<
                (int) (primary_secondary_length / 1000) << "," <<
                (int) (other_road_length / 1000) << "," <<
                (int) (residential_road_length / 1000) << "," <<
                (int) (residential_road_with_name_length / 1000) << "," <<
                (int) (path_length / 1000) << "," <<
                (int) (river_length / 1000) << "," <<
                (int) (railway_length / 1000) << "," <<
                (int) (powerline_length / 1000) << "," <<
                building_count << "," <<
                housenumber_count << "," <<
                place_count << "," <<
                landuse_green_count << "," <<
                landuse_blue_count << "," <<
                landuse_zone_count  << "," <<
                landuse_agri_count  << "," <<
                poi_power_count << "," <<
                poi_traffic_count << "," <<
                poi_public_count << "," <<
                poi_hospitality_count << "," <<
                poi_shop_count << "," <<
                poi_religion_count << "," <<
                poi_other_count << std::endl;
        }
        else
        {
            std::cout << "motorways and trunk roads km........"  <<  (int) (motorway_trunk_length / 1000)             << std::endl;
            std::cout << "primary and secondary roads km......"  <<  (int) (primary_secondary_length / 1000)          << std::endl;
            std::cout << "other connecting roads km..........."  <<  (int) (other_road_length / 1000)                 << std::endl;
            std::cout << "residential roads km................"  <<  (int) (residential_road_length / 1000)           << std::endl;
            std::cout << "residential roads with names km....."  <<  (int) (residential_road_with_name_length / 1000) << std::endl;
            std::cout << "tracks/paths km....................."  <<  (int) (path_length / 1000)                       << std::endl;
            std::cout << "rivers km..........................."  <<  (int) (river_length / 1000)                      << std::endl;
            std::cout << "railways km........................."  <<  (int) (railway_length / 1000)                    << std::endl;
            std::cout << "power lines km......................"  <<  (int) (powerline_length / 1000)                  << std::endl;
            std::cout << "buildings..........................."  <<  building_count                                   << std::endl;
            std::cout << "house numbers......................."  <<  housenumber_count                                << std::endl;
            std::cout << "named places........................"  <<  place_count                                      << std::endl;
            std::cout << "forest/meadow landcover count......."  <<  landuse_green_count                              << std::endl;
            std::cout << "water area landcover count.........."  <<  landuse_blue_count                               << std::endl;
            std::cout << "residential/industrial zone count..."  <<  landuse_zone_count                               << std::endl;
            std::cout << "agricultural landuse count.........."  <<  landuse_agri_count                               << std::endl;
            std::cout << "POIs power.........................."  <<  poi_power_count                                  << std::endl;
            std::cout << "POIs transport......................"  <<  poi_traffic_count                                << std::endl;
            std::cout << "POIs public........................."  <<  poi_public_count                                 << std::endl;
            std::cout << "POIs hospitality...................."  <<  poi_hospitality_count                            << std::endl;
            std::cout << "POIs shop/bank......................"  <<  poi_shop_count                                   << std::endl;
            std::cout << "POIs religion......................."  <<  poi_religion_count                               << std::endl;
            std::cout << "POIs other.........................."  <<  poi_other_count                                  << std::endl;
        }

    }


private:

double waylen(const osmium::Way& way)
{
    return osmium::geom::haversine::distance(way.nodes());
}

};

/* ================================================== */

// The type of index used. This must match the include file above
using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

// The location handler always depends on the index type
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

int main(int argc, char* argv[]) 
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE" << std::endl;
        exit(1);
    }

    StatisticsHandler stat_handler;

    // Initialize an empty DynamicHandler. Later it will be associated
    // with one of the handlers. You can think of the DynamicHandler as
    // a kind of "variant handler" or a "pointer handler" pointing to the
    // real handler.
    osmium::handler::DynamicHandler handler;

    osmium::io::File input_file{argv[optind]};

    // Configuration for the multipolygon assembler. Here the default settings
    // are used, but you could change multiple settings.
    osmium::area::Assembler::config_type assembler_config;

    // Initialize the MultipolygonCollector. Its job is to collect all
    // relations and member ways needed for each area. It then calls an
    // instance of the osmium::area::Assembler class (with the given config)
    // to actually assemble one area.
    osmium::area::MultipolygonCollector<osmium::area::Assembler> collector{assembler_config};

    // We read the input file twice. In the first pass, only relations are
    // read and fed into the multipolygon collector.
    osmium::io::Reader reader1{input_file, osmium::osm_entity_bits::relation};
    collector.read_relations(reader1);
    reader1.close();

    // The index storing all node locations.
    index_type index;

    // The handler that stores all node locations in the index and adds them
    // to the ways.
    location_handler_type location_handler{index};

    // If a location is not available in the index, we ignore it. It might
    // not be needed (if it is not part of a multipolygon relation), so why
    // create an error?
    location_handler.ignore_errors();

    // On the second pass we read all objects and run them first through the
    // node location handler and then the multipolygon collector. The collector
    // will put the areas it has created into the "buffer" which are then
    // fed through our "handler".
    osmium::io::Reader reader2{input_file};
    osmium::apply(reader2, location_handler, stat_handler, collector.handler([&stat_handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, stat_handler);
    }));
    reader2.close();

    stat_handler.print();
}

