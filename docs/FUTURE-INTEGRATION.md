# Future Integration: evolution-ternary-c

## Current State
C implementation of ternary evolution with tournament selection, crossover, and mutation for ternary genomes (trits: -1, 0, +1). Provides the basic evolutionary operators for on-device optimization.

## Integration Opportunities

### With ternary-fitness-c
Fitness evaluation drives evolution. `evolution-ternary-c` selects parents and generates offspring; `ternary-fitness-c` evaluates them. Together: a complete evolutionary loop on ESP32. Agents evolve their behavior in-situ based on local conditions.

### With ternary-evolution-advanced (Rust)
Rust provides DE, CMA-ES, NSGA-II. C provides tournament selection. Cross-language pipeline: use Rust to find the optimal evolutionary parameters (mutation rate, tournament size), hardcode them into the C implementation for edge deployment.

### With ternary-cell
Cell populations evolve. The `gc` phase is a selection event: low-fitness cells die. New cells are created through crossover of surviving cells. `evolution-ternary-c` provides the operators for on-device cell population evolution.

## Potential in Mature Systems
In room-as-codespace, rooms evolve their local agent populations. The C port runs on ESP32/Jetson, evolving agents to fit local conditions. Cloud (Rust) discovers general strategies; edge (C) refines them for local context. Cross-language evolution: cloud discovers, edge refines.

## Cross-Pollination Ideas
- Tournament selection as room priority ranking — which rooms get access to shared resources
- Crossover as room configuration blending — merge successful room configurations
- Mutation as controlled experimentation — occasionally try random room configurations

## Dependencies for Next Steps
- Integration with ternary-fitness-c for complete evolution loop
- FFI bindings for Rust interop
- ESP32 memory profiling for population size limits
