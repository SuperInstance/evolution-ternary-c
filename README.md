# evolution-ternary-c

C implementation of ternary evolution — tournament selection, crossover, and mutation for ternary genomes (trits: -1, 0, +1).

## Components

- **Genome**: Array of trits `{-1, 0, +1}` with create/copy/crossover/mutate operations
- **Population**: Collection of N genomes with per-individual fitness
- **Selection**: Tournament selection with configurable tournament size
- **Crossover**: Single-point and uniform crossover
- **Mutation**: Per-locus mutation with configurable rate (always mutates to a *different* trit)
- **Evolution**: Run N generations with elitism, track best fitness history

## Build & Test

```bash
gcc -o test_evolution tests/test_evolution.c src/evolution_ternary.c -lm -Wall -O2
./test_evolution
```

## Usage

```c
#include "src/evolution_ternary.h"

// Define a fitness function
void my_fitness(Population *pop, void *ctx) {
    for (size_t i = 0; i < pop->size; i++) {
        double fit = 0;
        for (size_t j = 0; j < pop->genomes[i]->length; j++)
            fit += pop->genomes[i]->trits[j];
        pop->fitness[i] = fit;
    }
}

int main(void) {
    srand(42);
    Population *pop = population_create_random(100, 20);

    EvolutionConfig cfg = {
        .mutation_rate = 0.05,
        .crossover_rate = 0.8,
        .crossover_uniform = 0,
        .tournament_size = 3,
        .elitism_count = 2
    };

    EvolutionResult *r = evolution_run(pop, &cfg, 200, my_fitness, NULL);
    printf("Best fitness: %.2f\n", r->best_fitness);

    evolution_result_free(r);
    population_free(pop);
    return 0;
}
```

## License

MIT
