# Original User Request

## Initial Request — 2026-09-25T21:57:21Z

# Teamwork Project: N8Effect Final Suite — Arturia Killer (Shimmer Reverb, Refraction, Spectral Smear, Grain Cloud Visualizer & Presets)

Implementar y certificar las funcionalidades clave necesarias para lograr paridad y superioridad sonora absoluta frente a la tríada creativa de Arturia (Efx FRAGMENTS, Efx MOTIONS y Efx REFRACT / Ambience), integrando Shimmer Reverb, Refracción Polifónica, Spectral Smear, Visualización interactiva de Nube Granular y Presets insignia.

Working directory: d:/Proyectos/TEST/N8Effect
Integrity mode: development

## Protocolo Multi-Agente Requerido
1. **Auditor de Similitud**: Revisar código base existente en d:/Proyectos/TEST/N8Effect/source/.
2. **Propositor**: Formular la propuesta técnica y cómo encaja en las capas (Regla 45).
3. **Validador de Aporte**: Validar solidez y aporte musical/técnico.
4. **Implementador**: Codificar en C++20 cumpliendo estrictamente con Regla 0 (NO Git), Regla 9 y 47 (Zero Allocations en audio thread, memoria bounded, FTZ/DAZ), Reglas 48 y 49.
5. **Validador de Implementación / Audio Safety**: Inspección de código para verificar ausencia de fugas, denormales, nan/inf y bloqueos.
6. **Validador de Pruebas**: Crear pruebas unitarias en tests/TestMain.cpp, verificar que pasen al 100% en N8Effect_Tests.exe y verificar que N8Effect_Standalone.exe y Audio Event Graph Engine.vst3 compilen limpiamente en Release. Copiar el VST3 en C:\Users\kevin.garrido\AppData\Local\Programs\Common\VST3\.

## Requirements

### R1. Shimmer Reverb Engine (ShimmerReverbNode)
Implementar un procesador de reverberación shimmer algorítmica de alta densidad con lazo de realimentación armónico transponible (+12st octava arriba, +7st quinta justa, -12st sub-octava), etapas de difusión allpass, filtro de amortiguamiento (damping), control de decaimiento (0.2s a 30s) y mezcla de shimmer. Cumplimiento estricto de Regla 9 (cero alocaciones en audio thread) y Regla 47 (anti-denormal y límites de feedback seguros).

### R2. Polyphonic Refraction Dispersion Engine (RefractionNode)
Implementar un motor de refracción y dispersión unísono estéreo (al estilo de Arturia Efx Refract) que divide la señal en 2 a 8 voces refractadas polifónicas con micro-desafinación (detune), retardo escalonado (delay offset), apertura estereofónica (stereo spread) y dispersión armónica resonante. Cero alocaciones dinámicas y procesamiento eficiente en tiempo real.

### R3. Spectral Smear & Phase Diffusion Engine (SpectralSmearNode)
Implementar un procesador espectral de difusión líquida y emborronamiento tímbrico basado en STFT FFT que aleatoriza progresivamente las fases de los bins espectrales y aplica persistencia de magnitud temporal (blur) para convertir cualquier fuente en pads y paisajes sonoros etéreos infinitos.

### R4. Visualizador de Nube Granular Activa en GUI (GrainCloudVisualizer)
Incorporar en la interfaz gráfica (AudioVisualizerComponent y GranularEditorComponent) un modo de visualización en tiempo real de partículas de nube granular que dibuje los granos activos del GrainPool (posición de reproducción, tamaño, color según tono, paneo espacial y envolvente) sincronizado mediante telemetría lock-free y respetando las Reglas 48 (GUI Interaction Guard) y 49 (Continuous Sweep Invariant).

### R5. Expansión de Presets Insignia "Arturia Killer"
Diseñar e integrar 10 presets creativos de alta gama en el catálogo de presets de fábrica, cubriendo explícitamente:
- Granular Clouds & Glitch Textures (Efx Fragments)
- Rhythmic Slicing & Automated Motion Chains (Efx Motions)
- Ethereal Shimmer & Refracted Ambient Drones (Efx Refract / Ambience)

## Acceptance Criteria

### Verificación Programática y de Audio en Tiempo Real
- [ ] Compilación limpia con 0 errores en Release para N8Effect_Tests, N8Effect_Standalone y N8Effect_VST3.
- [ ] Incorporación de pruebas unitarias para ShimmerReverbNode, RefractionNode y SpectralSmearNode en tests/TestMain.cpp.
- [ ] El 100% de la suite de pruebas unitarias (115+ pruebas) pasa exitosamente sin fallos ni excepciones.
- [ ] Cero alocaciones dinámicas (malloc, new, std::vector::resize, std::string) en los métodos process() de los nuevos nodos.
- [ ] Ausencia total de anomalías NaN e Inf bajo silencios, impulsos y saturaciones.
- [ ] Despliegue automático del plugin binario .vst3 actualizado en la carpeta de plugins VST3 del sistema (C:\Users\kevin.garrido\AppData\Local\Programs\Common\VST3\).

