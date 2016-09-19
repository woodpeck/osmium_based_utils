/*
 osmium-based tool which to grep an OSM File with a faster C++-Code
 */

/*

 Written 2013 by Dietmar Sauer <Dietmar@geofabrik.de>
 Ported 2016 to libosmium 2.9 by Philip Beelmann <beelmann@geofabik.de>

 Public Domain.

*/
#include <string>

#include <limits>
#include <iostream>
#include <getopt.h>
#include <vector>
#include <algorithm> // for std::copy_if
#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/handler/dump.hpp>

// Get access to isatty utility function and progress bar utility class.
#include <osmium/util/file.hpp>
#include <osmium/util/progress_bar.hpp>

#include <osmium/io/input_iterator.hpp>
#include <osmium/io/output_iterator.hpp>

void print_help(const char *progname)
{
    std::cerr << "\n" << progname << " [OPTIONS] <inputfile> \n"
            << "\nFinds and counts (optionally outputs) matching objects in an OSM file.\n"
            << "\nOptions:\n"
            << "  --help          this help message\n"
            << "  --type <t>      match objects of type t (node, way, relation)\n"
            << "  --oid <i>       match object ID i\n"
            << "  --uid <i>       match user ID i\n"
            << "  --version <i>   match verison i (+i = larger than i, -i = smaller than i)\n"
            << "  --user <u>      match user name u\n"
            << "  --expr <e>      match objects with given tag (e can be type=value or type=*)\n"
            << "  --output <o>    write output to file o (without, just displays counts)\n"
            << "  --progress <p>  shows progress bar\n"
            << "\nIf multiple selectors are given, objects have to match all conditions.\n"
            << "Multiple occurrences of same selector mean the object has to match one of them.\n"
            << "Example: --type node --expr amenity=restaurant --expr tourism=hotem will match\n"
            << "only nodes that have at least one of the given tags.\n\n";
}



