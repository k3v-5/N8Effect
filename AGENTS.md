# AUDIO ENGINE MASTER RULES & WORKSPACE GUIDELINES

## 0. Restricción del Entorno: No Git
- Este proyecto NO debe inicializarse como repositorio Git (`git init`), ni usar submódulos, ni ejecutar comandos `git`.
- Toda dependencia externa (como JUCE) se obtiene mediante descarga de archivo ZIP (`FetchContent` con URL de archivo comprimido o descompresión local), manteniendo el espacio de trabajo completamente limpio y libre de Git.

---

## 1. Arquitectura
Este proyecto es un motor modular de procesamiento de audio/eventos.
NO debe implementarse como una cadena fija de efectos.

La arquitectura principal debe mantenerse separada en:
- Dry Signal
- Analysis Engine
- Event Engine
- Graph Engine
- DSP Processor Nodes
- Universal Modulation Engine
- Macro/Scene System
- Preset System
- GUI

Nunca mezclar estas responsabilidades sin una razón arquitectónica explícita.
El Dry Signal debe permanecer completamente independiente del Wet/Event World.
Por defecto: DRY = audio original sin procesamiento.
Los efectos generados por el sistema deben vivir en el Wet/Event World.
No modificar el Dry Signal salvo que exista un nodo explícito que el usuario haya colocado en el grafo.

---

## 2. Event Engine
Los eventos de audio son entidades independientes.
Un evento puede: nacer, reproducirse, modificarse, duplicarse, dividirse, generar nuevos eventos, entrar en diferentes rutas, recibir efectos, generar tails, morir, ser cortado inmediatamente, continuar después de que desaparezca la fuente.
Los eventos NO deben asumir que su duración coincide con la duración de la fuente original.
Cada evento debe tener un ciclo de vida explícito:
`CREATED -> PLAYING -> RELEASING -> FROZEN -> LOOPING -> KILLED -> DESTROYED`
No usar lógica implícita de vida de eventos.

---

## 3. Source Following
Cada evento puede tener independencia respecto a la fuente:
- `sourceFollow`: 0.0 = completamente independiente de la fuente, 1.0 = completamente dependiente de la fuente.
- Comportamiento configurable al desaparecer la fuente: `CUT`, `SHORT`, `FADE`, `NATURAL`, `HOLD`, `FREEZE`.
- No asumir que un evento siempre debe continuar después de que termina la fuente.

---

## 4. Graph Engine
El procesamiento debe estar basado en un grafo modular acíclico/controlado.
No crear cadenas hardcoded como `Input -> Delay -> Reverb -> Output`.
Los procesadores deben ser nodos independientes conectados explícitamente.
El sistema debe soportar conceptualmente:
`SERIAL`, `PARALLEL`, `SPLIT`, `MERGE`, `SEND`, `RETURN`, `FEEDBACK`, `MULTIBAND`, `EVENT ROUTING`, `CONTAINERS`.
El orden de procesamiento viene determinado por el grafo y no por el orden arbitrario de clases en `PluginProcessor`.

---

## 5. Processor Nodes
Todos los efectos deben implementar una interfaz común (`AudioProcessorNode`).
Ningún efecto debe tener privilegios arquitectónicos especiales salvo que sea técnicamente necesario.
Un nuevo efecto debe poder añadirse como un nuevo `ProcessorNode` sin modificar el núcleo del Graph Engine.

---

## 6. Containers
Los containers también son nodos (`Serial`, `Parallel`, `Multiband`, `Feedback`, `Event Container`).
Pueden anidar otros nodos y containers. No duplicar lógica DSP innecesariamente.

---

## 7. Universal Modulation
Todos los parámetros modulables deben utilizar el mismo sistema centralizado de modulación.
Fuentes: LFO, Envelope, Audio Follower, Step Sequencer, Random, MIDI, Velocity, Note, Macro, XY, Scene, Host Automation, Event Energy, Event Pitch, Event Spectral Data.
No crear sistemas de modulación independientes para cada efecto.
La modulación debe pasar por un sistema centralizado.

