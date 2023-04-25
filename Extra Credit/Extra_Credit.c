#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Cache line struct
typedef struct cache_line {
    int valid;
    int tag;
    time_t access_time;
} cache_line;

// Cache struct
typedef struct cache {
    int size;
    int line_size;
    int num_lines;
    cache_line* lines;
} cache;

// Hash function for direct-mapped cache
int direct_map_hash(int address, int num_lines) {
    return address % num_lines;
}

// Function to find a cache line in a set-associative cache
int find_cache_line(cache* c, int set_index, int tag) {
    int i;
    for (i = 0; i < c->line_size; i++) {
        int index = set_index * c->line_size + i;
        cache_line line = c->lines[index];
        if (line.valid && line.tag == tag) {
            return index;
        }
    }
    return -1;
}

// Function to find a cache line in a fully associative cache
int find_cache_line_fully(cache* c, int tag) {
    int i;
    for (i = 0; i < c->num_lines; i++) {
        cache_line line = c->lines[i];
        if (line.valid && line.tag == tag) {
            return i;
        }
    }
    return -1;
}

// Function to update access time of cache line
void update_access_time(cache* c, int index) {
    c->lines[index].access_time = time(NULL);
}

// Function to evict least-recently used line from set-associative cache
void evict_lru_line(cache* c, int set_index) {
    int i, oldest_index;
    time_t oldest_time = time(NULL);
    for (i = 0; i < c->line_size; i++) {
        int index = set_index * c->line_size + i;
        cache_line line = c->lines[index];
        if (line.valid && line.access_time < oldest_time) {
            oldest_index = index;
            oldest_time = line.access_time;
        }
    }
    c->lines[oldest_index].valid = 0;
}

// Function to evict a random line from set-associative cache
void evict_random_line(cache* c, int set_index) {
    int random_index = set_index * c->line_size + (rand() % c->line_size);
    c->lines[random_index].valid = 0;
}

// Function to evict least-recently used line from fully associative cache
void evict_lru_line_fully(cache* c) {
    int i, oldest_index;
    time_t oldest_time = time(NULL);
    for (i = 0; i < c->num_lines; i++) {
        cache_line line = c->lines[i];
        if (line.valid && line.access_time < oldest_time) {
            oldest_index = i;
            oldest_time = line.access_time;
        }
    }
    c->lines[oldest_index].valid = 0;
}

// Function to evict a random line from fully associative cache
void evict_random_line_fully(cache* c) {
    int random_index = rand() % c->num_lines;
    c->lines[random_index].valid = 0;
}

// Function to access memory address in direct-mapped cache
void access_direct_map(cache* c, int address) {
    int line_index = direct_map_hash(address, c->num_lines);
    cache_line line = c->lines[line_index];
    if (line.valid && line.tag == address / c->num_lines) {
        update_access_time(c, line_index);
    } else {
        c->lines[line_index].valid = 1;
        c->lines[line_index].tag = address / c->num_lines;
        update_access_time(c, line_index);

    }
}

// Function to access memory address in set-associative cache
void access_set_assoc(cache* c, int address, int num_ways, int evict_policy) {
    int set_index = address % (c->num_lines / num_ways);
    int tag = address / (c->num_lines / num_ways);
    int line_index = find_cache_line(c, set_index, tag);
    if (line_index != -1) {
        update_access_time(c, line_index);
        printf("Cache hit!\n");
    } else {
        int i;
        for (i = 0; i < num_ways; i++) {
            int index = set_index * c->line_size + i;
            if (!c->lines[index].valid) {
                line_index = index;
                break;
            }
        }
        if (line_index == -1) {
            if (evict_policy == 0) {
                evict_lru_line(c, set_index);
            } else {
                evict_random_line(c, set_index);
            }
            line_index = set_index * c->line_size + (rand() % num_ways);
        }
        c->lines[line_index].valid = 1;
        c->lines[line_index].tag = tag;
        update_access_time(c, line_index);
        printf("Cache miss!\n");
    }
}

// Function to access memory address in fully associative cache
void access_fully_assoc(cache* c, int address, int evict_policy) {
    int line_index = find_cache_line_fully(c, address);
    if (line_index != -1) {
        update_access_time(c, line_index);
    } else {
        int i;
        for (i = 0; i < c->num_lines; i++) {
            if (!c->lines[i].valid) {
                line_index = i;
                break;
            }
        }
        if (line_index == -1) {
            if (evict_policy == 0) {
                evict_lru_line_fully(c);
            } else {
                evict_random_line_fully(c);
            }
            line_index = rand() % c->num_lines;
        }
        c->lines[line_index].valid = 1;
        c->lines[line_index].tag = address;
        update_access_time(c, line_index);
    }
}
#define CACHE_SIZE 8
#define LINE_SIZE 2
#define NUM_WAYS 2

int main() {
    // Initialize cache
    cache* c = (cache*) malloc(sizeof(cache));
    c->size = CACHE_SIZE;
    c->line_size = LINE_SIZE;
    c->num_lines = CACHE_SIZE / LINE_SIZE;
    c->lines = (cache_line*) malloc(CACHE_SIZE * sizeof(cache_line));
    memset(c->lines, 0, CACHE_SIZE * sizeof(cache_line));

    // Initialize variables for hit rate calculation
    int hits = 0;
    int accesses = 0;
    double hit_rate = 0.0;

    // Open file for reading
    FILE* file = fopen("traces.txt", "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    // Read file line by line
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Parse memory address from line
        int address;
        sscanf(line, "%x", &address);

        // Access memory address in cache
        access_set_assoc(c, address, NUM_WAYS, 0);

        // Update hit rate calculation variables
        if (strstr(line, "hit") != NULL) {
            hits++;
        }
        accesses++;
    }

    // Calculate hit rate
    if (accesses > 0) {
        hit_rate = (double)hits / (double)accesses;
    }
        

    //Print out results
    printf("Hits: %d\n", hits);
    printf("Total accesses: %d\n", accesses);
    printf("Hit rate: %.2f\n", hit_rate);

    // Free memory and close file
    free(c->lines);
    free(c);
    fclose(file);

    return 0;
}
