#include "evolution_ternary.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* ---- RNG helper ---- */
static double rand_double(void) { return (double)rand() / (double)RAND_MAX; }
static size_t rand_size(size_t n) { return (size_t)(rand_double() * (double)n) % n; }

/* ---- Genome ---- */

Genome *genome_create(size_t length) {
    Genome *g = malloc(sizeof(Genome));
    if (!g) return NULL;
    g->trits = calloc(length, sizeof(int));
    if (!g->trits) { free(g); return NULL; }
    g->length = length;
    return g;
}

Genome *genome_create_random(size_t length) {
    Genome *g = genome_create(length);
    if (!g) return NULL;
    static const int values[3] = { TRIT_NEG, TRIT_ZERO, TRIT_POS };
    for (size_t i = 0; i < length; i++)
        g->trits[i] = values[rand_size(3)];
    return g;
}

Genome *genome_copy(const Genome *src) {
    if (!src) return NULL;
    Genome *g = genome_create(src->length);
    if (!g) return NULL;
    memcpy(g->trits, src->trits, src->length * sizeof(int));
    return g;
}

void genome_free(Genome *g) {
    if (!g) return;
    free(g->trits);
    free(g);
}

Genome *genome_crossover_single(const Genome *a, const Genome *b, size_t point) {
    if (!a || !b || a->length != b->length) return NULL;
    size_t len = a->length;
    if (point > len) point = len;
    Genome *child = genome_create(len);
    if (!child) return NULL;
    memcpy(child->trits, a->trits, point * sizeof(int));
    memcpy(child->trits + point, b->trits + point, (len - point) * sizeof(int));
    return child;
}

Genome *genome_crossover_uniform(const Genome *a, const Genome *b, double p) {
    if (!a || !b || a->length != b->length) return NULL;
    size_t len = a->length;
    Genome *child = genome_create(len);
    if (!child) return NULL;
    for (size_t i = 0; i < len; i++)
        child->trits[i] = (rand_double() < p) ? a->trits[i] : b->trits[i];
    return child;
}

void genome_mutate(Genome *g, double rate) {
    if (!g) return;
    static const int values[3] = { TRIT_NEG, TRIT_ZERO, TRIT_POS };
    for (size_t i = 0; i < g->length; i++) {
        if (rand_double() < rate) {
            int current = g->trits[i];
            int pick;
            do { pick = values[rand_size(3)]; } while (pick == current);
            g->trits[i] = pick;
        }
    }
}

/* ---- Population ---- */

Population *population_create(size_t size, size_t genome_length) {
    Population *pop = malloc(sizeof(Population));
    if (!pop) return NULL;
    pop->genomes = calloc(size, sizeof(Genome *));
    pop->fitness = calloc(size, sizeof(double));
    if (!pop->genomes || !pop->fitness) {
        free(pop->genomes); free(pop->fitness); free(pop);
        return NULL;
    }
    pop->size = size;
    pop->genome_length = genome_length;
    return pop;
}

Population *population_create_random(size_t size, size_t genome_length) {
    Population *pop = population_create(size, genome_length);
    if (!pop) return NULL;
    for (size_t i = 0; i < size; i++)
        pop->genomes[i] = genome_create_random(genome_length);
    return pop;
}

void population_free(Population *pop) {
    if (!pop) return;
    for (size_t i = 0; i < pop->size; i++)
        genome_free(pop->genomes[i]);
    free(pop->genomes);
    free(pop->fitness);
    free(pop);
}

void population_set_fitness(Population *pop, size_t idx, double fit) {
    if (!pop || idx >= pop->size) return;
    pop->fitness[idx] = fit;
}

/* ---- Selection ---- */

size_t selection_tournament(const Population *pop, size_t tournament_size) {
    if (!pop || pop->size == 0) return 0;
    if (tournament_size > pop->size) tournament_size = pop->size;
    if (tournament_size == 0) tournament_size = 1;

    size_t best = rand_size(pop->size);
    for (size_t i = 1; i < tournament_size; i++) {
        size_t candidate = rand_size(pop->size);
        if (pop->fitness[candidate] > pop->fitness[best])
            best = candidate;
    }
    return best;
}

