# Audio Event Graph Engine (N8Effect)

[![Repository](https://img.shields.io/badge/Repository-k3v--5%2FN8Effect-blue?logo=github)](https://github.com/k3v-5/N8Effect.git)
[![Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Framework](https://img.shields.io/badge/JUCE-8.0.4-orange.svg)](https://juce.com/)
[![Tests](https://img.shields.io/badge/Tests-54%20Passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-Proprietary-lightgrey.svg)]()

> **Repositorio Oficial:** [https://github.com/k3v-5/N8Effect.git](https://github.com/k3v-5/N8Effect.git)

---

## 1. Visión General del Proyecto

**N8Effect (Audio Event Graph Engine)** es un plugin VST3 y aplicación Standalone de efectos modulares de audio de última generación, inspirado conceptualmente en la familia de efectos creativos de **Arturia (Efx MOTIONS, Efx FRAGMENTS, Efx REFRACT)**. 

Combina la flexibilidad intuitiva de procesamiento secuencial en cola/cadena (*Insert Slots / Sequential Chains*) con la potencia ilimitada de un grafo acíclico dirigido (**DAG**) totalmente modular y un motor de síntesis y fragmentación por eventos acústicos soberanos (**Event World**).

### Características Clave
- **Estilo Modular & Secuencial**: Permite tanto encadenar elementos en serie/cola linealmente (estilo slots o containers) como crear ramificaciones paralelas, lazos de feedback controlados, crossovers multibanda y subgrafos anidados jerárquicamente.
- **Arquitectura Dual World**: Separación estricta entre la señal original (*Dry World*, matemáticamente intocada con fidelidad de fase perfecta de 0 dB) y el mundo de efectos y eventos acústicos (*Wet/Event World*).
- **Event Engine Soberano**: Generación, manipulación y reciclaje de fragmentos acústicos con ciclo de vida explícito (`CREATED -> PLAYING -> RELEASING -> FROZEN -> LOOPING -> KILLED -> DESTROYED`), seguimiento de energía (*Source Following* de 0.0 a 1.0) y políticas de extinción configurables (`CUT`, `SHORT`, `FADE`, `NATURAL`, `HOLD`, `FREEZE`).
- **Modulación Universal Centralizada**: Matriz de modulación unificada (64 rutas concurrentes) alimentada por LFOs sincronizados a DAW, envolventes ADSR analógicas, secuenciador de pasos (32 pasos con glide y probabilidad estocástica), seguidor de envolvente dinámico, 8 Macros globales y XY Pad interactivo.
- **Motor de Análisis Acústico en Vivo**: Detección de transitorios en tiempo real, seguimiento de tono fundamental (*YIN sub-sample pitch tracker*) y extractor espectral (Centroide tímbrico y Flujo espectral) disponibles como moduladores universales.
- **Procesamiento Espacial 3D & Mid/Side**: Paneo psicoacústico 3D binaural (Woodworth ITD + sombra de cabeza ILD) aplicable a nivel de nodo o por evento acústico individual, junto con codificador/decodificador Mid/Side dotado de *Mono Bass Maker* (Butterworth paso alto de 20 a 400 Hz en canal Side).
- **Real-Time Audio Safety (Anti-Glitch / Cero Allocations)**: Cumplimiento riguroso de la Regla de Oro de audio en tiempo real: cero llamadas a `malloc`/`new`/`free`, colas lock-free, pools de memoria fijos prealocados (`EventPool` de 1024 slots, `GrainPool` de 128 granos, `AudioBufferPool`), aproximaciones matemáticas analíticas de Padé (`FastMath`) y protección anti-denormales por hardware (FTZ + DAZ).

---

## 2. Catálogo de Procesadores y Nodos DSP (23 Módulos)

El motor cuenta con un catálogo extensible registrado en `NodeFactory`:

| Categoría | Procesadores Disponibles |
| :--- | :--- |
| **Dinámica & Ganancia** | `CompressorNode` (VCA soft-knee), `MultibandDynamicsNode` (Crossover Linkwitz-Riley LR4 de 3 bandas + OTT descendente/ascendente), `PassthroughNode` |
| **Filtros & EQ** | `ParametricEQNode` (3 bandas paramétricas completas), `SimpleFilterNode`, `ResonatorBankNode` (Banco de 6 resonadores modales/armónicos) |
| **Tiempo & Espacio** | `AdvancedDelayNode` (Ping-Pong estéreo, cross-feedback), `SimpleDelayNode`, `ReverbNode` (FDN de 4 líneas con matriz unitaria Householder), `SpatialPannerNode` (Acimut, Elevación, Distancia 3D) |
| **Modulación & Tono** | `PhaserNode` (6 etapas allpass), `ChorusNode` (1 a 4 voces en cuadratura a 90°), `FlangerNode` (retardo en peine bipolar), `RingModulatorNode` (4 cuadrantes), `PitchShifterNode` (doble cabezal anti-click), `FrequencyShifterNode` (Hilbert 90° SSB) |
| **Granular & Glitch** | `GranularNode` (Nube granular con jitter y pitch), `GlitchNode` (Beat slicer y repeticiones estocásticas), `SpectralFreezeNode` (Retención STFT de magnitudes con smear), `SpectralProcessorNode` |
| **Saturación & Color** | `DistortionNode` (5 algoritmos: Soft, Hard, Wavefold, Tube, Bitcrush), `TapeSaturationNode` (Cinta abierta con calor analógico y wow/flutter) |
| **Ruteo & Espacialización** | `MidSideEncoderNode`, `MidSideDecoderNode` (Con Mono Bass Maker), `ContainerNode` (Subgrafos jerárquicos), `FeedbackContainerNode` (Lazos con saturación y DC blocker), `EventContainerNode` (Rack de eventos acústicos) |

---

## 3. Presets, Escenas y Morphing

- **Dual Scene Morphing**: Captura instantánea de Escena A y Escena B con interpolación continua anti-click mediante un control de Morphing `t` (0.0 a 1.0).
- **Historial Deshacer/Rehacer**: Pila acotada de Undo/Redo con snapshots completos de topología, cables y parámetros.
- **Serialización JSON Nativa**: Formato compacto sin dependencias externas pesadas para guardar y cargar configuraciones completas de grafos (`.n8preset`).
- **Presets de Fábrica Integrados**:
  1. *Ambient Cloud* (Nube granular con reverb espacial y retardo ping-pong)
  2. *Glitch Beat Slicer* (Manipulación rítmica y repeticiones estocásticas)
  3. *OTT Mastering Chain* (Compresión multibanda descendente/ascendente de 3 bandas)
  4. *Resonant Shimmer* (Banco de resonadores modales y pitch shifter)
  5. *Dub Tape Echo* (Saturación de cinta, retardo cruzado y modulación wow/flutter)
  6. *Spectral Drone* (Congelamiento espectral STFT y paneo binaural 3D)

---

## 4. Estructura del Código Fuente

```text
source/
├── analysis/           # Detección de transitorios, pitch YIN y extractor espectral
├── core/               # Tipos numéricos estables, pools lock-free y CpuProfiler
├── dsp/
│   ├── core/           # Primitivas matemáticas, FastMath, Biquad, DelayLine, FFT, Panner
│   └── processors/     # Implementaciones de los 23 nodos procesadores de audio
├── event/              # Motor de eventos acústicos soberanos, EventPool y AudioFragment
├── graph/              # DAG Graph, ExecutionPlan, ContainerNode y NodeFactory
├── gui/                # GraphCanvasComponent, NodePalette, PresetBar, PerformanceHUD
├── modulation/         # LFO, EnvelopeGenerator, StepSequencer, AudioFollower, Matrix
├── plugin/             # DualWorldEngine, PluginProcessor y PluginEditor (JUCE)
└── preset/             # GraphSerializer JSON, SceneManager y GraphUndoManager
```

---

## 5. Compilación y Ejecución de Pruebas

### Prerrequisitos
- Compilador compatible con **C++20** (Visual Studio 2022 v17+ / MSVC, GCC 11+ o Clang 13+).
- **CMake 3.22** o superior.
- *Nota arquitectónica*: La dependencia de JUCE 8 se resuelve automáticamente descargando el archivo ZIP oficial sin necesidad de herramientas Git externas en el pipeline local.

### Ejecución de la Suite de Pruebas Automatizadas
```powershell
# Compilación del target de pruebas
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target N8Effect_Tests

# Ejecución de las 54 pruebas unitarias, de integración y estrés
.\build\Release\N8Effect_Tests.exe
```

---

## 6. Telemetría y Rendimiento

- **Tiempo de Procesamiento DSP**: ~180 µs para un bloque de 512 muestras con un grafo masivo de 30 nodos activos (menos del 2.0% del presupuesto de 10,666 µs a 48 kHz).
- **Alocaciones en Audio Thread**: **0 bytes** durante `processBlock`.
- **Protección de CPU**: HUD en vivo con Peak Hold, contador de microsegundos y botón reset de sobrecarga.
