#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "../src/evolution_ternary.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  %-50s ", name);
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

/* ======== Fitness functions for testing ======== */

/* Count number of +1 trits (target: all +1) */
static void fitness_all_positive(Population *pop, void *ctx) {
    (void)ctx;
    for (size_t i = 0; i < pop->size; i++) {
        double count = 0;
        for (size_t j = 0; j < pop->genomes[i]->length; j++)
            if (pop->genomes[i]->trits[j] == TRIT_POS) count++;
        pop->fitness[i] = count;
    }
}

/* Sum of trits (max when all +1) */
static void fitness_sum_trits(Population *pop, void *ctx) {
    (void)ctx;
    for (size_t i = 0; i < pop->size; i++) {
        double sum = 0;
        for (size_t j = 0; j < pop->genomes[i]->length; j++)
            sum += pop->genomes[i]->trits[j];
        pop->fitness[i] = sum;
    }
}

/* ======== Tests ======== */

static void test_genome_create(void) {
    TEST("genome_create basic");
    Genome *g = genome_create(10);
    ASSERT(g != NULL, "genome should not be NULL");
    ASSERT(g->length == 10, "length should be 10");
    for (size_t i = 0; i < g->length; i++)
        ASSERT(g->trits[i] == 0, "trits should be initialized to 0");
    genome_free(g);
    PASS();
}

static void test_genome_create_random(void) {
    TEST("genome_create_random valid trits");
    srand(42);
    Genome *g = genome_create_random(100);
    ASSERT(g != NULL, "genome should not be NULL");
    for (size_t i = 0; i < g->length; i++) {
        int t = g->trits[i];
        ASSERT(t == TRIT_NEG || t == TRIT_ZERO || t == TRIT_POS, "trit out of range");
    }
    genome_free(g);
    PASS();
}

static void test_genome_copy(void) {
    TEST("genome_copy identical");
    srand(7);
    Genome *g = genome_create_random(20);
    Genome *c = genome_copy(g);
    ASSERT(c != NULL, "copy should not be NULL");
    ASSERT(c != g, "copy should be different object");
    ASSERT(c->trits != g->trits, "trits should be different buffer");
    ASSERT(memcmp(c->trits, g->trits, 20 * sizeof(int)) == 0, "trits should match");
    genome_free(g);
    genome_free(c);
    PASS();
}

static void test_genome_copy_null(void) {
    TEST("genome_copy NULL returns NULL");
    Genome *c = genome_copy(NULL);
    ASSERT(c == NULL, "copy of NULL should be NULL");
    PASS();
}

static void test_crossover_single_point(void) {
    TEST("single-point crossover");
    srand(99);
    Genome *a = genome_create(10);
    Genome *b = genome_create(10);
    for (size_t i = 0; i < 10; i++) { a->trits[i] = TRIT_NEG; b->trits[i] = TRIT_POS; }

    Genome *child = genome_crossover_single(a, b, 5);
    ASSERT(child != NULL, "child should not be NULL");
    for (size_t i = 0; i < 5; i++)
        ASSERT(child->trits[i] == TRIT_NEG, "first half from parent a");
    for (size_t i = 5; i < 10; i++)
        ASSERT(child->trits[i] == TRIT_POS, "second half from parent b");

    genome_free(a); genome_free(b); genome_free(child);
    PASS();
}

static void test_crossover_uniform(void) {
    TEST("uniform crossover mixes parents");
    srand(123);
    Genome *a = genome_create(100);
    Genome *b = genome_create(100);
    for (size_t i = 0; i < 100; i++) { a->trits[i] = TRIT_NEG; b->trits[i] = TRIT_POS; }

    Genome *child = genome_crossover_uniform(a, b, 0.5);
    ASSERT(child != NULL, "child should not be NULL");
    int from_a = 0, from_b = 0;
    for (size_t i = 0; i < 100; i++) {
        if (child->trits[i] == TRIT_NEG) from_a++;
        else from_b++;
    }
    /* With p=0.5 and 100 trits, both should be present (extremely unlikely to be all one) */
    ASSERT(from_a > 10 && from_b > 10, "should have genes from both parents");

    genome_free(a); genome_free(b); genome_free(child);
    PASS();
}

static void test_mutation_changes_trits(void) {
    TEST("mutation changes some trits");
    srand(55);
    Genome *g = genome_create(100);
    for (size_t i = 0; i < 100; i++) g->trits[i] = TRIT_ZERO;

    genome_mutate(g, 1.0); /* 100% mutation rate */
    int changed = 0;
    for (size_t i = 0; i < 100; i++)
        if (g->trits[i] != TRIT_ZERO) changed++;
    ASSERT(changed == 100, "all trits should change at rate 1.0");

    genome_free(g);
    PASS();
}

static void test_mutation_zero_rate(void) {
    TEST("mutation rate 0.0 changes nothing");
    Genome *g = genome_create_random(50);
    Genome *orig = genome_copy(g);

    genome_mutate(g, 0.0);
    ASSERT(memcmp(g->trits, orig->trits, 50 * sizeof(int)) == 0, "no trits should change");

    genome_free(g); genome_free(orig);
    PASS();
}

static void test_population_create_random(void) {
    TEST("population_create_random basic");
    srand(10);
    Population *pop = population_create_random(20, 15);
    ASSERT(pop != NULL, "pop should not be NULL");
    ASSERT(pop->size == 20, "size should be 20");
    ASSERT(pop->genome_length == 15, "genome_length should be 15");

    int all_valid = 1;
    for (size_t i = 0; i < pop->size && all_valid; i++) {
        if (!pop->genomes[i]) all_valid = 0;
        else for (size_t j = 0; j < pop->genomes[i]->length; j++) {
            int t = pop->genomes[i]->trits[j];
            if (t < TRIT_NEG || t > TRIT_POS) all_valid = 0;
        }
    }
    ASSERT(all_valid, "all genomes should have valid trits");

    population_free(pop);
    PASS();
}

