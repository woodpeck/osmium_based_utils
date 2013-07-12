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
	uint64_t suche;
	uint64_t suche2;
	uint64_t suche3;

	std::vector<SearchTag *> searchTags;
	std::vector<std::string> usernames;
	std::vector<int> userid;

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
	}

	void setTypes(bool nodes, bool ways, bool rels)
	{
		process_nodes = nodes;
		process_ways = ways;
		process_rels = rels;
	}


	bool addOutputfile(const char *filename)
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

	bool addUserID(char *i)
	{
		int a=atoi(i);
		if(a==0) return false;
		userid.push_back(a);
		return true;
	}

	void addUserName(char *u)
	{
		usernames.push_back(u);
	}

	bool idmatch(int id)
	{
		if (userid.empty()) return true;
		for (unsigned int i=0; i < userid.size(); i++)
		{
			if	(userid[i]==id)
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
			if	(usernames[i]==user)
			{
				return true;
			}
		}
		return false;
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
		if (!idmatch(node->uid())) return;
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
		if (!idmatch(way->uid())) return;
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
		if (!idmatch(relation->uid())) return;
		if (outHandler) outHandler->relation(relation);
		rels++;
	}

	void after_relations()
	{
		if (process_rels)
		{
			std::cerr << "relations: " << rels << "\n";
		}
		std::cerr << "Ergebnis: " << nodes + ways + rels << std::endl;
	}

	void final(){if (outHandler) outHandler->final();
	}

private:

};

void print_help()
{
	std::cerr << "osmgrep [OPTIONS]  \n" \
			<< "\nOptions:\n" \
			<< "  -h, --help 		\t This help message\n" \
			<< "  -t, --type 		\t Please specify which type of keyword you are looking for like -trelation \n" \
			<< "  -i, --id 		\t Please specify the ID-Number\n" \
			<< "  -u, --user		\t Please specify user name\n" \
			<< "  -e, --exp 		\t Please specify the key and the value of the tag like amenity=school oder amenity=* \n" \
			<< "  -o, --output 		\t Please specify the filename of your output like output.osm.bz2\n" ;
}


int main(int argc, char *argv[])
{
	static struct option long_options[] =
	{
			{"help",        no_argument, 0, 'h'},
			{"type", required_argument, 0, 't'},
			{"id", required_argument, 0, 'i'},
			{"user",  required_argument, 0, 'u'},
			{"exp", required_argument, 0, 'e'},
			{"output", required_argument, 0, 'o'},
			{0, 0, 0, 0}
	};

	bool node=false;
	bool way=false;
	bool relation=false;
	CountHandler handler;

	while (true)
	{
		int c = getopt_long(argc, argv, "ht:i:u:e:o:", long_options, 0);
		if (c == -1) {
			break;
		}
		switch (c)
		{
		case 'h':
			print_help();
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
				print_help();
				exit(1);
			}
			break;
		case 'i':
			if (!handler.addUserID(optarg))
			{
				std::cerr << "-i flag requires a number for ID like -i23232\n"  << std::endl;
				print_help();
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
			if(!handler.addOutputfile(optarg))
			{
				std::cerr << "-o flag requires a suitable filename" << std::endl;
				exit(1);
			}
			break;
		default:
			print_help();
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
	if (remaining_args > 1) {
		print_help();
		exit(1);
	}
	else if (remaining_args == 1) {
		input =  argv[optind];
	}

	Osmium::OSMFile infile(input);
	Osmium::Input::read(infile, handler);
	google::protobuf::ShutdownProtobufLibrary();
}
