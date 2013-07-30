/*

  Add a synthesized timestamp tag to all objects in the output file.

  This makes little sense *except* if you are using some kind of 
  OSM file processor that cannot use the timestamp attribute; 
  using this program you can transform the timestamp attributo
  into a proper tag which can then be used.

  Written by Frederik Ramm <frederik@remote.org>, public domain.

*/

#include <iostream>
#include <getopt.h>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT
#include <osmium.hpp>
#include <osmium/output/xml.hpp>
#include <osmium/output/pbf.hpp>

void print_help()
{
    std::cout << "add_timestamp <inputfile> <outputfile>\n\nadds a synthesized timestamp tag to all OSM objects.\n";
}

class AddTimestampHandler : public Osmium::Output::Handler
{
    public: 

    AddTimestampHandler(Osmium::OSMFile& outfile) : Osmium::Output::Handler(outfile) 
    {
    }

    void node(const shared_ptr<Osmium::OSM::Node >& node) 
    { 
        node->tags().add("timestamp", node->timestamp_as_string().c_str());
        Osmium::Output::Handler::node(node);
    }
    void way(const shared_ptr<Osmium::OSM::Way >& way)
    { 
        way->tags().add("timestamp", way->timestamp_as_string().c_str());
        Osmium::Output::Handler::way(way);
    }
    void relation(const shared_ptr<Osmium::OSM::Relation >& relation)
    { 
        relation->tags().add("timestamp", relation->timestamp_as_string().c_str());
        Osmium::Output::Handler::relation(relation);
    }
};


int main(int argc, char* argv[]) 
{
    static struct option long_options[] = 
    {
        {"help",        no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    std::string input_format;
    std::string output_format;

    while (true) 
    {
        int c = getopt_long(argc, argv, "h", long_options, 0);
        if (c == -1) 
        {
            break;
        }

        switch (c) 
        {
            case 'h':
                print_help();
                exit(0);
            default:
                exit(1);
        }
    }

    std::string input;
    std::string output;
    int remaining_args = argc - optind;
    if (remaining_args != 2) 
    {
        print_help();
        exit(1);
    }

    input =  argv[optind];
    output = argv[optind+1];

    Osmium::OSMFile infile(input);
    Osmium::OSMFile outfile(output);

    AddTimestampHandler ats(outfile);
    ats.set_generator("add_timestamp");

    Osmium::Input::read(infile, ats);
}