static void test_tournament_selection(void) {
    TEST("tournament selection favors fitter");
    srand(77);
    Population *pop = population_create(10, 5);
    for (size_t i = 0; i < 10; i++) {
        pop->genomes[i] = genome_create(5);
        pop->fitness[i] = (double)i; /* index 9 is fittest */
    }

    int picked_9 = 0;
    for (int trial = 0; trial < 1000; trial++) {
        size_t sel = selection_tournament(pop, 3);
        if (sel == 9) picked_9++;
    }
    /* With tournament_size=3, fittest should be picked much more than random (10%) */
    ASSERT(picked_9 > 200, "fittest should be picked frequently");

    population_free(pop);
    PASS();
}

static void test_evolution_improves_fitness(void) {
    TEST("evolution improves fitness over generations");
    srand(42);

    Population *pop = population_create_random(50, 20);

    EvolutionConfig cfg = {
        .mutation_rate = 0.05,
        .crossover_rate = 0.8,
        .crossover_uniform = 0,
        .tournament_size = 3,
        .elitism_count = 2
    };

    EvolutionResult *r = evolution_run(pop, &cfg, 100, fitness_all_positive, NULL);
    ASSERT(r != NULL, "result should not be NULL");
    ASSERT(r->generations_run == 100, "should run 100 generations");

    /* First generation best should be <= last generation best */
    double first_best = r->history_best[0];
    double last_best = r->history_best[99];
    printf("(first=%.1f last=%.1f) ", first_best, last_best);
    ASSERT(last_best >= first_best, "fitness should improve or stay same");
    ASSERT(r->best_fitness > 0, "best fitness should be positive");

    evolution_result_free(r);
    population_free(pop);
    PASS();
}

static void test_evolution_with_elitism_preserves_best(void) {
    TEST("elitism preserves best individual");
    srand(88);

    Population *pop = population_create_random(30, 10);

    EvolutionConfig cfg = {
        .mutation_rate = 0.02,
        .crossover_rate = 0.9,
        .crossover_uniform = 1,
        .tournament_size = 2,
        .elitism_count = 3
    };

    EvolutionResult *r = evolution_run(pop, &cfg, 50, fitness_sum_trits, NULL);
    ASSERT(r != NULL, "result should not be NULL");

    /* Best fitness should equal or exceed history[0] */
    ASSERT(r->best_fitness >= r->history_best[0], "best should not regress");

    evolution_result_free(r);
    population_free(pop);
    PASS();
}

static void test_evolution_converges_to_optimal(void) {
    TEST("evolution converges on short genome");
    srand(314);

    Population *pop = population_create_random(100, 5);

    EvolutionConfig cfg = {
        .mutation_rate = 0.03,
        .crossover_rate = 0.85,
        .crossover_uniform = 0,
        .tournament_size = 5,
        .elitism_count = 5
    };

    EvolutionResult *r = evolution_run(pop, &cfg, 200, fitness_all_positive, NULL);
    ASSERT(r != NULL, "result should not be NULL");
    printf("(best=%.1f/%zu) ", r->best_fitness, pop->genome_length);
    /* With small genome and many generations, should reach optimal */
    ASSERT(r->best_fitness >= (double)pop->genome_length, "should reach optimal");

    evolution_result_free(r);
    population_free(pop);
    PASS();
}

static void test_population_set_fitness(void) {
    TEST("population_set_fitness stores values");
    Population *pop = population_create(5, 3);
    for (size_t i = 0; i < 5; i++) pop->genomes[i] = genome_create(3);

    population_set_fitness(pop, 0, 3.14);
    population_set_fitness(pop, 4, -2.71);
    ASSERT(pop->fitness[0] == 3.14, "fitness[0] should be 3.14");
    ASSERT(pop->fitness[4] == -2.71, "fitness[4] should be -2.71");

    population_free(pop);
    PASS();
}

static void test_history_length_matches_generations(void) {
    TEST("history array length matches generations");
    srand(99);
    Population *pop = population_create_random(10, 8);
    EvolutionConfig cfg = { 0.1, 0.5, 0, 2, 1 };

    EvolutionResult *r = evolution_run(pop, &cfg, 25, fitness_sum_trits, NULL);
    ASSERT(r->generations_run == 25, "should run 25 generations");
    /* history should be monotonically non-decreasing with elitism */
    int monotonic = 1;
    for (size_t i = 1; i < r->generations_run; i++) {
        if (r->history_best[i] < r->history_best[i-1]) monotonic = 0;
    }
    ASSERT(monotonic, "history should be non-decreasing with elitism=1");

    evolution_result_free(r);
    population_free(pop);
    PASS();
}

static void test_free_null_safety(void) {
    TEST("free NULL pointers is safe");
    genome_free(NULL);
    population_free(NULL);
    evolution_result_free(NULL);
    PASS();
}

/* ======== Main ======== */

int main(void) {
    printf("\n=== evolution-ternary-c tests ===\n\n");

    test_genome_create();
    test_genome_create_random();
    test_genome_copy();
    test_genome_copy_null();
    test_crossover_single_point();
    test_crossover_uniform();
    test_mutation_changes_trits();
    test_mutation_zero_rate();
    test_population_create_random();
    test_tournament_selection();
    test_population_set_fitness();
    test_evolution_improves_fitness();
    test_evolution_with_elitism_preserves_best();
    test_evolution_converges_to_optimal();
    test_history_length_matches_generations();
    test_free_null_safety();

    printf("\n%d passed, %d failed\n\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
