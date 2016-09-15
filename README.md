# A collection of osmium-based utilities for doing things with OSM data files.


## Note:

The osmcode/osmtool repo has a command line utility that can be used to replace some of these programs:

* As a lastnode.cpp replacement, use `osmium fileinfo x.osm.pbf -eg data.maxid.nodes`.
  For more information call `osmium help fileinfo`.

## Updated
* osmgrep.cpp
* osmstats.cpp
* count_addresses.cpp


## Replacements

### count.cpp

    $ osmium fileinfo -e karlsruhe-regbez.osm.pbf | grep Number | grep -v changesets
      Number of nodes: 8291451
      Number of ways: 1424970
      Number of relations: 26632

---
### Dropped

* noderef.cpp