## Follow-up — 2026-09-26T02:57:16Z

Rediseño estético y visual de alta gama para las 8 perillas maestras de Macro en el dock inferior (`MacroDashboardComponent`), transformándolas en diales de precisión de inspiración **Minimalista Suizo / Teenage Engineering (OP-1 / TX-6 / Arturia Pigments)**, perfectamente integradas al chasis oscuro y reactivas al sistema de temas de N8Effect.

Working directory: `F:\Dev\N8Effect`
Integrity mode: development

---

## Critical Workspace Constraints (Strictly Enforced)
- **Rule 0 (No Git):** DO NOT run any `git` commands (`git init`, `git commit`, `git add`, `git status`, etc.). Syncing to `D:\Proyectos\TEST\N8Effect` must be done strictly via PowerShell `Copy-Item`.
- **Rules 9 & 47 (Real-Time Audio Safety):** Zero dynamic allocations (`new`, `malloc`, `vector::resize`), zero locks (`std::mutex`), and zero disk I/O in the audio thread.
- **Rule 48 (GUI Drag vs Click):** Strict disambiguation of mouse drag vs click on all controls (`MacroDragPin`, `MacroKnob`).
- **Rule 46 (Mandatory Verification):** All existing and new unit tests must compile and pass cleanly on `N8Effect_Tests.exe` (100% pass rate).

---

## Requirements

### R1. Rediseño Visual de los Diales Macro (MacroKnob — Minimalista Suizo / Teenage Engineering)
- **Chasis y Dial:** Rotor circular plano con acabado mate oscuro aeroespacial, bisel sutil y proporción balanceada dentro del slot de 42px de altura.
- **Graduación de Precisión:** Micro-ticks o marcas discretas perimetrales calibradas a intervalos regulares de valor (cada 10% / 25%).
- **Arco Activo Fino:** Línea de arco perimetral de alta definición (1.8px) con terminaciones suaves y resplandor sutil según el valor actual (0% - 100%).
- **Aguja / Puntero de Precisión:** Puntero afilado de alta nitidez que indica la rotación exacta con contraste visual perfecto.
- **Tipografía Técnica Monotipo:** Rotulación limpia en tipografía técnica condensada para el título del macro (`TEXTURE`, `MOTION`, etc.) y lectura numérica (`0%` - `100%`) con diseño ordenado.

### R2. Integración Dinámica y Reactiva con ThemeManager
- Suscribir a `MacroDashboardComponent` como `ThemeManager::Listener`.
- Vincular dinámicamente los colores de fondo del dock, divisores de slot, color de rotor, marcas de escala, arcos de valor y tipografía a los tokens oficiales de `ThemeColors`:
  - `Cyberpunk`: Acento Cyan / Magenta neón con base OLED negro azabache.
  - `VintageConsole`: Acento Ámbar / Bronce cálido con base caoba oscura.
  - `CleanStudio`: Acento Blanco polar / Grafito con líneas plateadas de alta definición.
  - `PhosphorCRT`: Acento Fósforo verde con fondo CRT monocromático.
- Actualización inmediata en caliente al conmutar el tema sin parpadeos ni recarga.

### R3. Micro-Puerto de Parcheo Reestilizado (MacroDragPin)
- Rediseñar el puerto modular de modulación drag & drop como un conector jack aeroespacial integrado con corona metálica pulida, bisel cónico y micro-LED interior de estado.
- Mantener compatibilidad total con `ModulationDragPayload` y respetar estrictamente la **Regla 48** (umbral de arrastre > 4px, supresión total de clic al arrastrar).

---

## Acceptance Criteria

### Visual & Rendering
- [ ] Renderizado vectorial antialiased fluido a 60 FPS sin glitches ni artefactos en `MacroDashboardComponent`.
- [ ] Cambio instantáneo y coherente de la paleta de colores al alternar de tema visual en el modal de configuración (`EngineConfigModalComponent`).
- [ ] Los 8 macros globales mantienen su funcionalidad de arrastre de modulación (`MacroDragPin`) hacia los nodos del grafo y clic rápido para mapeo contextual.

### Verificación Técnica
- [ ] Compilación limpia con 0 errores en `N8Effect_Tests`, `N8Effect_Standalone` y `N8Effect_VST3`.
- [ ] 100% de la suite de pruebas unitarias (`N8Effect_Tests.exe`, 115/115 tests) pasando sin fallos ni excepciones.
- [ ] Cambios sincronizados de forma segura a `D:\Proyectos\TEST\N8Effect` sin uso de Git.
