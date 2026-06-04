#ifndef EVOLUTION_TERNARY_H
#define EVOLUTION_TERNARY_H

#include <stddef.h>

/* Trit values */
#define TRIT_NEG  (-1)
#define TRIT_ZERO ( 0)
#define TRIT_POS  (+1)

/* ---- Genome ---- */
typedef struct {
    int  *trits;      /* array of {-1, 0, +1} */
    size_t length;
} Genome;

Genome *genome_create(size_t length);
Genome *genome_create_random(size_t length);
Genome *genome_copy(const Genome *src);
void    genome_free(Genome *g);

/* Crossover */
Genome *genome_crossover_single(const Genome *a, const Genome *b, size_t point);
Genome *genome_crossover_uniform(const Genome *a, const Genome *b, double p);

/* Mutation: each locus mutates with probability rate, to a random different trit */
void genome_mutate(Genome *g, double rate);

/* ---- Population ---- */
typedef struct {
    Genome **genomes;
    double  *fitness;
    size_t   size;
    size_t   genome_length;
} Population;

Population *population_create(size_t size, size_t genome_length);
Population *population_create_random(size_t size, size_t genome_length);
void        population_free(Population *pop);

/* Fitness must be set by caller; this just stores it */
void population_set_fitness(Population *pop, size_t idx, double fit);

/* ---- Selection ---- */
/* Tournament selection: pick tournament_size random individuals, return index of best */
size_t selection_tournament(const Population *pop, size_t tournament_size);

/* ---- Evolution ---- */
typedef struct {
    double mutation_rate;
    double crossover_rate;      /* probability of doing crossover vs copying parent */
    int    crossover_uniform;   /* 0 = single-point, 1 = uniform */
    size_t tournament_size;
    size_t elitism_count;       /* number of top individuals preserved */
} EvolutionConfig;

typedef struct {
    Population *pop;
    double      best_fitness;
    size_t      best_index;
    size_t      generation;
    double     *history_best;   /* best fitness per generation */
    size_t      generations_run;
} EvolutionResult;

/* Run evolution for `generations` generations.
   fitness_fn is called each generation to evaluate the entire population.
   Returns a heap-allocated result; caller must free with evolution_result_free. */
typedef void (*FitnessFn)(Population *pop, void *ctx);

EvolutionResult *evolution_run(Population          *pop,
                               const EvolutionConfig *cfg,
                               size_t                generations,
                               FitnessFn             fitness_fn,
                               void                 *fitness_ctx);

void evolution_result_free(EvolutionResult *r);

#endif /* EVOLUTION_TERNARY_H */
