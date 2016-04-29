# CRP
Open source C++ Implementation of Customizable Route Planning (CRP) by Delling et al. This project was part of a practical course at Karlsruhe Institute of Technology (KIT). 

Requirements
============

In order to build CRP you need to have the following software installed:

- Boost C++ Library (http://www.boost.org), more specifically Boost Iostreams.
- Scons (http://scons.org)
- g++ >= 4.8 (https://gcc.gnu.org)

Building CRP
============

If the Boost Library is not in your PATH, make sure to edit the *SConstruct* file in the root directory to point the build script to the correct location of Boost. There is a section *Libraries* in the *SConstruct* file where you can specify the paths. 

Once you have installed all the software packages listed above, you can build the CRP programs by typing
```
scons --target=CRP --optimize=Opt -jX
```
into your terminal where `X` is the number of cores you want to use for building the project. If you want to use a specific g++ compiler version you can add `--compiler=g++-Version`. We also support a debug and profiling build that you can call with `--optimize=Dbg` and `--optimize=Pro` respectively. 

This command will build three programs in the folder *deploy*:

- *osmparser*: Used to parse an OpenStreetMap (OSM) bz2-compressed map file. Call it with `./deploy/osmparser path_to_osm.bz2 path_to_output.graph.bz2`
- *precalculation*: Used to build an overlay graph based on a given partition. Call it with `./deploy/precalculation path_to_graph path_to_mlp output_directory`. Here, *path_to_mlp* is the path to a *MultiLevelPartition* file for the graph that you need to provide. For more details, take a look into our project documentation.
- *customization*: Used to precompute the metric weights for the overlay graph. Call it with `./deploy/customization path_to_graph path_to_overlay_graph metric_output_directory metric_type`. We currently support the following metric types: *hop* (number of edges traversed), *time* and *dist*. You can compute all metrics with *all* as *metric_type*.

Example
-------

Start by calling 

```
./deploy/osmparser examples/karlsruhe/karlsruhe.osm.bz2 examples/karlsruhe/karlsruhe.graph.bz2
```
This will parse the osm map data for the city of Karlsruhe, Germany and writes the extracted graph.

The next step would be to compute a *MultiLevelPartition* for the graph. For this example, we already provide one and can directly continue with computing the overlay graph:

```
./deploy/precalculation examples/karlsruhe/karlsruhe.graph.bz2 examples/karlsruhe/karlsruhe.mlp  examples/karlsruhe/
```

Note that specifying the same folder as output directory (like in this example) will overwrite the initial graph with a new one containing additional information. Since we do some vertex sorting in this step, you cannot call *precalculation* on this graph anymore (i.e. first run the *osmparser* again or choose a different output directory so that the original graph still exists).

In a final step we run the customization phase to build the metric information for the overlay graph:

```
./deploy/customization examples/karlsruhe/karlsruhe.graph.bz2 examples/karlsruhe/karlsruhe.overlay examples/karlsruhe/metrics/ all
```

This completes the precomputation steps and CRP is now ready to compute shortest paths.

Building the Tests
------------------

You can build some tests to see how CRP performs with the following command:

```
scons --target=TEST --optimize=Opt
```

where *TEST* can be one of the following: *QueryTest* (runs our three available query algorithms), *UnpackPathTest* (checks the performance of the *PathUnpacker*), *DijkstraTest* (checks that our query algorithms work as expected) and *OverlayGraphTest* (builds a small overlay graph and performs some sanity tests on it).

The list of required parameters is printed to the terminal by calling the built test program in the *deploy* folder without any additional arguments.


