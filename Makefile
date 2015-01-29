#------------------------------------------------------------------------------
#
#  Makefile for Osmium examples.
#
#------------------------------------------------------------------------------

CXX = g++
#CXX = clang++

#CXXFLAGS = -g
CXXFLAGS = -O3 

CXXFLAGS += -Wall -Wextra -Wdisabled-optimization -pedantic -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wno-long-long

CXXFLAGS_GEOS    = $(shell geos-config --cflags)
CXXFLAGS_LIBXML2 = $(shell xml2-config --cflags)
CXXFLAGS_OGR     = $(shell gdal-config --cflags)

CXXFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CXXFLAGS += -I../include

# remove this if you do not want debugging to be compiled in
# CXXFLAGS += -DOSMIUM_WITH_DEBUG

LIB_EXPAT  = -lexpat
LIB_GD     = -lgd -lz -lm
LIB_GEOS   = $(shell geos-config --libs)
LIB_OGR    = $(shell gdal-config --libs)
LIB_PBF    = -lz -lpthread -lprotobuf-lite -losmpbf
LIB_SHAPE  = -lshp $(LIB_GEOS)
LIB_SQLITE = -lsqlite3
LIB_XML2   = $(shell xml2-config --libs)

LDFLAGS = $(LIB_EXPAT) $(LIB_PBF)

PROGRAMS = \
    noderef \
    count_addresses \
    count \
    osmgrep \
    grep-user-from-history \
    history-cleartext \
    add_timestamp \
    osmstats \
    lastnode

.PHONY: all clean

all: $(PROGRAMS)

count_addresses: count_addresses.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2)

noderef: noderef.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2)

count: count.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2)

osmgrep: osmgrep.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2)

grep-user-from-history: grep-user-from-history.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2)

history-cleartext: history-cleartext.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2)

add_timestamp: add_timestamp.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2)

osmstats: osmstats.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2) $(LIB_GEOS)

lastnode: lastnode.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $< $(LDFLAGS) $(LIB_XML2)

clean:
	rm -f *.o core $(PROGRAMS)

