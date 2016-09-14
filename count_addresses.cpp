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
    size_t numbers_nodes_overall;
    size_t numbers_nodes_withstreet;  
    size_t numbers_nodes_withcity;
    size_t numbers_nodes_withcountry;
    size_t numbers_nodes_withpostcode;
    size_t numbers_ways_overall;
    size_t numbers_ways_withstreet;  
    size_t numbers_ways_withcity;
    size_t numbers_ways_withcountry;
    size_t numbers_ways_withpostcode;
    size_t postcode_boundaries;
    size_t interpolation_count;
    size_t interpolation_error;
    size_t numbers_through_interpolation;
    std::map<std::string, bool> postcode;
    bool debug;


public:

    AddressCountHandler(bool dbg) 
    {
         numbers_nodes_overall = 0;
         numbers_nodes_withstreet = 0;  
         numbers_nodes_withcity = 0;
         numbers_nodes_withcountry = 0;
         numbers_nodes_withpostcode = 0;
         numbers_ways_overall = 0;
         numbers_ways_withstreet = 0;  
         numbers_ways_withcity = 0;
         numbers_ways_withcountry = 0;
         numbers_ways_withpostcode = 0;
         postcode_boundaries = 0;
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
            housenumberMap[node->id()] = atoi(hno);
            numbers_nodes_overall ++;
            if (node->tags().get_value_by_key("addr:street")) numbers_nodes_withstreet ++;
            if (node->tags().get_value_by_key("addr:city")) numbers_nodes_withcity ++;
            if (node->tags().get_value_by_key("addr:country")) numbers_nodes_withcountry ++;
            if (node->tags().get_value_by_key("addr:postcode")) 
            {
                numbers_nodes_withpostcode ++;
                postcode[node->tags().get_value_by_key("addr:postcode")] = true;
            }
        }
    }

    void way(const shared_ptr<Osmium::OSM::Way>& way) 
    {
        const char* inter = way->tags().get_value_by_key("addr:interpolation");
        if (inter)
        {
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
        else
        {
            const char *hno = way->tags().get_value_by_key("addr:housenumber");
            if (hno)
            {
                numbers_ways_overall ++;
                if (way->tags().get_value_by_key("addr:street")) numbers_ways_withstreet ++;
                if (way->tags().get_value_by_key("addr:city")) numbers_ways_withcity ++;
                if (way->tags().get_value_by_key("addr:country")) numbers_ways_withcountry ++;
                if (way->tags().get_value_by_key("addr:postcode")) 
                {
                    numbers_ways_withpostcode ++;
                    postcode[way->tags().get_value_by_key("addr:postcode")] = true;
                }
            }
            else
            {
                const char *bdy = way->tags().get_value_by_key("boundary");
                if (bdy && !strcmp(bdy, "postal_code")) 
                {
                    postcode_boundaries++;
                    const char *ref = way->tags().get_value_by_key("ref");
                    if (ref) postcode[ref]=true;
                }
            }
        }
    }

    void relation(const shared_ptr<Osmium::OSM::Relation>& rel) 
    {
        const char *bdy = rel->tags().get_value_by_key("boundary");
        if (bdy && !strcmp(bdy, "postal_code")) 
        {
            postcode_boundaries++;
            const char *ref = rel->tags().get_value_by_key("ref");
            if (ref) postcode[ref]=true;
        }
    }

    void after_ways() 
    {
        printf("                      nodes      ways      total\n");
        printf("with house number   %8ld   %8ld   %8ld\n", numbers_nodes_overall, numbers_ways_overall, numbers_nodes_overall + numbers_ways_overall);
        printf("... and street      %8ld   %8ld   %8ld\n", numbers_nodes_withstreet, numbers_ways_withstreet, numbers_nodes_withstreet + numbers_ways_withstreet);
        printf("... and city        %8ld   %8ld   %8ld\n", numbers_nodes_withcity, numbers_ways_withcity, numbers_nodes_withcity + numbers_ways_withcity);
        printf("... and post code   %8ld   %8ld   %8ld\n", numbers_nodes_withpostcode, numbers_ways_withpostcode, numbers_nodes_withpostcode + numbers_ways_withpostcode);
        printf("... and country     %8ld   %8ld   %8ld\n", numbers_nodes_withcountry, numbers_ways_withcountry, numbers_nodes_withcountry + numbers_ways_withcountry);
        printf("\ntotal interpolations: %ld (%ld ignored)\n", interpolation_count, interpolation_error);
        printf("\nhouse numbers added through interpolation: %ld\n", numbers_through_interpolation);
        printf("\ngrand total (interpolation, ways, nodes): %ld\n", numbers_through_interpolation + numbers_nodes_overall + numbers_ways_overall);
        printf("\nnumber of different post codes: %ld\n", postcode.size());
        printf("\nnumber of post code boundaries: %ld\n", postcode_boundaries);

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

