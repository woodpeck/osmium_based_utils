/*
  Osmium-based tool that counts the number of addresses
  (house numbers) in the input file.
*/

/*

Written 2012 by Frederik Ramm <frederik@remote.org>

Public Domain.

*/

#include <iostream>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT


#include <getopt.h>
#include <osmium.hpp>
#include <osmium/storage/byid/sparse_table.hpp>
#include <osmium/storage/byid/mmap_file.hpp>
#include <osmium/handler/coordinates_for_ways.hpp>
#include <osmium/geometry/point.hpp>
#include <osmium/export/shapefile.hpp>

#include <map>

class AddressCountHandler : public Osmium::Handler::Base 
{

private:
    std::map<osm_object_id_t, uint16_t> housenumberMap;
    size_t numbers_overall;
    size_t numbers_withstreet;  
    size_t numbers_withcity;
    size_t numbers_withcountry;
    size_t interpolation_count;
    size_t interpolation_error;
    size_t numbers_through_interpolation;
    bool debug;


public:

    AddressCountHandler(bool dbg) 
    {
         numbers_overall = 0;
         numbers_withstreet = 0;  
         numbers_withcity = 0;
         numbers_withcountry = 0;
         interpolation_count = 0;
         interpolation_error = 0;
         numbers_through_interpolation = 0;
         debug = dbg;
    }

    ~AddressCountHandler() 
    {
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) 
    {
        const char *hno = node->tags().get_value_by_key("addr:housenumber");
        if (hno)
        {
            if(node->id()==643600355) std::cerr << "643600355" << std::endl;
            housenumberMap[node->id()] = atoi(hno);
            numbers_overall ++;
            if (node->tags().get_value_by_key("addr:street")) numbers_withstreet ++;
            if (node->tags().get_value_by_key("addr:city")) numbers_withcity ++;
            if (node->tags().get_value_by_key("addr:country")) numbers_withcountry ++;
        }
    }

    void way(const shared_ptr<Osmium::OSM::Way>& way) 
    {
        const char* inter = way->tags().get_value_by_key("addr:interpolation");
        if (!inter) return;
        interpolation_count ++;
        osm_object_id_t fromnode = way->get_first_node_id();
        osm_object_id_t tonode = way->get_last_node_id();
        uint16_t fromhouse = housenumberMap[fromnode];
        uint16_t tohouse = housenumberMap[tonode];

        // back out if we don't have both house numbers
        if (!(fromhouse && tohouse)) 
        {
            interpolation_error++;
            if (debug)
            {
                if (!fromhouse) std::cerr << "interpolation way " << way->id() << " references node " << fromnode << " which has no addr:housenumber" << std::endl;
                if (!tohouse) std::cerr << "interpolation way " << way->id() << " references node " << tonode << " which has no addr:housenumber" << std::endl;
            }
            return;
        }

        // swap if range is backwards
        if (tohouse < fromhouse) 
        {
            fromhouse = tohouse;
            tohouse = housenumberMap[fromnode];
        }

        if (!strcmp(inter, "even"))
        {
            if ((fromhouse %2 == 1) || (tohouse %2 == 1))
            {
                if (debug)
                {
                    if (fromhouse%2==1) std::cerr << "interpolation way " << way->id() << " (addr:interpolation=even) references node " << fromnode << " which has an odd house number of " << fromhouse << std::endl;
                    if (tohouse%2==1) std::cerr << "interpolation way " << way->id() << " (addr:interpolation=even) references node " << tonode << " which has an odd house number of " << tohouse << std::endl;
                }
                interpolation_error++;
                return;
            }
            numbers_through_interpolation += (tohouse - fromhouse) / 2 - 1;
        }
        else if (!strcmp(inter, "odd"))
        {
            if ((fromhouse %2 == 0) || (tohouse %2 == 0))
            {
                if (debug)
                {
                    if (fromhouse%2==1) std::cerr << "interpolation way " << way->id() << " (addr:interpolation=odd) references node " << fromnode << " which has an even house number of " << fromhouse << std::endl;
                    if (tohouse%2==1) std::cerr << "interpolation way " << way->id() << " (addr:interpolation=odd) references node " << tonode << " which has an even house number of " << tohouse << std::endl;
                }
                interpolation_error++;
                return;
            }
            numbers_through_interpolation += (tohouse - fromhouse) / 2 - 1;
        }
        else if (!strcmp(inter, "both") || !strcmp(inter, "all"))
        {
            numbers_through_interpolation += (tohouse - fromhouse) - 1;
        }
        else
        {
            if (debug)
            {
                std::cerr << "interpolation way " << way->id() << " has invalid interpolation mode '" << inter << "'" << std::endl;
            }
            interpolation_error++;
        }
        // slight error here - if interpolation way contains intermediate nodes with addresses then these are counted twice.
    }

    void after_ways() 
    {
        std::cout << "Nodes with house numbers: " <<  numbers_overall << std::endl;
        std::cout << "  thereof, with street:   " <<  numbers_withstreet << std::endl;
        std::cout << "  thereof, with city:     " <<  numbers_withcity << std::endl;
        std::cout << "  thereof, with country:  " <<  numbers_withcountry << std::endl;
        std::cout << "Number of interpolations: " <<  interpolation_count << " (" << interpolation_error << " ignored due to errors)" << std::endl;
        std::cout << "House numbers added through interpolation: " <<  numbers_through_interpolation << std::endl;

        throw Osmium::Handler::StopReading();
    }

};

/* ================================================== */

void usage(const char *prg)
{
    std::cerr << "Usage: " << prg << " [-d] [-h] OSMFILE" << std::endl;
}

int main(int argc, char* argv[]) 
{
    static struct option long_options[] = {
        {"debug",  no_argument, 0, 'd'},
        {"help",   no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    bool debug = false;

    while (true) 
    {
        int c = getopt_long(argc, argv, "dh", long_options, 0);
        if (c == -1) break;

        switch (c) 
        {
            case 'd':
                debug = true;
                break;
            case 'h':
                usage(argv[0]);
                exit(0);
            default:
                exit(1);
        }
    }

    if (argc != 2) 
    {
        usage(argv[0]);
        exit(1);
    }

    Osmium::OSMFile infile(argv[1]);
    AddressCountHandler handler(debug);
    Osmium::Input::read(infile, handler);
}