---

## 8. Parameter System
Todos los parámetros deben tener IDs únicos y estables (sin strings mágicos dispersos).
Registrados en un sistema central. Los DSP no deben depender directamente de componentes de GUI.
La GUI nunca modifica directamente variables internas del DSP: `GUI -> Parameter System -> DSP`.

---

## 9. Real-Time Audio Safety (CRÍTICO)
Dentro de `processBlock()` y cualquier función ejecutada por el audio thread:
- **NO allocations dinámicas** (NO `malloc`, `new`, `free`, `delete`, `std::vector::resize`, `std::string`).
- **NO locks** (NO `std::mutex`, `std::lock_guard`, semáforos ni primitivas bloqueantes).
- **NO acceso al sistema de archivos / I/O**.
- **NO logging pesado** ni llamadas impredecibles.
- Utilizar pools prealocados, buffers de capacidad fija y colas lock-free SPSC.
- Todo recurso necesario para DSP debe prepararse durante `prepareToPlay()`.

---

## 10. Event Pools
Eventos, granos, voces y buffers temporales deben residir en pools prealocados reutilizables (`EventPool`, `GrainPool`, `AudioBufferPool`).
Al morir un evento o grano, retorna inmediatamente al pool.

---

## 11. CPU Protection
Límites de seguridad estrictos: `MAX_EVENTS`, `MAX_GRAINS`, `MAX_GENERATIONS`, `MAX_FEEDBACK`, `MAX_EVENT_LIFETIME`, `MAX_PROCESSING_DEPTH`.
Nunca permitir feedback infinito ni multiplicación exponencial descontrolada de eventos.
Cuando se alcance un límite, el sistema debe fallar de manera controlada.

---

## 12. Feedback
Los feedback loops son válidos pero deben estar rigurosamente controlados.
Límites configurables y protección contra runaway feedback.
Nunca permitir que un feedback loop cree eventos infinitamente.

---

## 13. DSP Independence
Separación clara entre capas:
`DSP Algorithms` -> `Audio Engine` -> `Parameter System` -> `Plugin Layer` -> `GUI`.
Algoritmos DSP independientes de JUCE/GUI cuando sea razonable.

---

## 14. Sample Rate
Soportar dinámicamente cualquier sample rate del host (44.1, 48, 88.2, 96, 176.4, 192 kHz).
Nunca asumir valores fijos; recalcular coeficientes y buffers en `prepare()`.

---

## 15. Block Size
Nunca asumir un block size fijo. Soportar tamaños de buffer variables proporcionados por el host.

---

## 16. Stereo / Channel Layout
Validar el número de canales. Definir explícitamente manejo Mono, Stereo, Mid/Side, Left/Right y multicanal cuando corresponda.

---

## 17. Dry/Wet
Dry y Wet deben permanecer conceptualmente separados.
Debe existir un Wet World independiente. El Dry debe poder permanecer 100% limpio.
El procesamiento Wet jamás debe sobreescribir o corromper accidentalmente el buffer Dry.

---

## 18. Tails
Efectos con tails independientes (Delay, Reverb, Granular, Feedback).
La desaparición de la fuente no elimina los tails a menos que la política del evento/nodo así lo dicte (`CUT`, `FADE`, `NATURAL`, etc.).

---

## 19. No Hardcodear el Catálogo
Diseñado para crecer sin rediseñar el núcleo. Usar `ProcessorRegistry` donde cada procesador declara: ID, Name, Category, Parameters, Capabilities, Factory.

---

## 20. Processor Factory
Los procesadores deben crearse vía registro (`createProcessor("type")`). Sin cadenas masivas de `if/else` o `switch`.

---

## 21. Presets
Guardar Graph, Nodes, Connections, Parameters, Modulation, Macros, Scenes, Version.
Versionado estricto con rutinas de migración.

