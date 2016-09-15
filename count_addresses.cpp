/*
  osmium-based tool that counts the number of addresses
  (house numbers) in the input file.
*/

/*

Written 2012 by Frederik Ramm <frederik@remote.org>
Ported 2016 to libosmium 2.9 by Philip Beelmann <beelmann@geofabik.de>

Public Domain.

*/

#include <iostream>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT


#include <getopt.h>

#include <osmium/osm.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

#include <map>

class AddressCountHandler : public osmium::handler::Handler
{

private:
    std::map<osmium::unsigned_object_id_type, uint16_t> housenumberMap;
    size_t numbers_nodes_overall = 0;
    size_t numbers_nodes_withstreet = 0;
    size_t numbers_nodes_withcity = 0;
    size_t numbers_nodes_withcountry = 0;
    size_t numbers_nodes_withpostcode = 0;
    size_t numbers_ways_overall = 0;
    size_t numbers_ways_withstreet = 0;
    size_t numbers_ways_withcity = 0;
    size_t numbers_ways_withcountry = 0;
    size_t numbers_ways_withpostcode = 0;
    size_t postcode_boundaries = 0;
    size_t interpolation_count = 0;
    size_t interpolation_error = 0;
    size_t numbers_through_interpolation = 0;
    std::map<std::string, bool> postcode;
    bool debug;


public:

    AddressCountHandler(bool debug) : debug(debug){
    }

    void node(const osmium::Node& node)
    {
        const char *hno = node.tags().get_value_by_key("addr:housenumber");
        if (hno)
        {
            housenumberMap[node.id()] = atoi(hno);
            numbers_nodes_overall ++;
            if (node.tags().get_value_by_key("addr:street")) numbers_nodes_withstreet ++;
            if (node.tags().get_value_by_key("addr:city")) numbers_nodes_withcity ++;
            if (node.tags().get_value_by_key("addr:country")) numbers_nodes_withcountry ++;
            if (node.tags().get_value_by_key("addr:postcode"))
            {
                numbers_nodes_withpostcode ++;
                postcode[node.tags().get_value_by_key("addr:postcode")] = true;
            }
        }
    }

    void way(const osmium::Way& way)
    {
        const char* inter = way.tags().get_value_by_key("addr:interpolation");
        if (inter)
        {
            interpolation_count ++;
            osmium::unsigned_object_id_type fromnode = way.nodes().front().ref();
            osmium::unsigned_object_id_type tonode = way.nodes().back().ref();
            uint16_t fromhouse = housenumberMap[fromnode];
            uint16_t tohouse = housenumberMap[tonode];

            // back out if we don't have both house numbers
            if (!(fromhouse && tohouse)) 
            {
                interpolation_error++;
                if (debug)
                {

                    if (!fromhouse) std::cerr << "interpolation way " << way.id() << " references node " << fromnode << " which has no addr:housenumber" << std::endl;
                    if (!tohouse) std::cerr << "interpolation way " << way.id() << " references node " << tonode << " which has no addr:housenumber" << std::endl;
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
                        if (fromhouse%2==1) std::cerr << "interpolation way " << way.id() << " (addr:interpolation=even) references node " << fromnode << " which has an odd house number of " << fromhouse << std::endl;
                        if (tohouse%2==1) std::cerr << "interpolation way " << way.id() << " (addr:interpolation=even) references node " << tonode << " which has an odd house number of " << tohouse << std::endl;
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
                        if (fromhouse%2==1) std::cerr << "interpolation way " << way.id() << " (addr:interpolation=odd) references node " << fromnode << " which has an even house number of " << fromhouse << std::endl;
                        if (tohouse%2==1) std::cerr << "interpolation way " << way.id() << " (addr:interpolation=odd) references node " << tonode << " which has an even house number of " << tohouse << std::endl;
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
                    std::cerr << "interpolation way " << way.id() << " has invalid interpolation mode '" << inter << "'" << std::endl;
                }
                interpolation_error++;
            }
            // slight error here - if interpolation way contains intermediate nodes with addresses then these are counted twice.
        }
        else
        {
            const char *hno = way.tags().get_value_by_key("addr:housenumber");
            if (hno)
            {
                numbers_ways_overall ++;
                if (way.tags().get_value_by_key("addr:street")) numbers_ways_withstreet ++;
                if (way.tags().get_value_by_key("addr:city")) numbers_ways_withcity ++;
                if (way.tags().get_value_by_key("addr:country")) numbers_ways_withcountry ++;
                if (way.tags().get_value_by_key("addr:postcode"))
                {
                    numbers_ways_withpostcode ++;
                    postcode[way.tags().get_value_by_key("addr:postcode")] = true;
                }
            }
            else
            {
                const char *bdy = way.tags().get_value_by_key("boundary");
                if (bdy && !strcmp(bdy, "postal_code")) 
                {
                    postcode_boundaries++;
                    const char *ref = way.tags().get_value_by_key("ref");
                    if (ref) postcode[ref]=true;
                }
            }
        }
    }

    void relation(const osmium::Relation& rel)
    {
        const char *bdy = rel.tags().get_value_by_key("boundary");
        if (bdy && !strcmp(bdy, "postal_code")) 
        {
            postcode_boundaries++;
            const char *ref = rel.tags().get_value_by_key("ref");
            if (ref) postcode[ref]=true;
        }
    }

    void print() {
        std::cout << "                      nodes      ways      total" << std::endl;
        std::cout << "with house number   " << numbers_ways_overall         << "   " << numbers_nodes_overall       << "   " << numbers_ways_overall                                    << std::endl;
        std::cout << "... and street      " << numbers_nodes_withstreet     << "   " << numbers_ways_withstreet     << "   " << numbers_nodes_withstreet + numbers_ways_withstreet      << std::endl;
        std::cout << "... and city        " << numbers_nodes_withcity       << "   " << numbers_ways_withcity       << "   " << numbers_nodes_withcity + numbers_ways_withcity          << std::endl;
        std::cout << "... and post code   " << numbers_nodes_withpostcode   << "   " << numbers_ways_withpostcode   << "   " << numbers_nodes_withpostcode + numbers_ways_withcountry   << std::endl;
        std::cout << "... and country     " << numbers_nodes_withcountry    << "   " << numbers_ways_withcountry    << "   " << numbers_nodes_withcountry + numbers_ways_withcountry    << std::endl;
        std::cout << "\ntotal interpolations: " << interpolation_count << " (" << interpolation_error << " ignored)" << std::endl;
        std::cout << "\nhouse numbers added through interpolation: " << numbers_through_interpolation << std::endl;
        std::cout << "\ngrand total (interpolation, ways, nodes): " << numbers_through_interpolation + numbers_nodes_overall + numbers_ways_overall << std::endl;
        std::cout << "\nnumber of different post codes: " << postcode.size() << std::endl;
        std::cout << "\nnumber of post code boundaries: " << postcode_boundaries << std::endl;
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

    std::string input;
    int remaining_args = argc - optind;
    if (remaining_args == 1) {
        input = argv[optind];
    } else {
        usage(argv[0]);
        exit(1);
    }


    osmium::io::File infile(input);
    osmium::io::Reader reader(infile);

    AddressCountHandler handler(debug);
    osmium::apply(reader, handler);
    reader.close();
    handler.print();
}

