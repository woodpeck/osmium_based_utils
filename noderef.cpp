/*
  Osmium-based tool that counts how often nodes are referenced.
*/

/*

Written 2013 by Frederik Ramm <frederik@remote.org>

Public Domain.

*/

#include <limits>
#include <iostream>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT

#define MAX_NODE_ID 4000000000

#include <osmium.hpp>

class RefHandler : public Osmium::Handler::Base {

    unsigned char *ref;
    uint64_t max_node_id;

public:

    RefHandler() 
    {
        ref = (unsigned char *) malloc(MAX_NODE_ID);
        memset(ref, 0, MAX_NODE_ID);
        max_node_id = 0;
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) 
    {
        max_node_id = node->id();
    }

    void way(const shared_ptr<Osmium::OSM::Way const>& way) 
    {
        for (Osmium::OSM::WayNodeList::const_iterator i = way->nodes().begin(); i!= way->nodes().end(); i++)
        {
            if (ref[i->ref()] < 255) ref[i->ref()]++;
        }
    }

    void after_ways() 
    {
        uint64_t count[256];
        for (int i=0; i<256; i++) count[i]=0;
        for (uint64_t i = 0; i <= max_node_id; i++)
        {
            count[ref[i]]++;
        }
        for (int i=0; i<256; i++) printf("%d,%ld\n", i, count[i]);
        throw Osmium::Handler::StopReading();
    }

    void relation(__attribute__((__unused__)) const shared_ptr<Osmium::OSM::Relation const>& relation) 
    {
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
    RefHandler handler;
    Osmium::Input::read(infile, handler);

    google::protobuf::ShutdownProtobufLibrary();
}