/* ---- Evolution ---- */

/* Helper: find index of best fitness in population */
static size_t find_best(const Population *pop) {
    size_t best = 0;
    for (size_t i = 1; i < pop->size; i++)
        if (pop->fitness[i] > pop->fitness[best])
            best = i;
    return best;
}

/* Helper: comparator for sorting indices by fitness descending */
typedef struct { size_t idx; double fit; } IndexFit;

static int cmp_index_fit_desc(const void *a, const void *b) {
    double da = ((const IndexFit *)a)->fit;
    double db = ((const IndexFit *)b)->fit;
    return (da < db) - (da > db);
}

EvolutionResult *evolution_run(Population          *pop,
                               const EvolutionConfig *cfg,
                               size_t                generations,
                               FitnessFn             fitness_fn,
                               void                 *fitness_ctx) {
    if (!pop || !cfg || !fitness_fn) return NULL;

    EvolutionResult *r = malloc(sizeof(EvolutionResult));
    if (!r) return NULL;
    r->pop = pop;
    r->history_best = calloc(generations, sizeof(double));
    r->generations_run = 0;
    r->best_fitness = -DBL_MAX;
    r->best_index = 0;
    r->generation = 0;

    /* Initial evaluation */
    fitness_fn(pop, fitness_ctx);

    for (size_t gen = 0; gen < generations; gen++) {
        r->generation = gen;

        /* Find best */
        size_t gen_best = find_best(pop);
        r->history_best[gen] = pop->fitness[gen_best];
        r->generations_run = gen + 1;

        if (pop->fitness[gen_best] > r->best_fitness) {
            r->best_fitness = pop->fitness[gen_best];
            r->best_index = gen_best;
        }

        /* Build elitism list: top elitism_count individuals by fitness */
        size_t elitism = cfg->elitism_count;
        if (elitism > pop->size) elitism = pop->size;

        IndexFit *ranked = NULL;
        if (elitism > 0) {
            ranked = malloc(pop->size * sizeof(IndexFit));
            for (size_t i = 0; i < pop->size; i++) {
                ranked[i].idx = i;
                ranked[i].fit = pop->fitness[i];
            }
            qsort(ranked, pop->size, sizeof(IndexFit), cmp_index_fit_desc);
        }

        /* Create next generation */
        Genome **next = calloc(pop->size, sizeof(Genome *));

        /* Copy elites */
        for (size_t i = 0; i < elitism; i++)
            next[i] = genome_copy(pop->genomes[ranked[i].idx]);

        /* Fill rest via selection + crossover + mutation */
        for (size_t i = elitism; i < pop->size; i++) {
            size_t p1 = selection_tournament(pop, cfg->tournament_size);
            size_t p2 = selection_tournament(pop, cfg->tournament_size);

            Genome *child;
            if (rand_double() < cfg->crossover_rate) {
                if (cfg->crossover_uniform) {
                    child = genome_crossover_uniform(pop->genomes[p1], pop->genomes[p2], 0.5);
                } else {
                    size_t pt = rand_size(pop->genome_length);
                    child = genome_crossover_single(pop->genomes[p1], pop->genomes[p2], pt);
                }
            } else {
                child = genome_copy(pop->genomes[p1]);
            }

            genome_mutate(child, cfg->mutation_rate);
            next[i] = child;
        }

        /* Replace population */
        for (size_t i = 0; i < pop->size; i++)
            genome_free(pop->genomes[i]);
        for (size_t i = 0; i < pop->size; i++)
            pop->genomes[i] = next[i];
        free(next);
        free(ranked);

        /* Evaluate */
        fitness_fn(pop, fitness_ctx);
    }

    /* Final best */
    size_t final_best = find_best(pop);
    if (pop->fitness[final_best] > r->best_fitness) {
        r->best_fitness = pop->fitness[final_best];
        r->best_index = final_best;
    }

    return r;
}

void evolution_result_free(EvolutionResult *r) {
    if (!r) return;
    free(r->history_best);
    /* Note: does NOT free r->pop; caller owns the population */
    free(r);
}