int main(int argc, char* argv[])
{
    uint64_t node_count = 0;
    uint64_t way_count = 0;
    uint64_t relation_count = 0;

    bool node = false;
    bool way = false;
    bool relation = false;

    bool enable_progress_bar = false;

    osmium::user_id_type user_id = 0;
    osmium::object_version_type version = 0;
    osmium::object_id_type object_id = 0;
    const char* user_name = nullptr;
    const char* search_key = nullptr;
    const char* search_value = nullptr;
    const char* output_file = nullptr;

    static struct option long_options[] = {
            { "help", no_argument, 0, 'h' },
            { "type", required_argument, 0, 't' },
            { "oid", required_argument, 0, 'd' },
            { "version", required_argument, 0, 'v' },
            { "uid", required_argument, 0, 'i' },
            { "user", required_argument, 0, 'u' },
            { "expr", required_argument, 0, 'e' },
            { "output", required_argument, 0, 'o' },
            { "progress", no_argument, 0, 'p' },
            { 0, 0, 0, 0 } };
    while (true) {
        int c = getopt_long(argc, argv, "ht:d:i:u:e:o:v:p", long_options, 0);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'h':
            print_help(argv[0]);
            exit(0);
        case 't':
            if (!strcmp(optarg, "node")) {
                node = true;
            } else if (!strcmp(optarg, "way")) {
                way = true;
            } else if (!strcmp(optarg, "relation")) {
                relation = true;
            } else {
                std::cerr << "-t flag requires a type node like -tnode for node -tway for way ...\n" << std::endl;
                print_help(argv[0]);
                exit(1);
            }

            break;
        case 'i':
            if (optarg) {
                user_id = strtol(optarg, NULL, 0);
            } else {
                std::cerr << "--uid flag requires a number for ID like --uid23232\n" << std::endl;
                print_help(argv[0]);
                exit(1);
            }
            break;
        case 'v':
            if (optarg) {
                version = strtol(optarg, NULL, 0);
            } else {
                std::cerr << "--version flag requires a number: --version [+-)]5\n" << std::endl;
                print_help(argv[0]);
                exit(1);
            }
            break;
        case 'd':
            if (optarg) {
                object_id = strtol(optarg, NULL, 0);
            } else {
                std::cerr << "--oid flag requires a number for ID like --oid 23232\n" << std::endl;
                print_help(argv[0]);
                exit(1);
            }
            break;
        case 'u':
            if (optarg) {
                user_name = optarg;
            } else {
                std::cerr << "--user flag requires a string like --user foobar\n" << std::endl;
                print_help(argv[0]);
                exit(1);
            }
            break;
        case 'e':
            if (optarg) {
                std::string tag(optarg);
                std::size_t delim = tag.find("=");
                if (delim != std::string::npos && tag.find("=", delim + 1) != std::string::npos) {
                    std::cerr << "-e flag requires key=value style argument" << std::endl;
                    exit(1);
                }
                search_key = strdup(tag.substr(0, delim).c_str());
                if (delim != std::string::npos) {
                    search_value = strdup(tag.substr(delim + 1).c_str());
                }
            } else {
                std::cerr << "-e flag requires key=value style argument" << std::endl;
                exit(1);
            }
            break;
        case 'o':
            if (optarg) {
                output_file = optarg;
            } else {
                std::cerr << "-o flag requires a suitable filename" << std::endl;
                exit(1);
            }
            break;
        case 'p':
            enable_progress_bar = true;
            break;
        default:
            print_help(argv[0]);
            exit(1);
        }
    }
    std::string input;
    int remaining_args = argc - optind;
    if (remaining_args > 1) {
        std::cerr << "extra arguments on command line" << std::endl;
        print_help(argv[0]);
        exit(1);
    } else if (remaining_args == 1) {
        input = argv[optind];
    } else {
        std::cerr << "Usage:     " << argv[0] << " OSMFILE OPTIONS" << std::endl;
        std::cerr << "           " << argv[0] << " --help" << std::endl;
        exit(1);
    }

    // The input file, deduce file format from file suffix
    osmium::io::File infile{input};


    osmium::osm_entity_bits::type entities = osmium::osm_entity_bits::nothing;
    if (node)       entities |= osmium::osm_entity_bits::node;
    if (way)        entities |= osmium::osm_entity_bits::way;
    if (relation)   entities |= osmium::osm_entity_bits::relation;

    if(entities == osmium::osm_entity_bits::nothing)
        entities = osmium::osm_entity_bits::all;

    // Initialize Reader for the input file.
    // Read only changesets (will ignore nodes, ways, and
    // relations if there are any).
    osmium::io::Reader reader{infile, entities};

    // Initialize progress bar, enable it only if STDERR is a TTY.
    osmium::ProgressBar progress{reader.file_size(), osmium::util::isatty(2) && enable_progress_bar};

    // Get the header from the input file
    osmium::io::Header header = reader.header();

    header.set("generator", "osmgrep");

    // Create range of input iterators that will iterator over all objects
    // delivered from input file through the "reader".
    auto input_range = osmium::io::make_input_iterator_range<osmium::OSMObject>(reader);


    auto condition = [&](const osmium::OSMObject& object) {
        progress.update(reader.offset());

        if(user_id && object.uid() != user_id) return false;
        if(version && object.version() != version) return false;
        if(object_id && object.id() != object_id) return false;
        if(user_name && strcmp(object.user(), user_name)) return false;
        if(search_key) {
            const char* value = object.tags()[search_key];
            if (!value) return false;
            if (search_value){
                if(strcmp(search_value, value) && strcmp(search_value, "*")) return false;
            }
        }
        if (!output_file){
            switch (object.type()) {
                case osmium::item_type::node:
                node_count++;
                break;
                case osmium::item_type::way:
                way_count++;
                break;
                case osmium::item_type::relation:
                relation_count++;
                break;
                default:
                break;
            }
        }
        return true;
    };

    if (output_file) {
        osmium::io::File outfile { output_file };

        // Initialize writer for the output file. Use the header from the input
        // file for the output file. This will copy over some header information.
        // The last parameter will tell the writer that it is allowed to overwrite
        // an existing file. Without it, it will refuse to do so.
        osmium::io::Writer writer(outfile, header, osmium::io::overwrite::allow);

        // Create an output iterator writing through the "writer" object to the
        // output file.
        auto output_iterator = osmium::io::make_output_iterator(writer);

        // Copy all objects from input to output that fit criteria
        std::copy_if(input_range.begin(), input_range.end(), output_iterator, condition);


        // Explicitly close the writer and reader. Will throw an exception if
        // there is a problem. If you wait for the destructor to close the writer
        // and reader, you will not notice the problem, because destructors must
        // not throw.
        writer.close();
    } else {
        std::for_each(input_range.begin(), input_range.end(), condition);
        std::cout << std::endl;
        std::cout << " #nodes matching      = " << node_count << std::endl;
        std::cout << " #ways matching       = " << way_count << std::endl;
        std::cout << " #relations matching  = " << relation_count << std::endl;

    }
    // Progress bar is done.
    progress.done();
    reader.close();

}
