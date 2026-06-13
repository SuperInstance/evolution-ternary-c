# Evolution Ternary (C)

**Evolution Ternary (C)** is a C library implementing evolutionary algorithms over ternary genomes {-1, 0, +1} — providing genome operations (crossover, mutation), population management, tournament selection, and configurable evolution parameters with a clean C99 API.

## Why It Matters

Binary evolutionary algorithms (genetic algorithms with {0, 1} genomes) are well-studied, but ternary genomes {-1, 0, +1} map naturally to the SuperInstance action space where they encode Avoid, Unknown, and Choose behaviors. This C implementation provides the high-performance computational core for fleet-scale ternary evolution, suitable for embedding in CUDA kernels, microcontrollers, or any environment where a Rust toolchain is unavailable. The C API is designed for FFI compatibility: all structs use explicit sizes, all functions return error codes, and memory management is manual (create/free pairs). This enables integration with Python (via ctypes), Julia, and other languages that interface with C.

## How It Works

**Genome representation:**
```c
typedef struct {
    int  *trits;    // array of {-1, 0, +1}
    size_t length;
} Genome;
```

Each trit takes a full `int` for simplicity and FFI compatibility. A 24-trit genome uses 96 bytes (vs 3 bytes packed), trading memory for speed — array indexing is O(1) without bit manipulation.

**Crossover:**
- **Single-point:** Copy parent A up to point P, parent B from P onward. O(length).
- **Uniform:** For each locus, select from A with probability p, else B. O(length).

**Mutation:**
Each trit mutates with probability `rate`, flipping to a *different* random trit value:

```c
void genome_mutate(Genome *g, double rate) {
    for (size_t i = 0; i < g->length; i++) {
        if (rand_double() < rate) {
            int current = g->trits[i];
            int pick;
            do { pick = values[rand_size(3)]; } while (pick == current);
            g->trits[i] = pick;
        }
    }
}
```

The "mutate to different value" constraint ensures every mutation actually changes the genome — unlike binary mutation where a bit flip to the same value is impossible but a ternary flip has a 1/3 chance of selecting the same value.

**Tournament selection:**
```c
size_t selection_tournament(pop, k):
  best_idx = random
  for i in 1..k:
    candidate = random
    if fitness[candidate] > fitness[best_idx]:
      best_idx = candidate
  return best_idx
```

Tournament size k controls selection pressure: k = 2 (binary tournament) is standard, k = 7 is high pressure.

**Evolution loop:**
```c
for each generation:
  fitness_fn(pop, ctx);           // user-provided fitness function
  sort by fitness;
  preserve top elitism_count;     // elitism
  while population not full:
    parent_a = tournament(k);
    parent_b = tournament(k);
    if rand < crossover_rate:
      child = crossover(a, b);
    else:
      child = copy(a);
    mutate(child, mutation_rate);
    add child to new population;
  swap populations;
```

**EvolutionConfig parameters:**

| Parameter | Default | Effect |
|-----------|---------|--------|
| `mutation_rate` | 0.01 | Per-locus mutation probability |
| `crossover_rate` | 0.7 | Probability of crossover vs. copy |
| `crossover_uniform` | 0 (single-point) | 0 = single-point, 1 = uniform |
| `tournament_size` | 3 | Selection pressure |
| `elitism_count` | 1 | Top individuals preserved |

## Quick Start

```c
#include "evolution_ternary.h"
#include <stdio.h>

/* User-provided fitness function */
void my_fitness(Population *pop, void *ctx) {
    for (size_t i = 0; i < pop->size; i++) {
        double fit = 0.0;
        for (size_t j = 0; j < pop->genome_length; j++) {
            fit += pop->genomes[i]->trits[j]; // sum of trits
        }
        pop->fitness[i] = fit / pop->genome_length;
    }
}

int main() {
    Population *pop = population_create_random(100, 24);
    EvolutionConfig cfg = {
        .mutation_rate = 0.01,
        .crossover_rate = 0.7,
        .crossover_uniform = 0,
        .tournament_size = 3,
        .elitism_count = 2,
    };
    EvolutionResult *result = evolution_run(pop, &cfg, 50, my_fitness, NULL);
    printf("Best fitness: %.4f at generation %zu\n",
           result->best_fitness, result->generation);
    evolution_result_free(result);
    population_free(pop);
}
```

## API

| Type/Function | Description |
|---------------|-------------|
| `Genome` | Ternary genome struct (trits array + length) |
| `Population` | Array of genomes with fitness values |
| `genome_create_random` | Random ternary genome |
| `genome_crossover_single` | Single-point crossover |
| `genome_crossover_uniform` | Uniform crossover |
| `genome_mutate` | Per-locus mutation to different trit |
| `selection_tournament` | Tournament selection |
| `evolution_run` | Full evolution loop with configurable parameters |
| `EvolutionConfig` | Mutation/crossover/tournament/elitism config |
| `EvolutionResult` | Best fitness, index, generation, history |

## Architecture Notes

Evolution Ternary (C) provides the **high-performance evolutionary core** for γ + η = C. The ternary genome directly encodes the conservation-law action space: each trit is a γ-layer decision (Avoid/Unknown/Choose). The evolution loop optimizes η-layer fitness while maintaining the ternary conservation invariants. The C implementation enables CUDA offloading for fleet-scale populations.

See [ARCHITECTURE.md](https://github.com/SuperInstance/SuperInstance/blob/main/ARCHITECTURE.md).

## References

1. Holland, J.H. (1992). *Adaptation in Natural and Artificial Systems*. MIT Press.
2. Goldberg, D.E. (1989). *Genetic Algorithms in Search, Optimization, and Machine Learning*. Addison-Wesley.
3. Bäck, T. (1996). *Evolutionary Algorithms in Theory and Practice*. Oxford University Press.

## License

MIT
