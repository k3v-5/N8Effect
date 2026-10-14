import json

presets = []

def add(name, cat, desc, dry, wet, macros, nodes, conns):
    data = {
        "schemaVersion": 1,
        "name": name,
        "author": "N8Audio",
        "category": cat,
        "description": desc,
        "dryLevel": dry,
        "wetLevel": wet,
        "macros": macros,
        "nodes": nodes,
        "connections": conns
    }
    json_str = json.dumps(data, indent=2)
    presets.append({
        "name": name,
        "category": cat,
        "json": json_str
    })

# 1. Dispersión & Texturas Cuánticas
add(
    "Quantum Chirp Diffusion Rig",
    "Dispersión & Texturas Cuánticas",
    "Cascada de 20 etapas de dispersión allpass con bifurcación paralela a pitch delay y trémolo dinámico",
    0.3, 0.95, [0.7, 0.5, 0.8, 0.4, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Transient Snap", "type": 33, "x": 40.0, "y": 80.0, "params": {"1": 0.7, "2": -0.2, "3": 1.2, "4": 1.0, "5": 2.0, "6": 1.0}},
        {"id": 2, "name": "Phase Dispersion", "type": 65, "x": 220.0, "y": 80.0, "params": {"1": 20.0, "2": 1800.0, "3": 0.85, "4": 12000.0, "5": 0.6, "6": 1.0}},
        {"id": 3, "name": "Pitch Delay", "type": 64, "x": 420.0, "y": 40.0, "params": {"1": 180.0, "2": 270.0, "3": 12.0, "4": 12.0, "5": 0.55, "6": 0.0, "7": 8000.0, "8": 0.7}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 420.0, "y": 140.0, "params": {"1": 4.5, "2": 0.8, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.6, "7": 0.85}},
        {"id": 5, "name": "Spectral Smear", "type": 52, "x": 640.0, "y": 80.0, "params": {"1": 4.0, "2": 0.85, "3": 0.9, "4": 0.0, "5": 0.8}},
        {"id": 6, "name": "Dimension Chorus", "type": 60, "x": 840.0, "y": 80.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Acoustic Ice Cracker",
    "Dispersión & Texturas Cuánticas",
    "Dispersión de chirps no lineales sobre fracturas de decimation y resonador comb con saturación",
    0.2, 0.9, [0.8, 0.6, 0.5, 0.7, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Bitcrusher Decimator", "type": 40, "x": 40.0, "y": 80.0, "params": {"1": 12.0, "2": 2.5, "3": 0.02, "4": 0.3, "5": 0.0, "6": 0.6}},
        {"id": 2, "name": "Phase Dispersion", "type": 65, "x": 240.0, "y": 80.0, "params": {"1": 24.0, "2": 2400.0, "3": 0.92, "4": 14000.0, "5": 0.75, "6": 1.0}},
        {"id": 3, "name": "Comb Resonator", "type": 63, "x": 440.0, "y": 30.0, "params": {"1": 440.0, "2": 0.88, "3": 9000.0, "4": 15.0, "5": 0.6}},
        {"id": 4, "name": "Multiband Saturator", "type": 62, "x": 440.0, "y": 140.0, "params": {"1": 1.8, "2": 1.0, "3": 2.2, "4": 2.0, "5": 1.6, "6": 1.0, "7": 200.0, "8": 3500.0, "9": 0.8}},
        {"id": 5, "name": "Space Delay", "type": 28, "x": 660.0, "y": 80.0, "params": {"1": 220.0, "2": 330.0, "3": 0.5, "4": 7500.0, "5": 1.0, "6": 0.6}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Laser Droplet Phantasm",
    "Dispersión & Texturas Cuánticas",
    "Slicing rítmico convertido en gotas de lluvia acústica y chirps láser con transposición espectral",
    0.4, 0.9, [0.75, 0.4, 0.85, 0.5, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Audio Slicer", "type": 49, "x": 40.0, "y": 80.0, "params": {"1": 0.0, "2": 8.0, "3": -1.0, "4": 1.0, "5": 0.0, "6": 50.0, "7": 4.0, "8": 0.35, "9": 0.0, "10": 14000.0, "11": 0.95}},
        {"id": 2, "name": "Laser Dispersion", "type": 65, "x": 240.0, "y": 80.0, "params": {"1": 18.0, "2": 3200.0, "3": 0.88, "4": 15000.0, "5": 0.7, "6": 1.0}},
        {"id": 3, "name": "Pitch Shifter +7st", "type": 11, "x": 440.0, "y": 30.0, "params": {"1": 7.0, "2": 0.0, "3": 35.0, "4": 0.8}},
        {"id": 4, "name": "Reverse Reverb Swell", "type": 38, "x": 440.0, "y": 140.0, "params": {"1": 1.5, "2": 0.8, "3": 6500.0, "4": 0.85}},
        {"id": 5, "name": "Mid/Side Decoder", "type": 25, "x": 660.0, "y": 80.0, "params": {"1": 1.5, "2": 120.0, "3": 0.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 25.0, "4": 1.5, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Subatomic Smear Continuum",
    "Dispersión & Texturas Cuánticas",
    "Cadena difusora de desfasamiento espectral FFT y dispersión allpass en peine holográfico",
    0.3, 0.95, [0.6, 0.7, 0.9, 0.5, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Spectral Smear", "type": 52, "x": 40.0, "y": 80.0, "params": {"1": 5.0, "2": 0.9, "3": 0.92, "4": -0.1, "5": 0.85}},
        {"id": 2, "name": "Phase Dispersion", "type": 65, "x": 240.0, "y": 80.0, "params": {"1": 22.0, "2": 950.0, "3": 0.75, "4": 10000.0, "5": 0.55, "6": 0.9}},
        {"id": 3, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 30.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 4, "name": "Bipolar Flanger", "type": 18, "x": 440.0, "y": 140.0, "params": {"1": 0.25, "2": 0.8, "3": 2.0, "4": -0.8, "5": 0.75}},
        {"id": 5, "name": "Space Delay", "type": 28, "x": 660.0, "y": 80.0, "params": {"1": 350.0, "2": 525.0, "3": 0.6, "4": 6500.0, "5": 1.0, "6": 0.55}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 35.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 2. Filtros Mutantes & Topología No Lineal
add(
    "Acid Vowel Metamorphosis",
    "Filtros Mutantes & Topología No Lineal",
    "SVF no lineal ZDF con barrido continuo de topología, modulación FM por modulador en anillo y saturador multibanda",
    0.35, 0.9, [0.8, 0.7, 0.5, 0.8, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Ring Modulator FM", "type": 19, "x": 40.0, "y": 140.0, "params": {"1": 180.0, "2": 0.0, "3": 0.2, "4": 0.9}},
        {"id": 2, "name": "Morphing SVF Acid", "type": 66, "x": 240.0, "y": 80.0, "params": {"1": 850.0, "2": 18.0, "3": 0.35, "4": 4.5, "5": 0.7}},
        {"id": 3, "name": "Multiband Saturator", "type": 62, "x": 440.0, "y": 40.0, "params": {"1": 2.2, "2": 1.0, "3": 3.0, "4": 2.0, "5": 1.5, "6": 1.0, "7": 220.0, "8": 3200.0, "9": 0.85}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 6.0, "2": 0.75, "3": 1.0, "4": 1.0, "5": 0.5, "6": 0.7, "7": 0.9}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 60.0, "2": 20.0, "3": 2.2, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 4},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Screaming Topology Wormhole",
    "Filtros Mutantes & Topología No Lineal",
    "Filtro auto-oscilante en lazo de retroalimentación con corrimiento de frecuencia Hilbert y eco ping-pong",
    0.25, 0.95, [0.85, 0.75, 0.6, 0.7, 0.8, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Morphing SVF Screamer", "type": 66, "x": 40.0, "y": 80.0, "params": {"1": 1400.0, "2": 24.0, "3": 0.55, "4": 5.0, "5": 0.0}},
        {"id": 2, "name": "Freq Shifter", "type": 20, "x": 240.0, "y": 30.0, "params": {"1": 7.0, "2": 0.3, "3": 0.8}},
        {"id": 3, "name": "Space Ping-Pong", "type": 28, "x": 240.0, "y": 140.0, "params": {"1": 240.0, "2": 360.0, "3": 0.6, "4": 8000.0, "5": 1.0, "6": 0.65}},
        {"id": 4, "name": "Comb Resonator", "type": 63, "x": 480.0, "y": 80.0, "params": {"1": 330.0, "2": 0.88, "3": 6000.0, "4": 15.0, "5": 0.7}},
        {"id": 5, "name": "Dimension Chorus", "type": 60, "x": 680.0, "y": 80.0, "params": {"1": 2.0, "2": 0.6, "3": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 880.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 25.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Cybernetic Formant Biome",
    "Filtros Mutantes & Topología No Lineal",
    "Bifurcación en paralelo de formantes vocales humanos y resonancia mutante con compresión dinámica reactiva",
    0.3, 0.9, [0.6, 0.7, 0.75, 0.5, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Transient Snap", "type": 33, "x": 40.0, "y": 80.0, "params": {"1": 0.6, "2": -0.2, "3": 1.0, "4": 1.0, "5": 2.0, "6": 1.0}},
        {"id": 2, "name": "Formant Vowel", "type": 31, "x": 240.0, "y": 30.0, "params": {"1": 0.35, "2": 5.0, "3": 1.2, "4": 0.5, "5": 0.85}},
        {"id": 3, "name": "Morphing SVF Peak", "type": 66, "x": 240.0, "y": 140.0, "params": {"1": 1800.0, "2": 8.0, "3": 0.75, "4": 2.5, "5": 0.0}},
        {"id": 4, "name": "Dynamic EQ", "type": 54, "x": 460.0, "y": 80.0, "params": {"1": 2500.0, "2": 2.0, "3": 4.0, "4": -18.0, "5": 10.0, "6": 100.0, "7": 0.8}},
        {"id": 5, "name": "Dimension Chorus", "type": 60, "x": 660.0, "y": 80.0, "params": {"1": 3.0, "2": 0.5, "3": 0.8}},
        {"id": 6, "name": "Space Reverb", "type": 6, "x": 860.0, "y": 80.0, "params": {"1": 0.85, "2": 4.0, "3": 6000.0, "4": 25.0, "5": 0.5}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Nonlinear Diode Quake",
    "Filtros Mutantes & Topología No Lineal",
    "Saturación de 3 bandas empujando a un SVF no lineal en self-oscillation con peine sintonizado",
    0.3, 0.9, [0.75, 0.6, 0.7, 0.8, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Multiband Saturator", "type": 62, "x": 40.0, "y": 80.0, "params": {"1": 3.0, "2": 1.0, "3": 2.5, "4": 2.0, "5": 2.0, "6": 1.0, "7": 180.0, "8": 3000.0, "9": 0.85}},
        {"id": 2, "name": "Morphing SVF Drive", "type": 66, "x": 240.0, "y": 80.0, "params": {"1": 650.0, "2": 12.0, "3": 0.15, "4": 6.5, "5": 0.0}},
        {"id": 3, "name": "Comb Resonator", "type": 63, "x": 440.0, "y": 30.0, "params": {"1": 220.0, "2": 0.85, "3": 7000.0, "4": 12.0, "5": 0.7}},
        {"id": 4, "name": "Tape Saturation", "type": 21, "x": 440.0, "y": 140.0, "params": {"1": 3.5, "2": 1.0, "3": 0.4, "4": 0.75, "5": 0.8}},
        {"id": 5, "name": "Dynamic Tremolo", "type": 61, "x": 660.0, "y": 80.0, "params": {"1": 5.0, "2": 0.7, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.6, "7": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 3. Placas Fractales & Resonadores Áureos
add(
    "Hyperbolic Crystal Cathedral",
    "Placas Fractales & Resonadores Áureos",
    "Matriz física de 6 líneas con retardo áureo (phi^k) dispersada en prisma de refracción y shimmer coral",
    0.2, 0.95, [0.85, 0.5, 0.9, 0.6, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Fractal Resonator", "type": 67, "x": 40.0, "y": 80.0, "params": {"1": 329.63, "2": 0.618, "3": 0.92, "4": 8500.0, "5": 0.9, "6": 0.85}},
        {"id": 2, "name": "Shimmer Reverb", "type": 50, "x": 260.0, "y": 30.0, "params": {"1": 7.0, "2": 7500.0, "3": 0.7, "4": 0.0, "5": 1.5, "6": 0.75}},
        {"id": 3, "name": "8-Voice Refraction", "type": 51, "x": 260.0, "y": 140.0, "params": {"1": 8.0, "2": 24.0, "3": 25.0, "4": 0.9, "5": 0.8, "6": 2.5, "7": 0.85}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 500.0, "y": 80.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Mid/Side Decoder", "type": 25, "x": 700.0, "y": 80.0, "params": {"1": 1.6, "2": 110.0, "3": 0.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 900.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 35.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Tibetan Singing Bowl Continuum",
    "Placas Fractales & Resonadores Áureos",
    "Excitación armónica impulsando resonador fractal áureo con peine sintonizado y trémolo acústico",
    0.3, 0.9, [0.7, 0.6, 0.8, 0.4, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Harmonic Exciter", "type": 35, "x": 40.0, "y": 80.0, "params": {"1": 3500.0, "2": 0.7, "3": 0.5, "4": 0.85}},
        {"id": 2, "name": "Fractal Resonator", "type": 67, "x": 240.0, "y": 80.0, "params": {"1": 196.0, "2": 0.5, "3": 0.96, "4": 6000.0, "5": 0.85, "6": 0.9}},
        {"id": 3, "name": "Comb Resonator", "type": 63, "x": 440.0, "y": 30.0, "params": {"1": 392.0, "2": 0.82, "3": 8000.0, "4": 10.0, "5": 0.6}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 2.5, "2": 0.65, "3": 1.0, "4": 1.0, "5": 0.3, "6": 0.5, "7": 0.8}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 45.0, "2": 15.0, "3": 1.8, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Microtonal Glass Monochord",
    "Placas Fractales & Resonadores Áureos",
    "Cuerda física pulsada de Karplus-Strong excitando matriz fractal con retardos de cristal de cuarzo",
    0.25, 0.95, [0.8, 0.5, 0.85, 0.6, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Karplus Plucked", "type": 37, "x": 40.0, "y": 80.0, "params": {"1": 220.0, "2": 0.98, "3": 0.3, "4": 0.8, "5": 0.0, "6": 0.9}},
        {"id": 2, "name": "Fractal Resonator", "type": 67, "x": 240.0, "y": 80.0, "params": {"1": 440.0, "2": 0.618, "3": 0.91, "4": 10000.0, "5": 0.8, "6": 0.85}},
        {"id": 3, "name": "Pitch Delay Crystals", "type": 64, "x": 440.0, "y": 30.0, "params": {"1": 250.0, "2": 375.0, "3": 7.0, "4": 10.0, "5": 0.6, "6": 0.0, "7": 9000.0, "8": 0.75}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.5, "2": 15.0, "3": 120.0, "4": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 25.0, "4": 1.5, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Sympathetic String Matrix",
    "Placas Fractales & Resonadores Áureos",
    "Cadena acoplada de peines bipolares y matriz fractal áurea con apertura estéreo y reverberación",
    0.3, 0.9, [0.75, 0.5, 0.8, 0.5, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Transient Snap", "type": 33, "x": 40.0, "y": 80.0, "params": {"1": 0.7, "2": -0.3, "3": 1.2, "4": 1.0, "5": 2.0, "6": 1.0}},
        {"id": 2, "name": "Comb Resonator", "type": 63, "x": 240.0, "y": 80.0, "params": {"1": 130.81, "2": 0.9, "3": 7500.0, "4": 14.0, "5": 0.75}},
        {"id": 3, "name": "Fractal Resonator", "type": 67, "x": 440.0, "y": 30.0, "params": {"1": 261.63, "2": 0.7, "3": 0.89, "4": 8000.0, "5": 0.9, "6": 0.8}},
        {"id": 4, "name": "Tape Saturation", "type": 21, "x": 440.0, "y": 140.0, "params": {"1": 2.8, "2": 1.0, "3": 0.35, "4": 0.8, "5": 0.85}},
        {"id": 5, "name": "Space Reverb", "type": 6, "x": 660.0, "y": 80.0, "params": {"1": 0.9, "2": 5.5, "3": 5500.0, "4": 30.0, "5": 0.55}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 4. Desfase Espectral & Halos Líquidos
add(
    "Binaural Spectral Smear Halo",
    "Desfase Espectral & Halos Líquidos",
    "Procesamiento independiente Mid/Side: Centro saturado analógico con laterales en difusión espectral y chirps allpass",
    0.3, 0.95, [0.7, 0.8, 0.9, 0.5, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "M/S Encoder", "type": 24, "x": 40.0, "y": 80.0, "params": {"1": 1.0}},
        {"id": 2, "name": "Mid Saturator", "type": 62, "x": 240.0, "y": 30.0, "params": {"1": 2.5, "2": 1.0, "3": 2.0, "4": 1.5, "5": 1.2, "6": 1.0, "7": 250.0, "8": 3000.0, "9": 0.85}},
        {"id": 3, "name": "Side Spectral Smear", "type": 52, "x": 240.0, "y": 140.0, "params": {"1": 6.0, "2": 0.92, "3": 0.95, "4": -0.1, "5": 0.9}},
        {"id": 4, "name": "Side Phase Dispersion", "type": 65, "x": 440.0, "y": 140.0, "params": {"1": 18.0, "2": 1600.0, "3": 0.8, "4": 12000.0, "5": 0.5, "6": 0.95}},
        {"id": 5, "name": "M/S Decoder", "type": 25, "x": 660.0, "y": 80.0, "params": {"1": 1.6, "2": 100.0, "3": 0.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 2, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Infinite Frozen Aurora",
    "Desfase Espectral & Halos Líquidos",
    "Congelamiento espectral con barrido de filtro mutante SVF, transposición a la quinta y retardo estéreo",
    0.2, 0.95, [0.9, 0.4, 0.85, 0.6, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Spectral Freeze", "type": 12, "x": 40.0, "y": 80.0, "params": {"1": 1.0, "2": 0.9, "3": 0.95, "4": 0.85}},
        {"id": 2, "name": "Morphing SVF Sweep", "type": 66, "x": 240.0, "y": 80.0, "params": {"1": 1100.0, "2": 6.0, "3": 0.4, "4": 2.0, "5": 0.0}},
        {"id": 3, "name": "Pitch Shift +7st", "type": 11, "x": 440.0, "y": 30.0, "params": {"1": 7.0, "2": 0.0, "3": 35.0, "4": 0.75}},
        {"id": 4, "name": "Space Delay", "type": 28, "x": 440.0, "y": 140.0, "params": {"1": 320.0, "2": 480.0, "3": 0.55, "4": 6500.0, "5": 1.0, "6": 0.6}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.6, "2": 18.0, "3": 140.0, "4": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 25.0, "4": 1.5, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Liquid Phase Refractor",
    "Desfase Espectral & Halos Líquidos",
    "Prisma de refracción en 8 voces acoplado a difusión espectral y reverberación inversa 3D",
    0.3, 0.9, [0.8, 0.7, 0.85, 0.5, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Refraction Prism", "type": 51, "x": 40.0, "y": 80.0, "params": {"1": 8.0, "2": 22.0, "3": 25.0, "4": 0.9, "5": 0.75, "6": 2.2, "7": 0.85}},
        {"id": 2, "name": "Spectral Smear", "type": 52, "x": 240.0, "y": 80.0, "params": {"1": 4.5, "2": 0.88, "3": 0.9, "4": 0.0, "5": 0.8}},
        {"id": 3, "name": "Reverse Reverb Swell", "type": 38, "x": 440.0, "y": 30.0, "params": {"1": 1.8, "2": 0.85, "3": 7000.0, "4": 0.8}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": -45.0, "2": 20.0, "3": 2.0, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Spectral Crystal Shards",
    "Desfase Espectral & Halos Líquidos",
    "Filtrado espectral FFT con transposición armónica a la 19na y retardos reflectantes",
    0.35, 0.9, [0.85, 0.6, 0.8, 0.7, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Spectral Processor", "type": 29, "x": 40.0, "y": 80.0, "params": {"1": 3.0, "2": 0.6, "3": 12.0, "4": 0.85}},
        {"id": 2, "name": "Pitch Shift +19st", "type": 11, "x": 240.0, "y": 80.0, "params": {"1": 19.0, "2": 0.0, "3": 40.0, "4": 0.7}},
        {"id": 3, "name": "Shimmer Reverb", "type": 50, "x": 440.0, "y": 30.0, "params": {"1": 6.5, "2": 8000.0, "3": 0.65, "4": 0.0, "5": 1.4, "6": 0.7}},
        {"id": 4, "name": "Advanced Delay", "type": 28, "x": 440.0, "y": 140.0, "params": {"1": 180.0, "2": 270.0, "3": 0.5, "4": 7000.0, "5": 1.0, "6": 0.6}},
        {"id": 5, "name": "Phase Dispersion", "type": 65, "x": 660.0, "y": 80.0, "params": {"1": 16.0, "2": 2200.0, "3": 0.75, "4": 12000.0, "5": 0.45, "6": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 5. Micro-Muestreo Granular & Nubes de Partículas
add(
    "Stochastic Granular Prism",
    "Micro-Muestreo Granular",
    "Nube de granos estocásticos dividida en prisma de 8 voces y retardos de micro-pitch crystals",
    0.25, 0.95, [0.85, 0.6, 0.8, 0.5, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Granular Cloud", "type": 7, "x": 40.0, "y": 80.0, "params": {"1": 45.0, "2": 35.0, "3": 30.0, "4": 0.0, "5": 0.5, "6": 0.85, "7": 0.9, "10": 1.0}},
        {"id": 2, "name": "Refraction Prism", "type": 51, "x": 260.0, "y": 30.0, "params": {"1": 8.0, "2": 18.0, "3": 20.0, "4": 0.9, "5": 0.7, "6": 2.0, "7": 0.8}},
        {"id": 3, "name": "Pitch Delay Crystals", "type": 64, "x": 260.0, "y": 140.0, "params": {"1": 150.0, "2": 225.0, "3": 12.0, "4": 15.0, "5": 0.5, "6": 0.0, "7": 7500.0, "8": 0.7}},
        {"id": 4, "name": "Comb Resonator", "type": 63, "x": 500.0, "y": 80.0, "params": {"1": 220.0, "2": 0.82, "3": 8500.0, "4": 12.0, "5": 0.65}},
        {"id": 5, "name": "Mid/Side Decoder", "type": 25, "x": 720.0, "y": 80.0, "params": {"1": 1.5, "2": 110.0, "3": 0.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 920.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Reverse Grain Vortex",
    "Micro-Muestreo Granular",
    "Granulación en tiempo inverso con lazo acoplado a saturación multibanda y chorus dimensional",
    0.3, 0.9, [0.8, 0.5, 0.85, 0.7, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Reverse Reverb", "type": 38, "x": 40.0, "y": 80.0, "params": {"1": 1.4, "2": 0.8, "3": 6500.0, "4": 0.85}},
        {"id": 2, "name": "Reverse Granular", "type": 7, "x": 240.0, "y": 80.0, "params": {"1": 80.0, "2": 25.0, "3": 50.0, "4": -12.0, "5": 0.0, "6": 0.8, "7": 0.85, "10": 1.0}},
        {"id": 3, "name": "Multiband Saturator", "type": 62, "x": 440.0, "y": 30.0, "params": {"1": 2.0, "2": 1.0, "3": 2.5, "4": 2.0, "5": 1.6, "6": 1.0, "7": 220.0, "8": 3400.0, "9": 0.8}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Space Delay", "type": 28, "x": 660.0, "y": 80.0, "params": {"1": 240.0, "2": 360.0, "3": 0.5, "4": 6500.0, "5": 1.0, "6": 0.6}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Crystalline Shimmer Swarm",
    "Micro-Muestreo Granular",
    "Micro-partículas granulares dispersadas en cascada allpass y shimmer de octava ascendente",
    0.2, 0.95, [0.85, 0.6, 0.9, 0.5, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Granular Cloud", "type": 7, "x": 40.0, "y": 80.0, "params": {"1": 20.0, "2": 60.0, "3": 35.0, "4": 0.0, "5": 1.2, "6": 0.9, "7": 0.9, "10": 2.0}},
        {"id": 2, "name": "Shimmer Reverb", "type": 50, "x": 240.0, "y": 80.0, "params": {"1": 8.0, "2": 8000.0, "3": 0.7, "4": 0.0, "5": 1.6, "6": 0.75}},
        {"id": 3, "name": "Phase Dispersion", "type": 65, "x": 440.0, "y": 30.0, "params": {"1": 18.0, "2": 2000.0, "3": 0.8, "4": 12000.0, "5": 0.5, "6": 0.9}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 3.5, "2": 0.7, "3": 1.0, "4": 1.0, "5": 0.35, "6": 0.6, "7": 0.85}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 45.0, "2": 15.0, "3": 2.0, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Sub-Atomic Texture Cloud",
    "Micro-Muestreo Granular",
    "Textura micro-granular filtrada por formantes vocales con difusión espectral líquida",
    0.3, 0.9, [0.75, 0.5, 0.85, 0.6, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Granular Texture", "type": 7, "x": 40.0, "y": 80.0, "params": {"1": 15.0, "2": 55.0, "3": 40.0, "4": 0.0, "5": 1.0, "6": 0.85, "7": 0.9, "10": 2.0}},
        {"id": 2, "name": "Formant Filter", "type": 31, "x": 240.0, "y": 80.0, "params": {"1": 0.45, "2": 4.5, "3": 1.1, "4": 0.4, "5": 0.8}},
        {"id": 3, "name": "Spectral Smear", "type": 52, "x": 440.0, "y": 30.0, "params": {"1": 4.0, "2": 0.88, "3": 0.9, "4": -0.15, "5": 0.85}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.55, "3": 0.8}},
        {"id": 5, "name": "Space Delay", "type": 28, "x": 660.0, "y": 80.0, "params": {"1": 260.0, "2": 390.0, "3": 0.5, "4": 6500.0, "5": 1.0, "6": 0.6}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 6. Glitch, Deconstrucción & Caos Rítmico
add(
    "Neural Buffer Fracture",
    "Glitch & Deconstrucción Rítmica",
    "Slicing rítmico estocástico con división a bitcrusher y frenado de cinta procesado por OTT",
    0.35, 0.9, [0.8, 0.7, 0.5, 0.8, 0.7, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Audio Slicer", "type": 49, "x": 40.0, "y": 80.0, "params": {"1": 0.0, "2": 16.0, "3": -1.0, "4": 1.0, "5": 0.0, "6": 40.0, "7": 6.0, "8": 0.4, "9": 0.0, "10": 16000.0, "11": 1.0}},
        {"id": 2, "name": "Glitch Slicer", "type": 14, "x": 240.0, "y": 80.0, "params": {"1": 2.0, "2": 0.5, "3": 0.4, "4": 0.8, "5": 0.85}},
        {"id": 3, "name": "Bitcrusher Drive", "type": 40, "x": 440.0, "y": 30.0, "params": {"1": 10.0, "2": 3.0, "3": 0.05, "4": 0.4, "5": 0.0, "6": 0.75}},
        {"id": 4, "name": "Tape Stop Brake", "type": 30, "x": 440.0, "y": 140.0, "params": {"1": 0.0, "2": 350.0, "3": 0.75, "4": 0.6}},
        {"id": 5, "name": "Multiband OTT", "type": 15, "x": 660.0, "y": 80.0, "params": {"1": 180.0, "2": 3000.0, "3": 1.4, "4": 1.2, "5": 1.3, "6": 0.9}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Polyrhythmic Glitch Chopper",
    "Glitch & Deconstrucción Rítmica",
    "Slicer sincopado bifurcado a eco estéreo y modulación en anillo con filtro morphing SVF",
    0.3, 0.9, [0.8, 0.6, 0.7, 0.7, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Audio Slicer", "type": 49, "x": 40.0, "y": 80.0, "params": {"1": 0.0, "2": 8.0, "3": -1.0, "4": 1.0, "5": 0.0, "6": 60.0, "7": 4.0, "8": 0.3, "9": 0.0, "10": 12000.0, "11": 0.9}},
        {"id": 2, "name": "Ping-Pong Delay", "type": 28, "x": 240.0, "y": 30.0, "params": {"1": 180.0, "2": 270.0, "3": 0.5, "4": 6000.0, "5": 1.0, "6": 0.6}},
        {"id": 3, "name": "Ring Modulator", "type": 19, "x": 240.0, "y": 140.0, "params": {"1": 220.0, "2": 0.0, "3": 0.25, "4": 0.75}},
        {"id": 4, "name": "Morphing SVF Band", "type": 66, "x": 460.0, "y": 80.0, "params": {"1": 1500.0, "2": 6.0, "3": 0.2, "4": 2.5, "5": 0.0}},
        {"id": 5, "name": "Dynamic Tremolo", "type": 61, "x": 660.0, "y": 80.0, "params": {"1": 6.0, "2": 0.8, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.6, "7": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Cybernetic Stutter Matrix",
    "Glitch & Deconstrucción Rítmica",
    "Deconstrucción glitch estocástica en paralelo a corrimiento de frecuencia Hilbert y dispersión allpass",
    0.3, 0.9, [0.85, 0.7, 0.6, 0.8, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Glitch Machine", "type": 14, "x": 40.0, "y": 80.0, "params": {"1": 1.0, "2": 0.7, "3": 0.35, "4": 0.4, "5": 0.9}},
        {"id": 2, "name": "Freq Shifter", "type": 20, "x": 240.0, "y": 30.0, "params": {"1": 14.0, "2": 0.3, "3": 0.8}},
        {"id": 3, "name": "Phase Dispersion", "type": 65, "x": 240.0, "y": 140.0, "params": {"1": 18.0, "2": 2200.0, "3": 0.85, "4": 12000.0, "5": 0.6, "6": 0.9}},
        {"id": 4, "name": "Multiband Saturator", "type": 62, "x": 460.0, "y": 80.0, "params": {"1": 2.2, "2": 1.0, "3": 2.8, "4": 2.0, "5": 1.5, "6": 1.0, "7": 200.0, "8": 3200.0, "9": 0.85}},
        {"id": 5, "name": "Space Delay", "type": 28, "x": 660.0, "y": 80.0, "params": {"1": 200.0, "2": 300.0, "3": 0.5, "4": 7000.0, "5": 1.0, "6": 0.55}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Quantum Beat Decimator",
    "Glitch & Deconstrucción Rítmica",
    "Fragmentador rítmico con pitch shift a la quinta, decimation no lineal y trémolo dinámico",
    0.35, 0.9, [0.8, 0.6, 0.6, 0.7, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Audio Slicer", "type": 49, "x": 40.0, "y": 80.0, "params": {"1": 0.0, "2": 8.0, "3": -1.0, "4": 1.0, "5": 0.0, "6": 45.0, "7": 5.0, "8": 0.35, "9": 0.0, "10": 13000.0, "11": 0.95}},
        {"id": 2, "name": "Pitch Shift +7st", "type": 11, "x": 240.0, "y": 30.0, "params": {"1": 7.0, "2": 0.0, "3": 35.0, "4": 0.8}},
        {"id": 3, "name": "Bitcrusher Decimator", "type": 40, "x": 240.0, "y": 140.0, "params": {"1": 10.0, "2": 2.2, "3": 0.03, "4": 0.3, "5": 0.0, "6": 0.7}},
        {"id": 4, "name": "Comb Resonator", "type": 63, "x": 460.0, "y": 80.0, "params": {"1": 260.0, "2": 0.85, "3": 7500.0, "4": 12.0, "5": 0.65}},
        {"id": 5, "name": "Dynamic Tremolo", "type": 61, "x": 660.0, "y": 80.0, "params": {"1": 5.5, "2": 0.75, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.65, "7": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 7. Cintas Analógicas, Lo-Fi & Saturación Multibanda
add(
    "Melted Tape Memory Machine",
    "Cintas Analógicas & Lo-Fi",
    "Emulación de cassette vintage desgastado con saturador tri-banda, eco oscuro y chorus BBD espacial",
    0.4, 0.9, [0.75, 0.6, 0.8, 0.5, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Lo-Fi Cassette", "type": 56, "x": 40.0, "y": 80.0, "params": {"1": 0.7, "2": 0.5, "3": 0.4, "4": 0.35, "5": 4500.0, "6": 0.6, "7": 0.85}},
        {"id": 2, "name": "Multiband Saturator", "type": 62, "x": 240.0, "y": 80.0, "params": {"1": 2.5, "2": 1.0, "3": 2.2, "4": 1.5, "5": 1.4, "6": 1.0, "7": 220.0, "8": 3200.0, "9": 0.85}},
        {"id": 3, "name": "Advanced Delay", "type": 28, "x": 440.0, "y": 30.0, "params": {"1": 280.0, "2": 420.0, "3": 0.55, "4": 4500.0, "5": 1.0, "6": 0.6}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.4, "2": 12.0, "3": 110.0, "4": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 25.0, "4": 1.5, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "1978 Magnetic Decay Spiral",
    "Cintas Analógicas & Lo-Fi",
    "Saturación de cinta con histéresis magnética dividida a resonador comb y trémolo óptico analógico",
    0.35, 0.9, [0.7, 0.7, 0.75, 0.6, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Tape Saturation", "type": 21, "x": 40.0, "y": 80.0, "params": {"1": 3.8, "2": 1.0, "3": 0.45, "4": 0.8, "5": 0.85}},
        {"id": 2, "name": "Comb Resonator", "type": 63, "x": 240.0, "y": 30.0, "params": {"1": 180.0, "2": 0.85, "3": 6500.0, "4": 10.0, "5": 0.65}},
        {"id": 3, "name": "Dynamic Tremolo", "type": 61, "x": 240.0, "y": 140.0, "params": {"1": 4.0, "2": 0.75, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.6, "7": 0.85}},
        {"id": 4, "name": "Space Reverb", "type": 6, "x": 460.0, "y": 80.0, "params": {"1": 0.85, "2": 4.5, "3": 5000.0, "4": 25.0, "5": 0.55}},
        {"id": 5, "name": "Tape Stop", "type": 30, "x": 660.0, "y": 80.0, "params": {"1": 0.0, "2": 450.0, "3": 0.8, "4": 0.7}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Radioactive Cassette Degradation",
    "Cintas Analógicas & Lo-Fi",
    "Generador de ruido y estática de vinilo con decimador, filtro paso-banda morphing y degradación física",
    0.3, 0.95, [0.8, 0.5, 0.7, 0.6, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Noise Texture", "type": 32, "x": 40.0, "y": 30.0, "params": {"1": 0.0, "2": 0.35, "3": 2500.0, "4": 0.8, "5": 0.4}},
        {"id": 2, "name": "Bitcrusher Decimator", "type": 40, "x": 40.0, "y": 140.0, "params": {"1": 10.0, "2": 2.0, "3": 0.04, "4": 0.3, "5": 0.0, "6": 0.65}},
        {"id": 3, "name": "Morphing SVF Tele", "type": 66, "x": 240.0, "y": 80.0, "params": {"1": 1600.0, "2": 4.5, "3": 0.25, "4": 3.0, "5": 0.0}},
        {"id": 4, "name": "Lo-Fi Cassette", "type": 56, "x": 460.0, "y": 80.0, "params": {"1": 0.8, "2": 0.6, "3": 0.5, "4": 0.45, "5": 3800.0, "6": 0.7, "7": 0.9}},
        {"id": 5, "name": "Dimension Chorus", "type": 60, "x": 660.0, "y": 80.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Sub-Zero Drone Machine",
    "Cintas Analógicas & Lo-Fi",
    "Plataforma de zumbidos y texturas oscuras con ecualización quirúrgica, delay de cinta y chorus analógico",
    0.3, 0.95, [0.75, 0.6, 0.85, 0.5, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Lo-Fi Cassette", "type": 56, "x": 40.0, "y": 80.0, "params": {"1": 0.6, "2": 0.4, "3": 0.3, "4": 0.3, "5": 5000.0, "6": 0.5, "7": 0.8}},
        {"id": 2, "name": "Parametric EQ", "type": 27, "x": 240.0, "y": 80.0, "params": {"1": 80.0, "2": 3.0, "3": 850.0, "4": 1.5, "5": -4.0, "6": 7000.0, "7": -3.0}},
        {"id": 3, "name": "Advanced Delay", "type": 28, "x": 440.0, "y": 30.0, "params": {"1": 350.0, "2": 525.0, "3": 0.6, "4": 5500.0, "5": 1.0, "6": 0.6}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.65, "3": 0.85}},
        {"id": 5, "name": "Space Reverb", "type": 6, "x": 660.0, "y": 80.0, "params": {"1": 0.95, "2": 6.5, "3": 4500.0, "4": 35.0, "5": 0.6}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 8. Espacios Infinitos & Reverbs Modulares
add(
    "Abyssal Shimmer Wormhole",
    "Espacios Infinitos & Shimmer",
    "Reverberación shimmer de colas infinitas dividida en difusión espectral y dispersión allpass con decodificador Mid/Side",
    0.2, 0.95, [0.9, 0.6, 0.95, 0.6, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Shimmer Reverb", "type": 50, "x": 40.0, "y": 80.0, "params": {"1": 12.0, "2": 7000.0, "3": 0.7, "4": 0.0, "5": 1.6, "6": 0.75}},
        {"id": 2, "name": "Spectral Smear", "type": 52, "x": 260.0, "y": 30.0, "params": {"1": 5.0, "2": 0.9, "3": 0.92, "4": -0.1, "5": 0.85}},
        {"id": 3, "name": "Phase Dispersion", "type": 65, "x": 260.0, "y": 140.0, "params": {"1": 20.0, "2": 1500.0, "3": 0.85, "4": 11000.0, "5": 0.55, "6": 0.9}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 500.0, "y": 80.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Mid/Side Decoder", "type": 25, "x": 720.0, "y": 80.0, "params": {"1": 1.6, "2": 130.0, "3": 0.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 920.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 35.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Celestial Catacomb Reflection",
    "Espacios Infinitos & Shimmer",
    "Reflexiones de catedral inmensa refractadas en prisma armónico con paneo orbital y trémolo",
    0.25, 0.9, [0.8, 0.7, 0.9, 0.5, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Space Reverb", "type": 6, "x": 40.0, "y": 80.0, "params": {"1": 0.96, "2": 9.0, "3": 5500.0, "4": 40.0, "5": 0.65}},
        {"id": 2, "name": "Refraction Prism", "type": 51, "x": 240.0, "y": 80.0, "params": {"1": 8.0, "2": 20.0, "3": 22.0, "4": 0.9, "5": 0.75, "6": 2.0, "7": 0.8}},
        {"id": 3, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 30.0, "params": {"1": 3.0, "2": 0.7, "3": 1.0, "4": 1.0, "5": 0.35, "6": 0.55, "7": 0.8}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 30.0, "2": 25.0, "3": 2.2, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Reverse Bloom Nebula",
    "Espacios Infinitos & Shimmer",
    "Hinchamiento de reverberación invertida retroalimentada a eco espacial y resonador áureo",
    0.3, 0.95, [0.85, 0.5, 0.85, 0.6, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Reverse Reverb", "type": 38, "x": 40.0, "y": 80.0, "params": {"1": 2.0, "2": 0.85, "3": 7000.0, "4": 0.85}},
        {"id": 2, "name": "Advanced Delay", "type": 28, "x": 240.0, "y": 80.0, "params": {"1": 300.0, "2": 450.0, "3": 0.6, "4": 6500.0, "5": 1.0, "6": 0.65}},
        {"id": 3, "name": "Fractal Resonator", "type": 67, "x": 440.0, "y": 30.0, "params": {"1": 220.0, "2": 0.618, "3": 0.9, "4": 8000.0, "5": 0.85, "6": 0.8}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.5, "2": 15.0, "3": 120.0, "4": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Prismatic Ambient Halo",
    "Espacios Infinitos & Shimmer",
    "Dispersión prismática con calor analógico de cinta, shimmer espacial y paneo tridimensional",
    0.3, 0.9, [0.85, 0.6, 0.9, 0.5, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Refraction Prism", "type": 51, "x": 40.0, "y": 80.0, "params": {"1": 6.0, "2": 16.0, "3": 18.0, "4": 0.85, "5": 0.5, "6": 1.8, "7": 0.8}},
        {"id": 2, "name": "Tape Flutter", "type": 21, "x": 240.0, "y": 80.0, "params": {"1": 2.8, "2": 0.0, "3": 0.65, "4": 0.75, "5": 0.75}},
        {"id": 3, "name": "Shimmer Reverb", "type": 50, "x": 440.0, "y": 30.0, "params": {"1": 9.0, "2": 7000.0, "3": 0.6, "4": 1.0, "5": 1.4, "6": 0.65}},
        {"id": 4, "name": "Phase Dispersion", "type": 65, "x": 440.0, "y": 140.0, "params": {"1": 16.0, "2": 1800.0, "3": 0.75, "4": 12000.0, "5": 0.5, "6": 0.85}},
        {"id": 5, "name": "Spatializer 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": -30.0, "2": 20.0, "3": 2.5, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 9. Modulación Psicoacústica & Espacio 3D
add(
    "Vortex Dimension 3D Orbital",
    "Modulación Psicoacústica",
    "Chorus SDD-320 de 4 voces cruzado con phaser de 6 etapas, trémolo armónico y paneo esférico 3D",
    0.35, 0.9, [0.7, 0.8, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Dimension Chorus", "type": 60, "x": 40.0, "y": 80.0, "params": {"1": 4.0, "2": 0.7, "3": 0.9}},
        {"id": 2, "name": "Phaser 6-Stage", "type": 16, "x": 240.0, "y": 30.0, "params": {"1": 0.4, "2": 0.8, "3": 0.6, "4": 1200.0, "5": 0.85}},
        {"id": 3, "name": "Dynamic Tremolo", "type": 61, "x": 240.0, "y": 140.0, "params": {"1": 5.0, "2": 0.8, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.65, "7": 0.85}},
        {"id": 4, "name": "Spatial Panner 3D", "type": 26, "x": 460.0, "y": 80.0, "params": {"1": 55.0, "2": 25.0, "3": 2.2, "4": 1.0}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.5, "2": 15.0, "3": 120.0, "4": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Bipolar Comb Weaver Matrix",
    "Modulación Psicoacústica",
    "Resonador de peine bipolar con codificación M/S: ecualización central y dispersión allpass en laterales",
    0.35, 0.9, [0.65, 0.75, 0.5, 0.7, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Comb Resonator", "type": 63, "x": 40.0, "y": 80.0, "params": {"1": 330.0, "2": -0.88, "3": 8500.0, "4": 16.0, "5": 0.8}},
        {"id": 2, "name": "M/S Encoder", "type": 24, "x": 240.0, "y": 80.0, "params": {"1": 1.0}},
        {"id": 3, "name": "Mid Tone EQ", "type": 27, "x": 440.0, "y": 30.0, "params": {"1": 100.0, "2": 2.0, "3": 2500.0, "4": 2.0, "5": 3.0, "6": 8000.0, "7": -1.0}},
        {"id": 4, "name": "Side Phase Dispersion", "type": 65, "x": 440.0, "y": 140.0, "params": {"1": 16.0, "2": 2000.0, "3": 0.8, "4": 12000.0, "5": 0.5, "6": 0.9}},
        {"id": 5, "name": "M/S Decoder", "type": 25, "x": 660.0, "y": 80.0, "params": {"1": 1.6, "2": 110.0, "3": 0.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Haas Micro-Pitch Hologram",
    "Modulación Psicoacústica",
    "Dual detuning micro-tonal de Cristales con retardo Haas estéreo y filtrado mutante en barrido 3D",
    0.3, 0.9, [0.8, 0.7, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Pitch Delay Micro", "type": 64, "x": 40.0, "y": 80.0, "params": {"1": 40.0, "2": 60.0, "3": 0.0, "4": 12.0, "5": 0.35, "6": 0.0, "7": 9000.0, "8": 0.8}},
        {"id": 2, "name": "Stereo Imager Haas", "type": 55, "x": 240.0, "y": 80.0, "params": {"1": 1.8, "2": 16.0, "3": 130.0, "4": 0.9}},
        {"id": 3, "name": "Morphing SVF Tone", "type": 66, "x": 440.0, "y": 30.0, "params": {"1": 1800.0, "2": 3.5, "3": 0.45, "4": 2.0, "5": 0.0}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 40.0, "2": 10.0, "3": 2.0, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Orbital Euclidean Pulsar",
    "Modulación Psicoacústica",
    "Paneo orbital 3D continuo alimentando chorus dimensional y retardo con modulación de cinta",
    0.3, 0.9, [0.75, 0.85, 0.7, 0.5, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Spatial Panner 3D", "type": 26, "x": 40.0, "y": 80.0, "params": {"1": 45.0, "2": 15.0, "3": 2.0, "4": 1.0}},
        {"id": 2, "name": "Dimension Chorus", "type": 60, "x": 240.0, "y": 80.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 3, "name": "Space Delay", "type": 28, "x": 440.0, "y": 30.0, "params": {"1": 240.0, "2": 360.0, "3": 0.5, "4": 7000.0, "5": 1.0, "6": 0.55}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 4.0, "2": 0.7, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.6, "7": 0.8}},
        {"id": 5, "name": "Lo-Fi Cassette", "type": 56, "x": 660.0, "y": 80.0, "params": {"1": 0.5, "2": 0.35, "3": 0.25, "4": 0.2, "5": 6000.0, "6": 0.4, "7": 0.75}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 10. Lazos de Realimentación Inestables
add(
    "Supercritical Feedback Reactor",
    "Lazos de Realimentación Inestable",
    "Reactor de realimentación acoplado con corrimiento armónico Hilbert, saturación y dispersión en peine",
    0.2, 0.95, [0.85, 0.75, 0.7, 0.8, 0.7, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Freq Shifter", "type": 20, "x": 40.0, "y": 80.0, "params": {"1": 8.0, "2": 0.3, "3": 0.85}},
        {"id": 2, "name": "Wavefold Distortion", "type": 8, "x": 240.0, "y": 80.0, "params": {"1": 4.5, "2": 2.0, "3": 5500.0, "4": 0.8}},
        {"id": 3, "name": "Comb Resonator", "type": 63, "x": 440.0, "y": 30.0, "params": {"1": 220.0, "2": 0.85, "3": 7000.0, "4": 12.0, "5": 0.7}},
        {"id": 4, "name": "Phase Dispersion", "type": 65, "x": 440.0, "y": 140.0, "params": {"1": 18.0, "2": 2000.0, "3": 0.8, "4": 11000.0, "5": 0.6, "6": 0.9}},
        {"id": 5, "name": "Dimension Chorus", "type": 60, "x": 660.0, "y": 80.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 25.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Chaotic Tape Resonance Loop",
    "Lazos de Realimentación Inestable",
    "Saturador analógico alimentando filtro mutante SVF en auto-oscilación con delay espacial",
    0.25, 0.9, [0.8, 0.7, 0.6, 0.8, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Tape Saturation", "type": 21, "x": 40.0, "y": 80.0, "params": {"1": 4.0, "2": 1.0, "3": 0.4, "4": 0.8, "5": 0.85}},
        {"id": 2, "name": "Morphing SVF Res", "type": 66, "x": 240.0, "y": 80.0, "params": {"1": 950.0, "2": 15.0, "3": 0.35, "4": 4.5, "5": 0.0}},
        {"id": 3, "name": "Space Delay", "type": 28, "x": 440.0, "y": 30.0, "params": {"1": 240.0, "2": 360.0, "3": 0.6, "4": 6000.0, "5": 1.0, "6": 0.65}},
        {"id": 4, "name": "Multiband Saturator", "type": 62, "x": 440.0, "y": 140.0, "params": {"1": 2.0, "2": 1.0, "3": 2.5, "4": 2.0, "5": 1.6, "6": 1.0, "7": 200.0, "8": 3200.0, "9": 0.8}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 50.0, "2": 15.0, "3": 2.0, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Self-Oscillating Resonant Spiral",
    "Lazos de Realimentación Inestable",
    "Banco de resonadores modales excitando matriz fractal con control automático de ganancia y paneo 3D",
    0.2, 0.95, [0.85, 0.6, 0.8, 0.5, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Resonator Bank", "type": 13, "x": 40.0, "y": 80.0, "params": {"1": 220.0, "2": 2.5, "3": 0.45, "4": 0.25, "5": 0.8}},
        {"id": 2, "name": "Fractal Resonator", "type": 67, "x": 240.0, "y": 80.0, "params": {"1": 440.0, "2": 0.618, "3": 0.94, "4": 9000.0, "5": 0.85, "6": 0.9}},
        {"id": 3, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 30.0, "params": {"1": 3.0, "2": 0.75, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.6, "7": 0.85}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 60.0, "2": 20.0, "3": 2.2, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 25.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Vocoded Spectral Mist Loop",
    "Lazos de Realimentación Inestable",
    "Vocoder modular acoplado a difusión espectral y retardo con realimentación densa",
    0.3, 0.9, [0.8, 0.5, 0.8, 0.6, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Vocoder Carrier", "type": 36, "x": 40.0, "y": 80.0, "params": {"1": 16.0, "2": 0.5, "3": 0.8, "4": 0.85}},
        {"id": 2, "name": "Spectral Smear", "type": 52, "x": 240.0, "y": 80.0, "params": {"1": 5.0, "2": 0.9, "3": 0.92, "4": -0.1, "5": 0.85}},
        {"id": 3, "name": "Space Delay", "type": 28, "x": 440.0, "y": 30.0, "params": {"1": 250.0, "2": 375.0, "3": 0.6, "4": 6500.0, "5": 1.0, "6": 0.65}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.5, "2": 15.0, "3": 120.0, "4": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 11. Sintetizador Vocal, Formantes & Robótica
add(
    "Alien Throat Morphosphere",
    "Sintetizador de Voces & Formantes",
    "Sintetizador de tracto vocal alienígena con vocoder, filtro de formantes, SVF mutante y chorus",
    0.3, 0.9, [0.8, 0.7, 0.7, 0.6, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Vocoder Engine", "type": 36, "x": 40.0, "y": 80.0, "params": {"1": 16.0, "2": 0.6, "3": 0.85, "4": 0.9}},
        {"id": 2, "name": "Formant Vowel", "type": 31, "x": 240.0, "y": 80.0, "params": {"1": 0.5, "2": 5.0, "3": 1.2, "4": 0.45, "5": 0.85}},
        {"id": 3, "name": "Morphing SVF Peak", "type": 66, "x": 440.0, "y": 30.0, "params": {"1": 1400.0, "2": 6.0, "3": 0.7, "4": 3.0, "5": 0.0}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 5.0, "2": 0.8, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.65, "7": 0.85}},
        {"id": 5, "name": "Dimension Chorus", "type": 60, "x": 660.0, "y": 80.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Android Talking Synthesizer",
    "Sintetizador de Voces & Formantes",
    "Modulador en anillo acoplado a formantes vocales con decimador 8-bit y retardo espacial",
    0.3, 0.9, [0.85, 0.6, 0.6, 0.7, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Ring Modulator", "type": 19, "x": 40.0, "y": 80.0, "params": {"1": 150.0, "2": 0.0, "3": 0.2, "4": 0.85}},
        {"id": 2, "name": "Formant Filter", "type": 31, "x": 240.0, "y": 80.0, "params": {"1": 0.4, "2": 4.0, "3": 1.0, "4": 0.4, "5": 0.8}},
        {"id": 3, "name": "Bitcrusher Decimator", "type": 40, "x": 440.0, "y": 30.0, "params": {"1": 8.0, "2": 2.5, "3": 0.03, "4": 0.3, "5": 0.0, "6": 0.7}},
        {"id": 4, "name": "Space Delay", "type": 28, "x": 440.0, "y": 140.0, "params": {"1": 220.0, "2": 330.0, "3": 0.5, "4": 6500.0, "5": 1.0, "6": 0.6}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.5, "2": 15.0, "3": 120.0, "4": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Spectral Robot Choir",
    "Sintetizador de Voces & Formantes",
    "Coro robótico extraterrestre con transposición a la octava, matriz fractal áurea y reverb",
    0.25, 0.95, [0.8, 0.5, 0.85, 0.6, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Vocoder Engine", "type": 36, "x": 40.0, "y": 80.0, "params": {"1": 16.0, "2": 0.7, "3": 0.85, "4": 0.9}},
        {"id": 2, "name": "Pitch Shift +12st", "type": 11, "x": 240.0, "y": 80.0, "params": {"1": 12.0, "2": 0.0, "3": 35.0, "4": 0.75}},
        {"id": 3, "name": "Fractal Resonator", "type": 67, "x": 440.0, "y": 30.0, "params": {"1": 440.0, "2": 0.618, "3": 0.92, "4": 9000.0, "5": 0.85, "6": 0.85}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Space Reverb", "type": 6, "x": 660.0, "y": 80.0, "params": {"1": 0.9, "2": 5.5, "3": 6000.0, "4": 30.0, "5": 0.6}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Vocal Talkbox Resonator",
    "Sintetizador de Voces & Formantes",
    "Filtro auto-wah con seguimiento envolvente impulsando formantes y saturación no lineal",
    0.35, 0.9, [0.75, 0.6, 0.6, 0.7, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Auto-Wah Filter", "type": 57, "x": 40.0, "y": 80.0, "params": {"1": 600.0, "2": 3.5, "3": 0.7, "4": 15.0, "5": 120.0, "6": 0.8}},
        {"id": 2, "name": "Formant Filter", "type": 31, "x": 240.0, "y": 80.0, "params": {"1": 0.35, "2": 4.5, "3": 1.1, "4": 0.45, "5": 0.85}},
        {"id": 3, "name": "Multiband Saturator", "type": 62, "x": 440.0, "y": 30.0, "params": {"1": 2.0, "2": 1.0, "3": 2.5, "4": 2.0, "5": 1.5, "6": 1.0, "7": 200.0, "8": 3200.0, "9": 0.8}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 4.5, "2": 0.7, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.6, "7": 0.8}},
        {"id": 5, "name": "Dimension Chorus", "type": 60, "x": 660.0, "y": 80.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 12. Dinámica Agresiva & Destrucción Sonora
add(
    "Master Wall Multiband Crusher",
    "Dinámica Agresiva & OTT",
    "Saturador multibanda tri-frecuencia acoplado a dinámica OTT agresiva, transient punch y limitador brickwall",
    0.3, 0.95, [0.8, 0.7, 0.5, 0.85, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Multiband Saturator", "type": 62, "x": 40.0, "y": 80.0, "params": {"1": 3.5, "2": 1.0, "3": 2.8, "4": 2.0, "5": 2.2, "6": 1.0, "7": 180.0, "8": 3000.0, "9": 0.9}},
        {"id": 2, "name": "Multiband OTT", "type": 15, "x": 240.0, "y": 80.0, "params": {"1": 180.0, "2": 3000.0, "3": 1.6, "4": 1.3, "5": 1.4, "6": 0.95}},
        {"id": 3, "name": "Transient Snap", "type": 33, "x": 440.0, "y": 30.0, "params": {"1": 0.8, "2": -0.3, "3": 1.2, "4": 1.0, "5": 2.0, "6": 1.0}},
        {"id": 4, "name": "Tape Saturation", "type": 21, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 1.0, "3": 0.3, "4": 0.8, "5": 0.85}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.4, "2": 12.0, "3": 120.0, "4": 0.85}},
        {"id": 6, "name": "Brickwall Peak Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 25.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Industrial Cybernetic Wavefolder",
    "Dinámica Agresiva & OTT",
    "Pliegue de onda no lineal con filtro mutante SVF saturado y compresión hacia arriba",
    0.35, 0.9, [0.85, 0.6, 0.5, 0.9, 0.7, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Wavefold Distortion", "type": 8, "x": 40.0, "y": 80.0, "params": {"1": 5.5, "2": 2.0, "3": 6500.0, "4": 0.85}},
        {"id": 2, "name": "Morphing SVF Peak", "type": 66, "x": 240.0, "y": 80.0, "params": {"1": 1200.0, "2": 8.0, "3": 0.65, "4": 4.5, "5": 0.0}},
        {"id": 3, "name": "Multiband OTT", "type": 15, "x": 440.0, "y": 30.0, "params": {"1": 200.0, "2": 3200.0, "3": 1.4, "4": 1.2, "5": 1.3, "6": 0.9}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 6.0, "2": 0.75, "3": 1.0, "4": 1.0, "5": 0.45, "6": 0.65, "7": 0.85}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 45.0, "2": 15.0, "3": 2.0, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 25.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Transient Demolisher Rig",
    "Dinámica Agresiva & OTT",
    "Modelador de transientes con decimación bitcrush y saturación analógica en tres bandas",
    0.3, 0.95, [0.8, 0.5, 0.7, 0.8, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Transient Snap", "type": 33, "x": 40.0, "y": 80.0, "params": {"1": 0.9, "2": -0.4, "3": 1.4, "4": 1.0, "5": 2.0, "6": 1.0}},
        {"id": 2, "name": "Bitcrusher Decimator", "type": 40, "x": 240.0, "y": 30.0, "params": {"1": 10.0, "2": 2.5, "3": 0.03, "4": 0.35, "5": 0.0, "6": 0.75}},
        {"id": 3, "name": "Multiband Saturator", "type": 62, "x": 240.0, "y": 140.0, "params": {"1": 2.8, "2": 1.0, "3": 2.6, "4": 2.0, "5": 1.8, "6": 1.0, "7": 200.0, "8": 3000.0, "9": 0.85}},
        {"id": 4, "name": "Comb Resonator", "type": 63, "x": 480.0, "y": 80.0, "params": {"1": 220.0, "2": 0.8, "3": 8000.0, "4": 12.0, "5": 0.65}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 680.0, "y": 80.0, "params": {"1": 1.5, "2": 15.0, "3": 120.0, "4": 0.8}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 880.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 25.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Hypersonic Cybernetic Slicer",
    "Dinámica Agresiva & OTT",
    "Fragmentación a 16vas con wavefolding no lineal, compresión multinivel y limitación de pico",
    0.3, 0.9, [0.85, 0.7, 0.6, 0.85, 0.7, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Hyper Slicer", "type": 49, "x": 40.0, "y": 80.0, "params": {"1": 0.0, "2": 16.0, "3": -1.0, "4": 1.0, "5": 0.0, "6": 40.0, "7": 6.0, "8": 0.4, "9": 0.0, "10": 16000.0, "11": 1.0}},
        {"id": 2, "name": "Wavefold Drive", "type": 8, "x": 240.0, "y": 80.0, "params": {"1": 5.5, "2": 2.0, "3": 6500.0, "4": 0.8}},
        {"id": 3, "name": "Multiband OTT", "type": 15, "x": 440.0, "y": 30.0, "params": {"1": 180.0, "2": 3000.0, "3": 1.3, "4": 1.1, "5": 1.2, "6": 0.9}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 7.0, "2": 0.8, "3": 1.0, "4": 1.0, "5": 0.5, "6": 0.7, "7": 0.9}},
        {"id": 5, "name": "Dimension Chorus", "type": 60, "x": 660.0, "y": 80.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 6, "name": "Brickwall Peak", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -1.0, "2": -0.2, "3": 40.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 13. Subarmónicos, Graves Profundos & Tonalidad Sísmica
add(
    "Sub-Octave Earthquake Growl",
    "Subarmónicos & Graves",
    "Generador de sub-octava con saturación de bajos gruesos, EQ dinámica y limitador anti-clip",
    0.35, 0.95, [0.8, 0.5, 0.6, 0.75, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Pitch Shift -12st", "type": 11, "x": 40.0, "y": 80.0, "params": {"1": -12.0, "2": 0.0, "3": 50.0, "4": 0.9}},
        {"id": 2, "name": "Multiband Saturator Low", "type": 62, "x": 240.0, "y": 80.0, "params": {"1": 4.0, "2": 1.0, "3": 1.8, "4": 1.2, "5": 1.0, "6": 1.0, "7": 150.0, "8": 2500.0, "9": 0.9}},
        {"id": 3, "name": "Dynamic EQ Low", "type": 54, "x": 440.0, "y": 30.0, "params": {"1": 60.0, "2": 1.5, "3": 6.0, "4": -12.0, "5": 20.0, "6": 120.0, "7": 0.85}},
        {"id": 4, "name": "Tape Saturation", "type": 21, "x": 440.0, "y": 140.0, "params": {"1": 2.5, "2": 1.0, "3": 0.3, "4": 0.8, "5": 0.8}},
        {"id": 5, "name": "Stereo Imager Mono-Bass", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.2, "2": 0.0, "3": 180.0, "4": 0.9}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 35.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Deep Space Resonant Drone",
    "Subarmónicos & Graves",
    "Resonador modal a 55Hz (La fundamental) acoplado a peine sintonizado y matriz fractal en paneo 3D",
    0.25, 0.95, [0.85, 0.6, 0.85, 0.6, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Modal 55Hz", "type": 58, "x": 40.0, "y": 80.0, "params": {"1": 55.0, "2": 0.92, "3": 0.3, "4": 0.5, "5": 0.0, "6": 0.9}},
        {"id": 2, "name": "Comb Resonator", "type": 63, "x": 240.0, "y": 80.0, "params": {"1": 110.0, "2": 0.85, "3": 6000.0, "4": 12.0, "5": 0.7}},
        {"id": 3, "name": "Fractal Resonator", "type": 67, "x": 440.0, "y": 30.0, "params": {"1": 165.0, "2": 0.618, "3": 0.91, "4": 7000.0, "5": 0.85, "6": 0.8}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 2.0, "2": 0.5, "3": 0.8}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 40.0, "2": 15.0, "3": 2.0, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Sub-Harmonic Earth Shaker",
    "Subarmónicos & Graves",
    "Cuerda física Karplus de graves profundos con realce paramétrico y compresión multibanda",
    0.3, 0.9, [0.8, 0.5, 0.6, 0.8, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Karplus Bass", "type": 37, "x": 40.0, "y": 80.0, "params": {"1": 65.41, "2": 0.98, "3": 0.25, "4": 0.85, "5": 0.0, "6": 0.9}},
        {"id": 2, "name": "Parametric Sub EQ", "type": 27, "x": 240.0, "y": 80.0, "params": {"1": 50.0, "2": 4.5, "3": 200.0, "4": 1.8, "5": -3.0, "6": 6000.0, "7": -4.0}},
        {"id": 3, "name": "Multiband Saturator", "type": 62, "x": 440.0, "y": 30.0, "params": {"1": 3.0, "2": 1.0, "3": 1.6, "4": 1.2, "5": 1.0, "6": 1.0, "7": 160.0, "8": 2800.0, "9": 0.85}},
        {"id": 4, "name": "Multiband OTT", "type": 15, "x": 440.0, "y": 140.0, "params": {"1": 150.0, "2": 2500.0, "3": 1.4, "4": 1.1, "5": 1.2, "6": 0.9}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.2, "2": 0.0, "3": 160.0, "4": 0.9}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Seismic Sub Reinforcer",
    "Subarmónicos & Graves",
    "Transientes contundentes reforzados con subarmónico a -12st y saturador de 3 bandas",
    0.35, 0.9, [0.75, 0.5, 0.5, 0.8, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Transient Snap", "type": 33, "x": 40.0, "y": 80.0, "params": {"1": 0.8, "2": -0.3, "3": 1.3, "4": 1.0, "5": 2.0, "6": 1.0}},
        {"id": 2, "name": "Pitch Shift Sub", "type": 11, "x": 240.0, "y": 80.0, "params": {"1": -12.0, "2": 0.0, "3": 45.0, "4": 0.85}},
        {"id": 3, "name": "Multiband Saturator", "type": 62, "x": 440.0, "y": 30.0, "params": {"1": 3.2, "2": 1.0, "3": 2.0, "4": 1.5, "5": 1.2, "6": 1.0, "7": 180.0, "8": 3000.0, "9": 0.85}},
        {"id": 4, "name": "Dynamic EQ Low", "type": 54, "x": 440.0, "y": 140.0, "params": {"1": 80.0, "2": 2.0, "3": 4.0, "4": -14.0, "5": 15.0, "6": 100.0, "7": 0.8}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.2, "2": 0.0, "3": 160.0, "4": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 14. Experimental, Avant-Garde & Texturas Alienígenas
add(
    "Extraterrestrial Bio-Resonator",
    "Experimental & Avant-Garde",
    "Resonador fractal acoplado a dispersión allpass y filtro SVF mutante con audio-rate FM",
    0.2, 0.95, [0.9, 0.7, 0.75, 0.8, 0.7, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Fractal Resonator", "type": 67, "x": 40.0, "y": 80.0, "params": {"1": 220.0, "2": 0.618, "3": 0.93, "4": 8500.0, "5": 0.85, "6": 0.9}},
        {"id": 2, "name": "Ring Modulator FM", "type": 19, "x": 240.0, "y": 30.0, "params": {"1": 180.0, "2": 0.0, "3": 0.25, "4": 0.85}},
        {"id": 3, "name": "Phase Dispersion", "type": 65, "x": 240.0, "y": 140.0, "params": {"1": 20.0, "2": 2400.0, "3": 0.88, "4": 12000.0, "5": 0.6, "6": 0.9}},
        {"id": 4, "name": "Morphing SVF Acid", "type": 66, "x": 460.0, "y": 80.0, "params": {"1": 950.0, "2": 14.0, "3": 0.45, "4": 4.0, "5": 0.6}},
        {"id": 5, "name": "Dimension Chorus", "type": 60, "x": 660.0, "y": 80.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.8, "2": -0.1, "3": 25.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 4},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Quantum Entangled Particle Wave",
    "Experimental & Avant-Garde",
    "Partículas segmentadas divididas en refracción prismática, pitch delay y difusión espectral",
    0.25, 0.95, [0.85, 0.6, 0.85, 0.6, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Audio Slicer", "type": 49, "x": 40.0, "y": 80.0, "params": {"1": 0.0, "2": 16.0, "3": -1.0, "4": 1.0, "5": 0.0, "6": 40.0, "7": 5.0, "8": 0.35, "9": 0.0, "10": 14000.0, "11": 0.95}},
        {"id": 2, "name": "Refraction Prism", "type": 51, "x": 240.0, "y": 30.0, "params": {"1": 8.0, "2": 22.0, "3": 24.0, "4": 0.9, "5": 0.8, "6": 2.2, "7": 0.85}},
        {"id": 3, "name": "Pitch Delay Crystals", "type": 64, "x": 240.0, "y": 140.0, "params": {"1": 200.0, "2": 300.0, "3": 7.0, "4": 14.0, "5": 0.55, "6": 0.0, "7": 8500.0, "8": 0.75}},
        {"id": 4, "name": "Spectral Smear", "type": 52, "x": 460.0, "y": 80.0, "params": {"1": 4.5, "2": 0.88, "3": 0.9, "4": -0.1, "5": 0.85}},
        {"id": 5, "name": "Mid/Side Decoder", "type": 25, "x": 660.0, "y": 80.0, "params": {"1": 1.6, "2": 120.0, "3": 0.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 1, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Cosmic Laser Phantasm",
    "Experimental & Avant-Garde",
    "Transientes convertidos en estallidos de chirps láser con retardo cristalino y paneo esférico",
    0.3, 0.95, [0.8, 0.7, 0.85, 0.6, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Transient Snap", "type": 33, "x": 40.0, "y": 80.0, "params": {"1": 0.85, "2": -0.3, "3": 1.3, "4": 1.0, "5": 2.0, "6": 1.0}},
        {"id": 2, "name": "Laser Dispersion", "type": 65, "x": 240.0, "y": 80.0, "params": {"1": 24.0, "2": 2800.0, "3": 0.92, "4": 15000.0, "5": 0.7, "6": 1.0}},
        {"id": 3, "name": "Pitch Delay Crystals", "type": 64, "x": 440.0, "y": 30.0, "params": {"1": 160.0, "2": 240.0, "3": 12.0, "4": 15.0, "5": 0.6, "6": 0.0, "7": 9000.0, "8": 0.75}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 65.0, "2": 20.0, "3": 2.2, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 25.0, "4": 1.5, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Bioluminescent Dark Drone",
    "Experimental & Avant-Garde",
    "Resonancia física modal en baja frecuencia con filtro mutante SVF, retardo de tono y congelador espectral",
    0.2, 0.95, [0.85, 0.5, 0.9, 0.6, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Modal 110Hz", "type": 58, "x": 40.0, "y": 80.0, "params": {"1": 110.0, "2": 0.9, "3": 0.35, "4": 0.5, "5": 0.0, "6": 0.85}},
        {"id": 2, "name": "Morphing SVF Tone", "type": 66, "x": 240.0, "y": 80.0, "params": {"1": 750.0, "2": 5.0, "3": 0.3, "4": 2.5, "5": 0.0}},
        {"id": 3, "name": "Pitch Delay", "type": 64, "x": 440.0, "y": 30.0, "params": {"1": 250.0, "2": 375.0, "3": -7.0, "4": 10.0, "5": 0.55, "6": 0.0, "7": 7500.0, "8": 0.7}},
        {"id": 4, "name": "Spectral Freeze", "type": 12, "x": 440.0, "y": 140.0, "params": {"1": 0.85, "2": 0.8, "3": 0.9, "4": 0.8}},
        {"id": 5, "name": "Stereo Imager", "type": 55, "x": 660.0, "y": 80.0, "params": {"1": 1.6, "2": 15.0, "3": 130.0, "4": 0.85}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

# 15. Ecos Cinematográficos & Paisajes de Ciencia Ficción
add(
    "Solaris Deep Space Echoplex",
    "Ecos Cinematográficos & Paisajes",
    "Eco de cinta analógico profundo con retardos de cristal de cuarzo y matriz fractal en shimmer infinito",
    0.25, 0.95, [0.85, 0.6, 0.9, 0.5, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Advanced Tape Delay", "type": 28, "x": 40.0, "y": 80.0, "params": {"1": 350.0, "2": 525.0, "3": 0.6, "4": 6500.0, "5": 1.0, "6": 0.65}},
        {"id": 2, "name": "Pitch Delay Crystals", "type": 64, "x": 240.0, "y": 80.0, "params": {"1": 180.0, "2": 270.0, "3": 12.0, "4": 15.0, "5": 0.6, "6": 0.0, "7": 8500.0, "8": 0.75}},
        {"id": 3, "name": "Fractal Resonator", "type": 67, "x": 440.0, "y": 30.0, "params": {"1": 261.63, "2": 0.618, "3": 0.92, "4": 9000.0, "5": 0.85, "6": 0.85}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Shimmer Reverb", "type": 50, "x": 660.0, "y": 80.0, "params": {"1": 9.0, "2": 7500.0, "3": 0.65, "4": 0.0, "5": 1.5, "6": 0.7}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Glacial Canyon Horizon",
    "Ecos Cinematográficos & Paisajes",
    "Dispersión allpass líquida con ecos reflectantes, difusión espectral y reverberación de cañón ártico",
    0.3, 0.95, [0.8, 0.7, 0.9, 0.5, 0.4, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Phase Dispersion", "type": 65, "x": 40.0, "y": 80.0, "params": {"1": 20.0, "2": 1400.0, "3": 0.8, "4": 11000.0, "5": 0.5, "6": 0.9}},
        {"id": 2, "name": "Space Delay", "type": 28, "x": 240.0, "y": 80.0, "params": {"1": 400.0, "2": 600.0, "3": 0.65, "4": 6000.0, "5": 1.0, "6": 0.65}},
        {"id": 3, "name": "Spectral Smear", "type": 52, "x": 440.0, "y": 30.0, "params": {"1": 5.0, "2": 0.9, "3": 0.92, "4": -0.1, "5": 0.85}},
        {"id": 4, "name": "Dimension Chorus", "type": 60, "x": 440.0, "y": 140.0, "params": {"1": 3.0, "2": 0.6, "3": 0.85}},
        {"id": 5, "name": "Space Reverb", "type": 6, "x": 660.0, "y": 80.0, "params": {"1": 0.95, "2": 8.0, "3": 4500.0, "4": 35.0, "5": 0.6}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Sub-Quantum Wormhole Drift",
    "Ecos Cinematográficos & Paisajes",
    "Corrimiento de frecuencia Hilbert continuo con reverberación inversa, saturación y paneo esférico",
    0.3, 0.95, [0.8, 0.7, 0.85, 0.6, 0.5, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Freq Shifter", "type": 20, "x": 40.0, "y": 80.0, "params": {"1": 6.0, "2": 0.35, "3": 0.85}},
        {"id": 2, "name": "Reverse Reverb", "type": 38, "x": 240.0, "y": 80.0, "params": {"1": 1.8, "2": 0.85, "3": 6500.0, "4": 0.85}},
        {"id": 3, "name": "Multiband Saturator", "type": 62, "x": 440.0, "y": 30.0, "params": {"1": 2.2, "2": 1.0, "3": 2.6, "4": 2.0, "5": 1.6, "6": 1.0, "7": 220.0, "8": 3200.0, "9": 0.85}},
        {"id": 4, "name": "Dynamic Tremolo", "type": 61, "x": 440.0, "y": 140.0, "params": {"1": 4.5, "2": 0.75, "3": 1.0, "4": 1.0, "5": 0.4, "6": 0.6, "7": 0.85}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": 50.0, "2": 20.0, "3": 2.2, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

add(
    "Alien Ring Transmission Matrix",
    "Ecos Cinematográficos & Paisajes",
    "Modulador en anillo acoplado a transposición armónica a la 19na con pliegue de onda no lineal y retardo espacial",
    0.35, 0.9, [0.85, 0.6, 0.75, 0.7, 0.6, 0.5, 0.5, 0.0],
    [
        {"id": 1, "name": "Ring Modulator", "type": 19, "x": 40.0, "y": 80.0, "params": {"1": 280.0, "2": 0.0, "3": 0.2, "4": 0.85}},
        {"id": 2, "name": "Pitch Shift +19st", "type": 11, "x": 240.0, "y": 80.0, "params": {"1": 19.0, "2": 0.0, "3": 40.0, "4": 0.75}},
        {"id": 3, "name": "Wavefold Distortion", "type": 8, "x": 440.0, "y": 30.0, "params": {"1": 4.5, "2": 2.0, "3": 5500.0, "4": 0.8}},
        {"id": 4, "name": "Space Delay", "type": 28, "x": 440.0, "y": 140.0, "params": {"1": 300.0, "2": 450.0, "3": 0.55, "4": 6500.0, "5": 1.0, "6": 0.6}},
        {"id": 5, "name": "Spatial Panner 3D", "type": 26, "x": 660.0, "y": 80.0, "params": {"1": -40.0, "2": 20.0, "3": 2.2, "4": 1.0}},
        {"id": 6, "name": "Brickwall Limiter", "type": 39, "x": 860.0, "y": 80.0, "params": {"1": -0.5, "2": -0.1, "3": 30.0, "4": 2.0, "5": 1.0}}
    ],
    [
        {"id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1},
        {"id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1},
        {"id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1},
        {"id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1},
        {"id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1}
    ]
)

print(f"Total presets ready: {len(presets)}")

# Generate C++ Header
header_code = """#pragma once

#include <vector>
#include <string>

namespace audio_graph {

struct FactoryPresetEntry {
    std::string name;
    std::string category;
    std::string jsonContent;
};

/**
 * @brief Catálogo Maestro de Presets DAG de Alta Complejidad (Reglas 4, 21, 22, 44 y 46).
 * 60 Presets Flagship organizados en 15 categorías temáticas especializadas.
 * Todos los presets implementan topologías no triviales de Grafo Acíclico Dirigido (DAG):
 * bifurcaciones paralelas (fan-out/fan-in), decodificación Mid/Side, filtros mutantes no lineales (ZDF),
 * dispersión de fase allpass y resonadores fractales basados en la proporción áurea.
 */
inline std::vector<FactoryPresetEntry> createFactoryPresetCatalog() {
    std::vector<FactoryPresetEntry> catalog;
    catalog.reserve(""" + str(len(presets)) + """);

"""

for p in presets:
    header_code += f"""    // -------------------------------------------------------------------------
    // {p['category']} -> {p['name']}
    // -------------------------------------------------------------------------
    catalog.push_back({{
        "{p['name']}",
        "{p['category']}",
        R"json({p['json']})json"
    }});

"""

header_code += """    return catalog;
}

} // namespace audio_graph
"""

with open("source/preset/FactoryPresetCatalog.h", "w", encoding="utf-8") as f:
    f.write(header_code)

print("Successfully written source/preset/FactoryPresetCatalog.h!")