---

## 22. Graph Serialization
Serialización/deserialización del grafo independiente de la GUI: Save, Load, Clone, Undo, Redo.

---

## 23. GUI
La GUI visualiza el estado del motor; NO es la fuente de verdad.
Debe poder destruirse y reconstruirse sin perder el estado del audio engine.

---

## 24. GUI Graph Editor
Representación fidedigna de nodos, puertos, conexiones y parámetros reales. Sin conexiones cosméticas falsas.

---

## 25. Modulación Visual
Mostrar Base Value, Modulation Amount y Current Modulated Value. Host automation y modulación interna separadas conceptualmente.

---

## 26. Threading
Separación estricta: Audio Thread, GUI Thread, Background Thread.
Sin estructuras mutables compartidas sin sincronización lock-free. Audio thread jamás bloqueado esperando a la GUI.

---

## 27. Background Tasks
Operaciones pesadas (presets, IRs, análisis, compilación de grafo) ejecutadas fuera del audio thread.

---

## 28 & 29. Graph Compilation & Validation
El audio thread ejecuta un `Runtime Graph` optimizado.
Validación previa estricta: conexiones inválidas, ciclos no permitidos, feedback descontrolado, puertos incompatibles, nodos inexistentes, parámetros inválidos, profundidad excesiva.
Un grafo inválido jamás llega al audio thread.

---

## 30. No Destruir el Runtime Graph Directamente (Double Buffering)
Nunca mutar destructivamente el grafo en ejecución en el audio thread.
Preparar nuevo runtime graph -> Validar -> Compilar -> Reemplazo atómico / safe swap cuando esté listo.

---

## 31. Error Handling
No romper silenciosamente partes del sistema. No eliminar funcionalidad para parchar un bug sin documentarlo.

---

## 32 & 33. Reusabilidad DSP y Efectos Complejos
No duplicar motores base (Delay core, Grain engine, FFT infrastructure).
Efectos complejos (OTT, multibanda, mastering chains) construidos componiendo módulos y containers reutilizables.

---

## 34. Calidad de Audio
Prioridad:
`Arquitectura -> Correctitud -> Estabilidad -> Audio Safety -> Calidad DSP -> Rendimiento CPU -> GUI -> Features`.

---

## 35. Anti-Click
Smoothing obligatorio en cambios de ganancia, mix, pan, feedback, delay time, pitch, filter y ruteo para evitar discontinuidades audibles.

---

## 36 & 37. Host Automation & Sync
Parámetros automatizables con precedencia clara. Módulos rítmicos sincronizados estrictamente al tempo, PPQ y transporte del host DAW.

---

## 38 & 39. Testing & Performance
Pruebas obligatorias por DSP: silencio, impulso, estabilidad, protección NaN/Inf, sample rates, block sizes, mono/stereo, reset.
Monitoreo de costo de CPU, uso de memoria y conteo de eventos/granos.

---

## 40. Documentación
Cada módulo documenta: propósito, I/O, parámetros, DSP behavior, requerimientos de thread, latencia y consideraciones de CPU.

---

## 41, 42 & 43. Filosofía y Regla de Oro
- No implementar atajos que comprometan la arquitectura a largo plazo.
- El sistema es un motor modular de procesamiento de audio y eventos diseñado para expandirse indefinidamente sin reescribir su núcleo.

---

## 44. Fases de Desarrollo del Motor (Roadmap Oficial)

### PHASE 1 — Core
- Parameter System
- Audio Thread
- Graph
- Nodes
- Connections
- Serialization

### PHASE 2 — Event Engine
- Event
- EventPool
- Event Lifecycle
- Fragment
- Tails

### PHASE 3 — Modulation
- LFO
- Envelope
- Sequencer
- Random
- Macro
- Matrix

### PHASE 4 — DSP Foundation
- Delay
- Filter
- Dynamics
- Distortion
- Reverb
- Pitch
- FFT

