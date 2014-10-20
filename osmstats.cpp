/*

Osmium-based program that displays a couple statistics about the 
date in the input file.

Frederik Ramm <frederik@remote.org>, public domain

Handling of areas is still missing.

*/

#include <iostream>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT

#include <osmium.hpp>
#include <osmium/storage/byid/sparse_table.hpp>
#include <osmium/storage/byid/mmap_file.hpp>
#include <osmium/handler/coordinates_for_ways.hpp>
#include <osmium/multipolygon/assembler.hpp>
#include <osmium/geometry/multipolygon.hpp>

/* ================================================== */

class StatisticsHandler : public Osmium::Handler::Base 
{

private:

    double motorway_trunk_length;
    double primary_secondary_length;
    double other_road_length;
    double residential_road_with_name_length;
    double residential_road_length;
    double path_length;

    double river_length;
    double railway_length;
    double powerline_length;
    double water_area;
    double forest_area;

    int building_count;
    int poi_count;
    int housenumber_count;
    int place_count;

    int poi_power_count;
    int poi_traffic_count;
    int poi_other_count;
    int poi_public_count;
    int poi_hospitality_count;
    int poi_shop_count;
    int poi_religion_count;

    int landuse_green_count;
    int landuse_blue_count;
    int landuse_zone_count;
    int landuse_agri_count;


public:

    StatisticsHandler() : Osmium::Handler::Base() 
    {
        motorway_trunk_length = 0;
        primary_secondary_length = 0;
        other_road_length = 0;
        residential_road_length = 0;
        residential_road_with_name_length = 0;
        path_length = 0;

        river_length = 0;
        railway_length = 0;
        powerline_length = 0;
        water_area = 0;
        forest_area = 0;

        building_count = 0;
        poi_count = 0;
        housenumber_count = 0;
        place_count = 0;

        poi_power_count = 0;
        poi_traffic_count = 0;
        poi_other_count = 0;
        poi_public_count = 0;
        poi_hospitality_count = 0;
        poi_shop_count = 0;
        poi_religion_count = 0;
        poi_traffic_count = 0;

        landuse_green_count = 0;
        landuse_blue_count = 0;
        landuse_zone_count = 0;
        landuse_agri_count = 0;
    }

    ~StatisticsHandler() {
    }

#define PI 3.14159265358979323846
#define rad(DEG) ((DEG)*((PI)/(180.0)))
    double lsarea(const geos::geom::LineString *ls)
    {
        const geos::geom::CoordinateSequence *cs = ls->getCoordinatesRO();
        double area = 0;
        size_t len = cs->getSize();
        if (len < 3) return 0;
        for (size_t i = 0; i < len-1; i++)
        {
            const geos::geom::Coordinate &p1 = cs->getAt(i);
            const geos::geom::Coordinate &p2 = cs->getAt(i+1);
            area += rad(p2.x - p1.x) *
                        (2 + sin(rad(p1.y)) +
                        sin(rad(p2.y)));
        }
        area = area * 6378137.0 * 6378137.0 / 2.0;
    }

    // unused, not properly working
    double comparea(const shared_ptr<Osmium::OSM::Area const>& area) 
    {
        double sum;
        const geos::geom::Geometry *a = Osmium::Geometry::MultiPolygon(*area).borrow_geos_geometry();
        if (a)
        {
            for (size_t i = 0; i < a->getNumGeometries(); i++)
            {
                const geos::geom::Polygon *p = dynamic_cast<const geos::geom::Polygon *>(a->getGeometryN(i));
                if (p)
                {
                    const geos::geom::LineString *er = p->getExteriorRing();
                    sum += lsarea(er);
                    for (size_t j = 0; j < p->getNumInteriorRing(); j++)
                    {
                        sum -= lsarea(p->getInteriorRingN(j));
                    }
                }
            }
        }
        else
        {
            std::cout << "none" << std::endl;
        }
        return 0.0;
    }

    void count_misc(Osmium::OSM::TagList tags)
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


    void area(const shared_ptr<Osmium::OSM::Area const>& area) 
    {
        if (area->tags().get_value_by_key("building"))
        {
            building_count++;
        }
        if (area->tags().get_value_by_key("addr_housenumber"))
        {
            housenumber_count++;
        }
        count_misc(area->tags());
    }

