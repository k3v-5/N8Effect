# C++ DSP Core Reference Implementations (Archival & Test Oracles)

> **AVISO DE ARQUITECTURA (Regla 13, 46, 50)**:
> Los módulos ubicados en este directorio (`source/dsp/core/`) corresponden a implementaciones canónicas de referencia matemática en C++.
> Se conservan como oráculos comparativos y fixtures para la suite de pruebas automatizadas (`N8Effect_Tests`).
> 
> **En producción y tiempo de ejecución en vivo**, el 100% de los algoritmos DSP, filtros, retardos, transformadas FFT y física acústica se procesan de forma nativa en **Rust** a través de `source/dsp/rust_core/`, garantizando cero alocaciones dinámicas, protección anti-denormal (FTZ/DAZ), paralelismo SIMD AVX2 y máxima eficiencia de CPU.
