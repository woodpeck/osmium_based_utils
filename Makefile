#------------------------------------------------------------------------------
#
#  Makefile for Osmium examples.
#
#------------------------------------------------------------------------------

CXX = g++
#CXX = clang++

#CXXFLAGS += -g
CXXFLAGS += -O3 

CXXFLAGS += -Wall -Wextra -Wdisabled-optimization -pedantic -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wno-long-long

CXXFLAGS += -std=c++11

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
LIB_PBF    = -lz -lpthread #-lprotobuf-lite -losmpbf
LIB_SHAPE  = -lshp $(LIB_GEOS)
LIB_SQLITE = -lsqlite3
LIB_XML2   = $(shell xml2-config --libs)
LIB_BZ2    = -lbz2

LDFLAGS = $(LIB_EXPAT) $(LIB_PBF)

PROGRAMS = \
    count_addresses \
    osmgrep \
    osmstats

.PHONY: all clean

all: $(PROGRAMS)

count_addresses: count_addresses.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIB_BZ2)

osmgrep: osmgrep.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIB_BZ2)

osmstats: osmstats.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIB_BZ2) $(LIB_GEOS)


clean:
	rm -f *.o core $(PROGRAMS)

