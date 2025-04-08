#ifndef GENERATOR_H
#define GENERATOR_H

#include <random>

struct Operation {
    char command;
    int index;
    int value;
};

class Generator {
  private:
    std::mt19937 rng;
    int size;
    int query_percentage;

  public:
    Generator(int size, int query_percentage=20, unsigned int seed = std::random_device{}())
        : size(size), query_percentage(query_percentage) {
        rng = std::mt19937(seed);
    }

    Operation next() {
        std::uniform_int_distribution<int> op_dist(1, 100);             // For operation type
        std::uniform_int_distribution<int> index_dist(0, size - 1);     // For index
        std::uniform_int_distribution<int> value_dist(1, 100);          // For value

        Operation op;

        if (op_dist(rng) <= query_percentage) {
            op.command = 'q'; // Query
        } else {
            op.command = 'a'; // Add
        }

        op.index = index_dist(rng);
        
        if (op.command == 'a') {
            op.value = value_dist(rng);
        }

        return op;
    }
};

#endif