### PHASE 5 — Advanced DSP
- Granular
- Spectral
- Resonators
- Glitch
- Advanced Dynamics

### PHASE 6 — Graph Editor

### PHASE 7 — Preset / Scene / Morph

### PHASE 8 — Optimization

### PHASE 9 — Large Effect Library

---

## 45. Regla Obligatoria de Asignación de Capa Arquitectónica (STOP & PROPOSE)
Antes de implementar una nueva funcionalidad:
1. **Determinar la capa**: Identificar a qué capa arquitectónica (Core, Event, Modulation, DSP, Editor, Preset, etc.) pertenece la funcionalidad.
2. **Si no encaja limpiamente**: Si una funcionalidad o requerimiento no puede ubicarse con claridad en una capa existente, **DETENERSE INMEDIATAMENTE y proponer una modificación arquitectónica** antes de escribir código.
3. No forzar componentes en capas inapropiadas ni saltarse la jerarquía de fases.

---

## 46. Estándares Obligatorios de Ingeniería de Software (SOLID, Modularidad, Tests y Escalabilidad)

### 1. Principios SOLID
- **Single Responsibility (SRP)**: Cada clase, archivo y módulo debe tener una única responsabilidad bien definida (separar DSP, gestión de eventos, grafo, UI y parámetros).
- **Open/Closed (OCP)**: El motor debe estar abierto a la extensión (nuevos efectos, generadores, tipos de eventos vía factories y registros) pero cerrado a la modificación de su núcleo.
- **Liskov Substitution (LSP)**: Cualquier clase derivada (ej. implementaciones de `AudioProcessorNode` o `Event`) debe ser completamente intercambiable sin alterar el comportamiento esperado del runtime.
- **Interface Segregation (ISP)**: Interfaces limpias y especializadas. Ningún módulo debe verse forzado a implementar métodos irrelevantes para su función.
- **Dependency Inversion (DIP)**: Los módulos de alto nivel (como `GraphExecutor` o `DualWorldEngine`) dependen de abstracciones e interfaces (`AudioProcessorNode`, `ProcessContext`), nunca de implementaciones concretas de efectos.

### 2. Modularidad y Reusabilidad
- Prohibida la duplicación de lógica DSP o estructuras de control.
- Algoritmos base (líneas de delay, generadores de envolventes, FFT, osciladores) deben implementarse como componentes modulares reutilizables.

### 3. Validación y Pruebas Obligatorias (Testing Mandatorio)
- **Todo código nuevo debe ser validado**: Ninguna funcionalidad se considera completada sin haber sido verificada mediante compilación y ejecución de tests.
- **Suite de Pruebas Unitaria (`N8Effect_Tests`)**: Cada nuevo componente debe acompañarse de pruebas unitarias que cubran:
  - Casos límite y estabilidad numérica (ausencia de NaN/Inf).
  - Comportamiento bajo reseteo y cambios de sample rate / buffer size.
  - Cero allocations en el hilo de audio y ausencia de memory leaks en pools.

### 4. Escalabilidad Permanente
- Todo desarrollo debe concebirse pensando en la escalabilidad a gran escala (soporte para cientos de nodos, miles de eventos, decenas de módulos de modulación sin pérdida de rendimiento ni cuellos de botella en CPU).

---

## 47. Optimización Rigurosa y Control Estricto del Presupuesto de Memoria (Zero Memory Abuse & CPU Efficiency)