    void way(const shared_ptr<Osmium::OSM::Way const>& way) 
    {
        const char *hwy = way->tags().get_value_by_key("highway");
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
                if (way->tags().get_value_by_key("name")) residential_road_with_name_length += l;
            }
            else if (!strcmp(hwy, "service") || !strcmp(hwy, "path") || !strcmp(hwy, "footway") || !strcmp(hwy, "cycleway") || !strcmp(hwy, "track"))
            {
                path_length += waylen(way);
            }
            return; 
        }
        const char *wwy = way->tags().get_value_by_key("waterway");
        if (wwy)
        {
            if (!strcmp(wwy, "river"))
            {
                river_length += waylen(way);
            }
            return;
        }
        const char *rwy = way->tags().get_value_by_key("railway");
        if (rwy)
        {
            if (!strcmp(rwy, "rail") || !strcmp(rwy, "light_rail"))
            {
                railway_length += waylen(way);
            }
            return;
        }
        const char *pwr = way->tags().get_value_by_key("power");
        if (pwr)
        {
            if (!strcmp(pwr, "line") || !strcmp(pwr, "minor_line"))
            {
                powerline_length += waylen(way);
            }
            return;
        }
        count_misc(way->tags());
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) 
    {
        if (node->tags().get_value_by_key("place"))
        {
            if (node->tags().get_value_by_key("name")) place_count++;
        }
        else if (node->tags().get_value_by_key("addr:housenumber"))
        {
            housenumber_count++;
        }
        else 
        {
            count_misc(node->tags());
        }
    }

    void final()
    {
        bool csv = true;
        if (csv)
        {
            std::cout <<
                "motorways and trunk roads km,"
                "primary and secondary roads km,"
                "other connecting roads km,"
                "residential roads km,"
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
                poi_other_count << 
                std::endl;
        }
        else
        {
        printf("motorways and trunk roads ....... %5.0f km\n", motorway_trunk_length / 1000);
        printf("primary and secondary roads ..... %5.0f km\n", primary_secondary_length / 1000);
        printf("other connecting roads .......... %5.0f km\n", other_road_length / 1000);
        printf("residential roads ............... %5.0f km\n", residential_road_length / 1000);
        printf("   thereof, with names .......... %5.0f%%\n",  residential_road_with_name_length * 100 / residential_road_length);
        printf("tracks, service ways, paths ..... %5.0f km\n", path_length / 1000);
        printf("rivers .......................... %5.0f km\n", river_length / 1000);
        printf("railways ........................ %5.0f km\n", railway_length / 1000);
        printf("power lines ..................... %5.0f km\n", powerline_length / 1000);
        printf("\n");
        printf("buildings ....................... %5d\n", building_count);
        printf("house numbers ................... %5d\n", housenumber_count);
        printf("named places .................... %5d\n", place_count);
        printf("various POIs .................... %5d\n", poi_count);
        }
    }


private:

double waylen(const shared_ptr<Osmium::OSM::Way const>& way) 
{
    return Osmium::Geometry::Haversine::distance(way->nodes());
}
double wayarea(const shared_ptr<Osmium::OSM::Area const>& area) 
{
    /*
    double len = 0;
    double lastlat = 0;
    double lastlon = 0;
    bool init = true;
    for (Osmium::OSM::WayNodeList::const_iterator i = way->nodes().begin(); i!= way->nodes().end(); i++)
    {
        double thislon = i->lon();
        double thislat = i->lat();
        if (!init)
        {
            len += dist(lastlat, lastlon, thislat, thislon);
        }
        else
        {
            init = false;
        }
        lastlat = thislat;
        lastlon = thislon;
    }
    */
    return 0.0;
}
};

/* ================================================== */

typedef Osmium::Storage::ById::SparseTable<Osmium::OSM::Position> storage_sparsetable_t;
typedef Osmium::Storage::ById::MmapFile<Osmium::OSM::Position> storage_mmap_t;

int main(int argc, char* argv[]) 
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE" << std::endl;
        exit(1);
    }

    Osmium::OSMFile infile(argv[1]);

    storage_sparsetable_t store_pos;
    storage_mmap_t store_neg;

    StatisticsHandler stat_handler;

    typedef Osmium::MultiPolygon::Assembler<StatisticsHandler> assembler_t;
    assembler_t assembler(stat_handler, false);
    assembler.set_debug_level(1);

    typedef Osmium::Handler::CoordinatesForWays<storage_sparsetable_t, storage_mmap_t> cfw_handler_t;
    cfw_handler_t cfw_handler(store_pos, store_neg);

    typedef Osmium::Handler::Sequence<cfw_handler_t, assembler_t::HandlerPass2> sequence_handler_t;
    sequence_handler_t sequence_handler(cfw_handler, assembler.handler_pass2());

    std::cerr << "First pass...\n";
    Osmium::Input::read(infile, assembler.handler_pass1());

    std::cerr << "Second pass...\n";
    Osmium::Input::read(infile, sequence_handler);

    google::protobuf::ShutdownProtobufLibrary();
}

