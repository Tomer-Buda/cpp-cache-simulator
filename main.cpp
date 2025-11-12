#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>
#include <climits>
#include <random>
#include <ctime>

// --- Data Structure for a Cache Block ---
struct CacheBlock {
    bool valid;
    unsigned long long tag;
    int lru_counter; // Will store a "timestamp" of when it was last used

    // Constructor to initialize a new block
    CacheBlock() : valid(false), tag(0), lru_counter(0) {}
};

// --- Global variables ---
std::vector<std::vector<CacheBlock>> cache; // The cache itself
long long hits = 0;
long long misses = 0;
int global_time_counter = 0; // Our "clock" for LRU

// --- Function to Parse config.ini ---
// This function reads the config file and returns a map of key-value pairs
std::map<std::string, std::string> parseConfig(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream configFile(filename);

    if (!configFile.is_open()) {
        std::cerr << "Error: Could not open config file " << filename << std::endl;
        exit(1); // Exit if config file not found
    }

    std::string line;
    while (std::getline(configFile, line)) {
        // Use stringstream to split the line by the ':' separator
        std::stringstream ss(line);
        std::string key, value;

        // Read the key (e.g., "CACHE_SIZE_KB")
        if (std::getline(ss, key, ':')) {
            // Read the value (e.g., " 32")
            if (std::getline(ss, value)) {
                // Trim leading whitespace from value
                value.erase(0, value.find_first_not_of(" \t"));
                // Store in the map as a string
                config[key] = value;
            }
        }
    }

    configFile.close();
    return config;
}

// --- Core Cache Logic Function ---
// This function is the core of our simulator
void accessCache(unsigned long long address, int offset_bits, int index_bits, int tag_bits) {
    // We increment our global "clock" on every access
    global_time_counter++;

    // 1. Calculate Tag and Index from the address

    // First, shift off the offset bits
    unsigned long long address_no_offset = address >> offset_bits;

    // Next, create a "mask" to extract the index bits
    // If index_bits is 4, (1 << 4) is 10000. (1 << 4) - 1 is 01111.
    unsigned long long index_mask = (1ULL << index_bits) - 1;

    // Use the mask to get the index
    unsigned long long index = address_no_offset & index_mask;

    // The rest of the bits are the tag
    unsigned long long tag = address_no_offset >> index_bits;

    // 2. Get the corresponding set from the cache
    std::vector<CacheBlock>& set = cache[index];

    // 3. Check for a Hit
    for (int i = 0; i < set.size(); ++i) {
        if (set[i].valid && set[i].tag == tag) {
            // --- HIT! ---
            hits++;
            // Update the LRU counter to show it was "just used"
            set[i].lru_counter = global_time_counter;
            return; // We are done
        }
    }

    // 4. Handle a Miss
    // If we are here, it means we looped and found no matching tag
    // --- MISS! ---
    misses++;

    // 5. Find a place to put the new block

    // First, try to find an "invalid" (empty) block
    for (int i = 0; i < set.size(); ++i) {
        if (!set[i].valid) {
            // Found an empty slot. This is a "cold miss".
            set[i].valid = true;
            set[i].tag = tag;
            set[i].lru_counter = global_time_counter;
            return; // We are done
        }
    }

    // 6. If no invalid blocks, we must EVIC a block (LRU)
    // This is a "capacity" or "conflict" miss.
    // We need to find the block with the *smallest* lru_counter

    int lru_way = 0;
    int min_lru_value = INT_MAX;

    for (int i = 0; i < set.size(); ++i) {
        if (set[i].lru_counter < min_lru_value) {
            min_lru_value = set[i].lru_counter;
            lru_way = i;
        }
    }

    // We found our victim! Replace it.
    set[lru_way].valid = true; // It's now valid
    set[lru_way].tag = tag; // with the new tag
    set[lru_way].lru_counter = global_time_counter; // and the current time
}


void generateTrace() {
    // Seed the random number generator
    std::srand((unsigned int)std::time(nullptr));

    // Create the trace file (this overwrites the old one)
    std::ofstream trace_file("trace.txt");

    int num_accesses = 5000; // Let's create 5000 accesses

    // Create a "hot" address for temporal locality
    unsigned long long hot_address = 0x1A000 + (std::rand() % 20) * 4;

    for (int i = 0; i < num_accesses; ++i) {

        int access_type = std::rand() % 100;

        // --- Decide what kind of access to simulate ---

        if (access_type < 50) {
            // 50% chance: Spatial Locality (like our array loop)
            // Access addresses 0x10000, 0x10004, 0x10008, etc.
            unsigned long long address = 0x10000 + (i * 4);
            trace_file << "R 0x" << std::hex << address << "\n";

        }
        else if (access_type < 80) {
            // 30% chance: Temporal Locality (hit that "hot" address)
            trace_file << "W 0x" << std::hex << hot_address << "\n";

        }
        else {
            // 20% chance: Random Access (jump somewhere else)
            unsigned long long random_address = (std::rand() % 0xFFFF) * 4;
            trace_file << "R 0x" << std::hex << random_address << "\n";
        }
    }

    trace_file.close();
    std::cout << "--- New 'trace.txt' generated with " << num_accesses << " accesses ---" << std::endl;
}