### 1. Control Estricto del Presupuesto de Memoria (Anti-Memory Abuse)
- **Zero Allocations en Audio Thread (Regla 9)**: Prohibido cualquier `malloc`, `new`, `std::vector::resize`, `std::string` o realocación dinámica dentro del ciclo de procesamiento.
- **Topes Máximos Bounded (Límites Finitos y Predecibles)**: Todo pool (`AudioBufferPool`, `EventPool`, `GrainPool`), cola lock-free, línea de retardo o historial de undo debe tener una capacidad máxima explícitamente acotada en tiempo de diseño.
- **No Stack Overflow / Prohibidos Buffers Estáticos Masivos en Stack**: Nunca declarar buffers de audio gigantescos como miembros de clase por valor en el stack (ej. arrays locales de cientos de kilobytes que excedan el límite de stack del hilo del host de 1 MB en Windows). Los buffers de trabajo grandes deben residir en el heap prealocados de forma segura durante `prepareToPlay()`.
- **Dimensionamiento Racional según Sample Rate**: Las líneas de retardo, buffers de análisis y convolución deben dimensionarse en función de la duración máxima real necesaria y el sample rate del host, nunca asignando tamaños arbitrariamente desorbitados.
- **Liberación Inmediata y Reciclaje**: Todo recurso, grano, fragmento o evento que concluya su ciclo de vida debe regresar de inmediato a su pool respectivo sin fugas de memoria (*zero leaks*).

### 2. Eficiencia de CPU y Aceleración DSP
- **Erradicación de Números Denormales**: Aplicar invariablemente protección anti-denormal (DAZ - *Denormals Are Zero* y FTZ - *Flush To Zero*) en el hilo de audio para evitar penalizaciones de CPU de hasta 100x en procesadores x86/x64 bajo silencios o colas IIR/reverb.
- **Localidad de Caché y Memoria Contigua**: Diseñar estructuras de datos compactas, planas y con acceso secuencial para maximizar aciertos en caché L1/L2 y evitar el *pointer chasing*.
- **Aproximaciones Rápidas en Bucles Internos**: Evitar funciones matemáticas trascendentes costosas (`std::tanh`, `std::pow`, `std::exp`) dentro de bucles por muestra cuando existan aproximaciones analíticas o de Padé de alta precisión con costo computacional significativamente menor.
- **Reutilización de Buffers de Interconexión**: En el plan de ejecución del grafo (`ExecutionPlan`), minimizar la cantidad de buffers intermediarios reutilizando slots de memoria entre nodos que no procesen en paralelo.

---

## 48. Desambiguación Estricta de Drag & Drop vs Clic en la Interfaz (GUI Interaction Guard)

Todo componente de la interfaz de usuario (como botones de paletas, conectores o tarjetas modulares) que soporte simultáneamente interacción por **clic** y por **arrastre (Drag & Drop)** debe implementar una máquina de estados de arrastre explícita para evitar acciones fantasma o duplicadas:

1. **Rastreo de Arrastre (`hasDragged_` / `isDragging_`)**:
   - En `mouseDrag()`, si la distancia euclidiana respecto al punto de pulsación inicial excede el umbral de arrastre de JUCE (`isDragAndDropActive()` o distancia $> 4\text{ px}$), se debe marcar inmediatamente el estado de arrastre (`hasDragged_ = true`).
   - Durante el arrastre, debe omitirse la propagación a la implementación por defecto de la clase base `Button::mouseDrag(e)` para evitar que el botón registre internamente una interacción de clic válida.

2. **Supresión Absoluta de Clic en `mouseUp()` y `clicked()`**:
   - En el método `mouseUp()`, si `hasDragged_` está activo, se debe restaurar el estado visual a normal (`setState(buttonNormal)`), limpiar el flag y ejecutar un `return;` inmediato **sin llamar a `Button::mouseUp(e)`**.
   - Se deben sobreescribir explícitamente los métodos virtuales `clicked()` y `clicked(const juce::ModifierKeys&)` verificando si existió arrastre previo, abortando la ejecución de cualquier callback `onClick`.

3. **Invariante de Clic Simple Limpio**:
   - Si el usuario simplemente hace clic y suelta sin exceder el umbral de movimiento, la cadena de ejecución estándar de JUCE debe conservarse intacta para permitir interacciones rápidas por un solo clic.
