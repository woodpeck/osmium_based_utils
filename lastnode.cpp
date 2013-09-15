/*

Osmium-based tool that finds the last node ID in the input file.

Written 2013 by Frederik Ramm <frederik@remote.org>

Public Domain.

*/

#include <iostream>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT

#include <osmium.hpp>

class LastIdHandler : public Osmium::Handler::Base {

osm_object_id_t lastid;

public:

    LastIdHandler()
    {
        lastid = 0;
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node)
    {
        lastid = node->id();
    }

    void after_nodes() 
    {
        std::cout << lastid << std::endl;
        throw Osmium::Handler::StopReading();
    }
};

int main(int argc, char* argv[]) 
{
    if (argc != 2)
    {
        std::cerr << "usage: lastnode inputfile.pbf" << std::endl;
        exit(1);
    }

    Osmium::OSMFile infile(argv[1]);
    LastIdHandler handler;
    Osmium::Input::read(infile, handler);
}
