/*
 Osmium-based tool which to grep an OSM File with a faster C++-Code
 */

/*

 Written 2013 by Dietmar Sauer <Dietmar@geofabrik.de>

 Public Domain.

*/

#include <limits>
#include <iostream>
#include <getopt.h>
#include <vector>
#include <osmium/output/xml.hpp>
#include <osmium/output/pbf.hpp>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT
#define OSMIUM_WITH_PBF_OUTPUT

#include <osmium.hpp>

struct SearchTag
{
    std::string key;
    std::string value;
    SearchTag(const char *k, const char *v) { key.assign(k); value.assign(v); }
};

class CountHandler: public Osmium::Handler::Base
{

    uint64_t nodes;
    uint64_t ways;
    uint64_t rels;

    std::vector<SearchTag *> searchTags;
    std::vector<std::string> usernames;
    std::vector<uint32_t> userid;
    std::vector<uint64_t> objectid;

    uint64_t version;
    short int version_sign;

    Osmium::Output::Handler *outHandler;

    bool process_nodes;
    bool process_ways;
    bool process_rels;


public:

    CountHandler()
    {
        nodes = 0;
        ways = 0;
        rels = 0;
        outHandler = NULL;
        process_nodes = false;
        process_ways = false;
        process_rels = false;
        version = 0;
        version_sign = 0;
    }

    void setTypes(bool nodes, bool ways, bool rels)
    {
        process_nodes = nodes;
        process_ways = ways;
        process_rels = rels;
    }

    bool setOutputfile(const char *filename)
    {
        Osmium::OSMFile *outfile = new Osmium::OSMFile(filename);
        outHandler = new Osmium::Output::Handler(*outfile);
        try
        {
            outHandler = new Osmium::Output::Handler(*outfile);
        }
        catch (Osmium::OSMFile::IOError *e)
        {
            std::cerr << "IOError: " << e << std::endl;
            return false;
        }
        return true;
    }

    bool addSearchTag(char *t)
    {
        char *equal = strchr(t, '=');
        if (!equal) return false;
        *equal++ = 0;
        if (!strlen(t)) return false;
        if (!strlen(equal)) return false;
        searchTags.push_back(new SearchTag(t, equal));
        return true;
    }

    bool addUserID(char *i)
    {
        int a=atoi(i);
        if(a==0) return false;
        userid.push_back(a);
        return true;
    }

    bool addObjectID(char *i)
    {
        int a=atoi(i);
        if(a==0) return false;
        objectid.push_back(a);
        return true;
    }

    bool setVersion(char *v)
    {
        if (*v == '+' || *v == '-')
        {
            version_sign = (*v == '-') ? -1 : 1;
            v++;
        }
        version = atoi(v);
        return (version > 0);
    }

    void addUserName(char *u)
    {
        usernames.push_back(u);
    }

    void init(Osmium::OSM::Meta& meta)
    {
        if (outHandler) outHandler->init(meta);
    }

    void node(const shared_ptr<Osmium::OSM::Node >& node)
    {
        if (!process_nodes) return;
        if (!tagmatch(node->tags())) return;
        if (!usermatch(node->user())) return;
        if (!uidmatch(node->uid())) return;
        if (!oidmatch(node->id())) return;
        if (!versionmatch(node->version())) return;
        if (outHandler) outHandler->node(node);
        nodes++;
    }

    void after_nodes()
    {
        if (process_nodes)
        {
        std::cerr << "nodes: "<< nodes << std::endl;
        }
    }

    void way(const shared_ptr<Osmium::OSM::Way >& way)
    {
        if (!process_ways) return;
        if (!tagmatch(way->tags())) return;
        if (!usermatch(way->user())) return;
        if (!uidmatch(way->uid())) return;
        if (!oidmatch(way->id())) return;
        if (!versionmatch(way->version())) return;
        if (outHandler) outHandler->way(way);
        ways++;
    }

    void after_ways()
    {
        if (process_ways)
        {
        std::cerr << "ways: " << ways << "\n";
        }
    }

    void relation(const shared_ptr<Osmium::OSM::Relation >& relation)
    {
        if (!process_rels) return;
        if (!tagmatch(relation->tags())) return;
        if (!usermatch(relation->user())) return;
        if (!uidmatch(relation->uid())) return;
        if (!oidmatch(relation->id())) return;
        if (!versionmatch(relation->version())) return;
        if (outHandler) outHandler->relation(relation);
        rels++;
    }

    void after_relations()
    {
        if (process_rels)
        {
            std::cerr << "relations: " << rels << "\n";
        }
        std::cerr << "total: " << nodes + ways + rels << std::endl;
    }

    void final()
    {
        if (outHandler) outHandler->final();
    }

private:

    bool uidmatch(uint32_t id)
    {
        if (userid.empty()) return true;
        for (unsigned int i=0; i < userid.size(); i++)
        {
            if (userid[i]==id)
            {
                return true;
            }
        }
        return false;
    }