int main() {
	// For testing, let's first generate a trace file
    generateTrace();
    // 1. Parse the config file
    std::map<std::string, std::string> config = parseConfig("config.ini");

    // For testing, let's print what we found
    std::cout << "--- Configuration ---" << std::endl;
    std::cout << "Cache Size: " << config["CACHE_SIZE_KB"] << " KB" << std::endl;
    std::cout << "Block Size: " << config["BLOCK_SIZE_BYTES"] << " Bytes" << std::endl;
    std::cout << "Associativity: " << config["ASSOCIATIVITY"] << std::endl;
    std::cout << "---------------------" << std::endl;

    // 2. Calculate cache parameters
    // We use std::stoi here to convert the string from the map to an int
    int cache_size = std::stoi(config["CACHE_SIZE_KB"]) * 1024; // Size in bytes
    int block_size = std::stoi(config["BLOCK_SIZE_BYTES"]);
    int associativity = std::stoi(config["ASSOCIATIVITY"]);

    // Total number of blocks in the cache
    int num_blocks = cache_size / block_size;

    // Number of sets
    // Add a check to prevent division by zero if associativity is 0
    if (associativity == 0) {
        std::cerr << "Error: Associativity cannot be zero." << std::endl;
        return 1;
    }
    int num_sets = num_blocks / associativity;

    // Add a check for num_sets being zero (e.g., cache size too small)
    if (num_sets == 0) {
        std::cerr << "Error: Number of sets is zero. Check cache/block size." << std::endl;
        return 1;
    }

    // Number of bits needed for...
    // ...the Offset (to find a byte within a block)
    int offset_bits = (int)std::log2(block_size);
    // ...the Index (to find the set)
    int index_bits = (int)std::log2(num_sets);
    // ...the Tag (the rest of the bits)
    // We assume a 64-bit address space
    int tag_bits = 64 - index_bits - offset_bits;

    // Let's print these too
    std::cout << "--- Cache Geometry ---" << std::endl;
    std::cout << "Num Sets: " << num_sets << std::endl;
    std::cout << "Offset Bits: " << offset_bits << std::endl;
    std::cout << "Index Bits: " << index_bits << std::endl;
    std::cout << "Tag Bits: " << tag_bits << std::endl;
    std::cout << "----------------------" << std::endl;

    // 3. Initialize the cache data structure
    // Resize the outer vector to the number of sets
    cache.resize(num_sets);
    for (int i = 0; i < num_sets; ++i) {
        // Resize each inner vector to the associativity
        cache[i].resize(associativity);
    }

    // 4. Process the trace file
    std::ifstream traceFile("trace.txt");
    if (!traceFile.is_open()) {
        std::cerr << "Error: Could not open trace file trace.txt" << std::endl;
        return 1;
    }

    std::string line;
    char access_type; // 'R' or 'W'
    std::string address_hex; // "0x..."

    while (std::getline(traceFile, line)) {
        std::stringstream ss(line);

        // Parse the line: R 0x...
        if (!(ss >> access_type >> address_hex)) {
            continue; // Skip empty or bad lines
        }

        // Convert the hex address string to a number
        // std::stoull (string to unsigned long long)
        // The '0' base automatically detects the "0x" prefix
        unsigned long long address = std::stoull(address_hex, 0, 0);

        // Call our simulator logic!
        accessCache(address, offset_bits, index_bits, tag_bits);
    }

    traceFile.close();

    // 5. Print the final results
    std::cout << "\n--- Simulation Results ---" << std::endl;
    long long total_accesses = hits + misses;
    double hit_rate = (total_accesses == 0) ? 0.0 : (double)hits / total_accesses;

    std::cout << "Total Accesses: " << total_accesses << std::endl;
    std::cout << "Hits: " << hits << std::endl;
    std::cout << "Misses: " << misses << std::endl;

    // Use iomanip to format the hit rate nicely
    std::cout << "Hit Rate: " << std::fixed << std::setprecision(4)
        << (hit_rate * 100.0) << "%" << std::endl;
    std::cout << "--------------------------" << std::endl;

    return 0; // End of main
}