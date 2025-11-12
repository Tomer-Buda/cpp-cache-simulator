# C++ Cache Simulator

A configurable, trace-driven cache simulator developed in C++ as a project for computer architecture studies. This program models a cache with a specified geometry, processes a dynamically generated memory trace, and reports on its performance.

## Features
* **Dynamic Trace Generation:** On every run, the program generates a new, randomized trace file simulating spatial and temporal locality.
* **Fully Configurable:** Easily set cache size, block size, and associativity via a `config.ini` file.
* **LRU Replacement Policy:** Implements the Least Recently Used (LRU) algorithm for cache block eviction.
* **Detailed Performance Metrics:** Reports total accesses, hits, misses, and the final cache hit rate.

## Key Concepts Demonstrated

* **C++:** File I/O (`fstream`), string parsing (`sstream`), STL containers (`vector`, `map`), and random number generation.
* **Computer Architecture:** Caches, memory hierarchy, spatial & temporal locality, hit/miss rates, and replacement policies.
* **Bitwise Operations:** Used to extract the tag, index, and offset from memory addresses.

## How to Run

1.  Clone the repository and open `CacheSimulator.sln` in Visual Studio.
2.  Build the solution.
3.  Place a `config.ini` file in the build directory.
4.  Run the project. A new `trace.txt` will be generated, and the simulation will run on it.