    bool oidmatch(uint64_t id)
    {
        if (objectid.empty()) return true;
        for (unsigned int i=0; i < objectid.size(); i++)
        {
            if (objectid[i]==id)
            {
                return true;
            }
        }
        return false;
    }

    bool usermatch(const char* user)
    {
        if (usernames.empty()) return true;
        for (unsigned int i=0; i < usernames.size(); i++)
        {
            if    (usernames[i]==user)
            {
                return true;
            }
        }
        return false;
    }

    bool tagmatch(const Osmium::OSM::TagList& tags)
    {
        if (searchTags.empty()) return true;
        for (unsigned int i=0; i < searchTags.size(); i++)
        {
            const char *tagvalue = tags.get_value_by_key(searchTags[i]->key.c_str());
            if (tagvalue)
            {
                if (searchTags[i]->value == "*")
                {
                    return true;
                }
                else if (searchTags[i]->value == tagvalue)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool versionmatch(uint64_t v)
    {
        if (version == 0) return true;
        if (version_sign == -1 && v < version) return true;
        if (version_sign == 1 && v > version) return true;
        return (v==version);
    }

};

void print_help(const char *progname)
{
    std::cerr << "\n" << progname << " [OPTIONS] <inputfile> \n" 
            << "\nFinds and counts (optionally outputs) matching objects in an OSM file.\n"
            << "\nOptions:\n" 
            << "  --help        this help message\n" 
            << "  --type <t>    match objects of type t (node, way, relation)\n"
            << "  --oid <i>     match object ID i\n"
            << "  --uid <i>     match user ID i\n"
            << "  --version <i> match verison i (+i = larger than i, -i = smaller than i)\n"
            << "  --user <u>    match user name u\n"
            << "  --expr <e>    match objects with given tag (e can be type=value or type=*)\n"
            << "  --output <o>  write output to file o (without, just displays counts)\n"
            << "\nIf multiple selectors are given, objects have to match all conditions.\n" 
            << "Multiple occurrences of same selector mean the object has to match one of them.\n"
            << "Example: --type node --expr amenity=restaurant --expr tourism=hotem will match\n"
            << "only nodes that have at least one of the given tags.\n\n";
}


int main(int argc, char *argv[])
{
    static struct option long_options[] =
    {
        {"help", no_argument, 0, 'h'},
        {"type", required_argument, 0, 't'},
        {"oid", required_argument, 0, 'd'},
        {"version", required_argument, 0, 'v'},
        {"uid", required_argument, 0, 'i'},
        {"user",  required_argument, 0, 'u'},
        {"expr", required_argument, 0, 'e'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    bool node=false;
    bool way=false;
    bool relation=false;
    CountHandler handler;

    while (true)
    {
        int c = getopt_long(argc, argv, "ht:d:i:u:e:o:v:", long_options, 0);
        if (c == -1) 
        {
            break;
        }

        switch (c)
        {
        case 'h':
            print_help(argv[0]);
            exit(0);
        case 't':
            if (!strcmp(optarg, "node"))
            {
                node=true;
            }
            else if (!strcmp(optarg, "way"))
            {
                way=true;
            }
            else if (!strcmp(optarg, "relation"))
            {
                relation=true;
            }
            else
            {
                std::cerr << "-t flag requires a type node like -tnode for node -tway for way ...\n" << std::endl;
                print_help(argv[0]);
                exit(1);
            }
            break;
        case 'i':
            if (!handler.addUserID(optarg))
            {
                std::cerr << "--uid flag requires a number for ID like --uid23232\n"  << std::endl;
                print_help(argv[0]);
                exit(1);
            }
            break;
        case 'v':
            if (!handler.setVersion(optarg))
            {
                std::cerr << "--version flag requires a number: --version [+-)]5\n"  << std::endl;
                print_help(argv[0]);
                exit(1);
            }
            break;
        case 'd':
            if (!handler.addObjectID(optarg))
            {
                std::cerr << "--oid flag requires a number for ID like --oid 23232\n"  << std::endl;
                print_help(argv[0]);
                exit(1);
            }
            break;
        case 'u':
            handler.addUserName(optarg);
            break;
        case 'e':
            if (!handler.addSearchTag(optarg))
            {
                std::cerr << "-e flag requires key=value style argument" << std::endl;
                exit(1);
            }
            break;
        case 'o':
            if(!handler.setOutputfile(optarg))
            {
                std::cerr << "-o flag requires a suitable filename" << std::endl;
                exit(1);
            }
            break;
        default:
            print_help(argv[0]);
            exit(1);
        }
    }

    if (node||way||relation)
    {
        handler.setTypes(node,way,relation);
    }
    else
    {
        handler.setTypes(true,true,true);
    }

    std::string input;
    int remaining_args = argc - optind;
    if (remaining_args > 1) 
    {
        std::cerr << "extra arguments on command line" << std::endl;
        print_help(argv[0]);
        exit(1);
    }
    else if (remaining_args == 1) {
        input = argv[optind];
    }

    Osmium::OSMFile infile(input);
    Osmium::Input::read(infile, handler);
    google::protobuf::ShutdownProtobufLibrary();
}
