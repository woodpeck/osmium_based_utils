/*
  Osmium-based tool that counts the number of nodes/ways/relations
  in the input file.
*/

/*

Written 2013 by Frederik Ramm <frederik@remote.org>

Public Domain.

*/

#include <limits>
#include <iostream>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT

#include <osmium.hpp>

class CountHandler : public Osmium::Handler::Base {

    uint64_t nodes;
    uint64_t ways;
    uint64_t rels;

public:

    CountHandler() 
    {
        nodes = 0;
        ways = 0;
        rels = 0;
    }

    void node(__attribute__((__unused__)) const shared_ptr<Osmium::OSM::Node const>& node) 
    {
        nodes++;
    }

    void after_nodes() 
    {
        std::cerr << "nodes: " << nodes << "\n";
    }
    void way(__attribute__((__unused__)) const shared_ptr<Osmium::OSM::Way const>& way) 
    {
        ways++;
    }

    void after_ways() 
    {
        std::cerr << "ways: " << ways << "\n";
    }

    void relation(__attribute__((__unused__)) const shared_ptr<Osmium::OSM::Relation const>& relation) 
    {
        rels++;
    }

    void after_relations() 
    {
        std::cerr << "relations: " << rels << "\n";
    }

}; 


int main(int argc, char *argv[]) 
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " osmfile" << std::endl;
        exit(1);
    }
    Osmium::OSMFile infile(argv[1]);
    CountHandler handler;
    Osmium::Input::read(infile, handler);

    google::protobuf::ShutdownProtobufLibrary();
}

