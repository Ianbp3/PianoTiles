# Piano Virtual (C++ · GLFW · RtAudio)

Un **sintetizador de piano virtual en tiempo real** que permite tocar música usando el teclado QWERTY de tu computadora. Implementado en C++ con renderizado OpenGL (GLFW) y síntesis de audio de baja latencia (RtAudio).

## 🎹 Características Principales

- **Polifonía completa** - Toca múltiples notas simultáneamente
- **Síntesis en tiempo real** - 4 formas de onda (Sine, Square, Triangle, Saw)
- **Envolvente ADSR** - Ataques y decaimientos suaves por nota
- **Baja latencia** - < 20ms usando buffer de 512 frames @ 44.1kHz
- **VU Meters** - Visualización RMS y Peak en tiempo real
- **Interfaz visual** - Teclado de piano interactivo con feedback visual
- **Control dinámico** - Volumen, octavas y formas de onda en vivo

## 🎮 Controles

### Teclas del Piano (Mapeado QWERTY → Notas Musicales)
```
Teclas Blancas (Naturales):    A   S   D   F   G   H   J
Notas:                        C4  D4  E4  F4  G4  A4  B4

Teclas Negras (Sostenidos):     W   E     T   Y   U
Notas:                        C#4 D#4   F#4 G#4 A#4
```

### Controles de Parámetros
- **Volume:** `←` / `→` (Disminuir/Aumentar volumen master)
- **Octava:** `↑` / `↓` (Cambiar rango de octavas ±3)
- **Forma de Onda:** 
  - `1` - Sine (Suave)
  - `2` - Square (8-bit)
  - `3` - Triangle (Cálido)  
  - `4` - Saw (Brillante)
- **Chorus:** `C` (Efecto experimental - reservado para futuras versiones)
- **Salir:** `ESC`

## 🏗️ Arquitectura y Decisiones de Diseño

### Arquitectura de Audio
```
[Teclado QWERTY] → [Note Objects] → [Mixer] → [Soft Limiter] → [RtAudio] → [Speakers]
```

**Flujo de procesamiento:**
1. **Input**: Callbacks de GLFW capturan eventos de teclado
2. **Síntesis**: Cada nota genera samples usando osciladores + ADSR
3. **Mixing**: Suma de todas las voces activas por frame
4. **Processing**: Soft-knee limiting para prevenir clipping
5. **Output**: Buffer estéreo float32 enviado a RtAudio

### Decisiones Técnicas Clave

#### **Latencia y Performance**
- **Sample Rate**: 44.1kHz fijo (estándar de audio)
- **Buffer Size**: 512 frames (≈11.6ms de latencia)
- **Threading**: Audio callback en thread de alta prioridad (RT)
- **Lock Strategy**: Mutex mínimo para syncronización estado UI ↔ Audio

#### **Síntesis de Audio**
- **ADSR por Nota**: Attack=10ms, Decay=60ms, Sustain=70%, Release=120ms
- **Normalización RMS**: Cada forma de onda normalizada para volumen consistente
- **Headroom**: 10% de margen para prevenir saturación
- **Polifonía**: Sin límite artificial (limitado por CPU)

#### **Compatibilidad Multiplataforma**
- **Linux**: PulseAudio (preferido) → ALSA (fallback)
- **Windows**: WASAPI (recomendado para baja latencia)
- **macOS**: CoreAudio (nativo)
- **WSLg**: Soporte completo via PulseAudio bridge

## 📦 Instalación y Compilación

### Dependencias del Sistema

#### Ubuntu/Debian/WSLg
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
  libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libasound2-dev libpulse-dev libasound2-plugins pulseaudio-utils

# Opcional: GLFW del sistema (sino se descarga automáticamente)
sudo apt install -y libglfw3-dev
```

#### Windows (Visual Studio)
- **Visual Studio 2022** con workload "Desktop development with C++"
- **CMake** incluido o desde [cmake.org](https://cmake.org)
- Las dependencias se descargan automáticamente via FetchContent

#### macOS
```bash
# Via Homebrew
brew install cmake glfw

# O via Xcode Command Line Tools
xcode-select --install
```

### Compilación

#### Linux/macOS
```bash
git clone <repository-url>
cd PianoVirtual
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
./PianoTiles
```

#### Windows (Command Prompt)
```cmd
# Abrir "x64 Native Tools Command Prompt for VS 2022"
git clone <repository-url>
cd PianoVirtual
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
.\build\Release\PianoTiles.exe
```

#### WSLg (Windows Subsystem for Linux)
```bash
# Verificar que PulseAudio está disponible
echo $PULSE_SERVER  # Debe mostrar: unix:/mnt/wslg/PulseServer

# Compilar normalmente
mkdir build && cd build
cmake .. && make -j
./PianoTiles

# Si hay audio crackling, ajustar latencia:
PULSE_LATENCY_MSEC=30 ./PianoTiles
```

## 🔧 Solución de Problemas

### Problemas de Audio

**❌ "No audio devices found via RtAudio"**
```bash
# Linux: Verificar servicios de audio
systemctl --user status pulseaudio
pactl info

# Instalar plugins ALSA para PulseAudio
sudo apt install -y libasound2-plugins

# WSLg: Verificar bridge
echo $PULSE_SERVER
```

**❌ Crackling/Pops en el audio**
- **Causa**: Audio underruns (XRUNs)
- **Solución**: 
  - Cerrar aplicaciones pesadas
  - Aumentar buffer: `PULSE_LATENCY_MSEC=40`
  - Verificar que no hay `[XRUN]` en console

**❌ Latencia muy alta**
- Verificar que RtAudio usa el backend correcto
- En Linux: forzar PulseAudio sobre ALSA
- Evitar conversiones de sample rate

### Problemas de Compilación

**❌ "GL/gl.h: No such file"**
```bash
sudo apt install -y libgl1-mesa-dev
```

**❌ "X11/Xlib.h: No such file"**
```bash
sudo apt install -y libx11-dev libxrandr-dev libxinerama-dev
```

**❌ Error de FetchContent con RtAudio**
- Verificar conexión a internet
- Limpiar cache: `rm -rf build/` y recompilar

### Problemas de Rendimiento

**❌ UI lenta o choppy**
- **macOS**: OpenGL legacy puede fallar → considerar Metal/Vulkan
- **Linux**: Verificar drivers gráficos
- Reducir `printf` frequency en debug builds

## 📊 Especificaciones Técnicas

| Parámetro | Valor | Rationale |
|-----------|-------|-----------|
| Sample Rate | 44.1 kHz | Estándar de audio, compatible universalmente |
| Bit Depth | 32-bit float | Precisión numérica, sin quantization noise |
| Buffer Size | 512 frames | Balance latencia vs. estabilidad |
| Channels | 2 (Stereo) | Estándar, extensible a surround |
| Latency Target | < 20ms | Percepción humana de "instantáneo" |
| Polyphony | Unlimited* | *Limitado por CPU (~20-50 voces típico |

## 🛠️ Problemas Conocidos y Limitaciones

### Issues Actuales
1. **macOS Compatibility**: OpenGL legacy deprecated en macOS 10.14+
2. **JACK Support**: Requiere parche manual en RtAudio (incluido)
3. **Memory Usage**: Sin pool de objetos Note (creación/destrucción dinámica)

### Futuras Mejoras
- [ ] Migrar a OpenGL 3.3+ Core Profile
- [ ] Implementar efectos DSP (reverb, delay, chorus)
- [ ] Soporte MIDI input/output
- [ ] Preset system para timbres
- [ ] Recording/playback functionality

## 🧬 Estructura del Código y Explicación de Archivos

```
PianoTiles/
├── CMakeLists.txt                    # Sistema de build principal
├── README.md                         # Documentación del proyecto
├── cmake/
│   └── FetchRtAudioPatched.cmake    # Descarga y parcha RtAudio
└── src/
    ├── main.cpp                      # Punto de entrada de la aplicación
    ├── VirtualPiano.hpp              # Header de la clase principal
    ├── VirtualPiano.cpp              # Implementación principal (UI + Audio)
    └── Note.hpp                      # Definición de nota musical (header-only)
```

### 📄 Análisis Detallado de Cada Archivo

#### **`src/main.cpp`** (6 líneas)
```cpp
#include "VirtualPiano.hpp"
#include <iostream>

int main() {
    VirtualPiano app;                    // Crear instancia del piano
    if (!app.initialize()) {             // Inicializar GLFW, OpenGL, RtAudio
        std::cerr << "Failed to initialize Piano Virtual.\n";
        return 1;
    }
    app.run();                          // Bucle principal de eventos
    app.cleanup();                      // Limpieza de recursos
    return 0;
}
```
**Función**: Entry point minimalista que instancia la aplicación y maneja el ciclo de vida.

---

#### **`src/VirtualPiano.hpp`** (53 líneas)
**Imports y Configuración:**
```cpp
#ifndef GLFW_INCLUDE_NONE            // Evitar inclusión automática de OpenGL
#define GLFW_INCLUDE_NONE
#endif

#if defined(__APPLE__)               // Headers OpenGL multiplataforma
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif
```

**Estructura Principal:**
```cpp
struct SynthParams {                 // Configuración global del sintetizador
    double sampleRate = 44100.0;     // Frecuencia de muestreo fija
    unsigned int bufferFrames = 512;  // Tamaño de buffer (latencia)
    unsigned int channels = 2;        // Estéreo
    double masterGain = 0.3;          // Volumen principal (30%)
    int    octaveOffset = 0;          // Transposición de octavas
    Waveform waveform = Waveform::Sine; // Forma de onda actual
    bool   chorus = false;            // Flag para futuro efecto chorus
};
```

**Clase VirtualPiano:**
- **Miembros privados**: `window` (GLFW), `audio` (RtAudio), `params`, `keyToFreqBase` (mapeo teclas), `activeNotes` (notas sonando)
- **Sincronización**: `std::mutex mtx` para comunicación UI ↔ Audio thread
- **VU Meters**: `std::atomic<float> meterRms/Peak` para visualización thread-safe
- **Métodos públicos**: `initialize()`, `run()`, `cleanup()`
- **Callbacks estáticos**: `s_keyCallback()`, `s_audioCallback()` (wrappers para GLFW/RtAudio)

---

#### **`src/VirtualPiano.cpp`** (402 líneas) - **ARCHIVO PRINCIPAL**

**Sección 1: Optimizaciones SSE (líneas 1-8)**
```cpp
#if defined(__SSE__)
  #include <xmmintrin.h>
  #include <pmmintrin.h>
#endif
```
Incluye headers para optimizaciones SIMD en arquitecturas x86.

**Sección 2: Función auxiliar OpenGL (líneas 10-18)**
```cpp
static void drawRect(float x, float y, float w, float h, bool filled=true) {
    if (filled) glBegin(GL_QUADS);    // OpenGL legacy mode
    else        glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);                 // Definir vértices del rectángulo
    // ...
    glEnd();
}
```
**Función**: Dibuja rectángulos usando OpenGL legacy (deprecated en macOS moderno).

**Sección 3: Inicialización Principal (líneas 20-95)**
```cpp
bool VirtualPiano::initialize() {
    // GLFW setup
    glfwInit();
    window = glfwCreateWindow(900, 300, "Piano Virtual - QWERTY", nullptr, nullptr);
    glfwSetKeyCallback(window, &VirtualPiano::s_keyCallback);
    
    // Optimizaciones SSE
    #if defined(__SSE__)
      _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);      // Evitar denormalizados
      _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    #endif
    
    // Selección de API de audio (Linux específico)
    RtAudio::Api chosen = RtAudio::UNSPECIFIED;
    if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_PULSE) != apis.end()) {
        chosen = RtAudio::LINUX_PULSE;               // Prefiere PulseAudio
    } else if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_ALSA) != apis.end()) {
        chosen = RtAudio::LINUX_ALSA;                // Fallback a ALSA
    }
```

**Configuración de Audio:**
```cpp
RtAudio::StreamParameters oParams;
oParams.deviceId = audio->getDefaultOutputDevice();
oParams.nChannels = params.channels;              // 2 (estéreo)

RtAudio::StreamOptions options;
options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME;
options.numberOfBuffers = 2;                     // Double buffering
```

**Sección 4: Mapeo de Teclas (líneas 97-116)**
```cpp
void VirtualPiano::setupKeymap() {
    keyToFreqBase = {
        {GLFW_KEY_A, 261.63},    // C4
        {GLFW_KEY_W, 277.18},    // C#4
        {GLFW_KEY_S, 293.66},    // D4
        // ... más mapeos QWERTY → Frecuencias Hz
    };
}
```
**Función**: Mapea teclas QWERTY a frecuencias musicales (escala de C4 a B4).

**Sección 5: Bucle Principal (líneas 118-125)**
```cpp
void VirtualPiano::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();        // Procesar eventos de entrada
        drawUI();               // Renderizar interfaz
        glfwSwapBuffers(window); // Present frame
    }
}
```

**Sección 6: Manejo de Eventos (líneas 139-192)**
```cpp
void VirtualPiano::keyCallback(int key, int action) {
    if (action == GLFW_PRESS) {
        // Controles especiales
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, 1);
        if (key == GLFW_KEY_RIGHT) params.masterGain += 0.05;  // Volumen +
        if (key == GLFW_KEY_UP) params.octaveOffset++;         // Octava +
        if (key == GLFW_KEY_1) params.waveform = Waveform::Sine; // Cambiar forma
        
        // Activar nota musical
        auto it = keyToFreqBase.find(key);
        if (it != keyToFreqBase.end()) {
            std::lock_guard<std::mutex> lock(mtx);      // Sincronización
            Note& n = activeNotes[key];
            n.alive = true;
            n.pressed = true;
            n.stage = EnvStage::Attack;                 // Iniciar ADSR
            double freq = it->second * std::pow(2.0, params.octaveOffset);
            n.setFrequency(freq, params.sampleRate);
        }
    }
    // GLFW_RELEASE: iniciar Release en ADSR
}
```

**Sección 7: Audio Callback - CORAZÓN DEL SINTETIZADOR (líneas 202-252)**
```cpp
int VirtualPiano::audioCallback(float* out, unsigned int nFrames) {
    std::fill(out, out + nFrames * params.channels, 0.0f);  // Limpiar buffer
    
    std::lock_guard<std::mutex> lock(mtx);                  // Sincronizar con UI
    
    for (unsigned int i = 0; i < nFrames; ++i) {
        double mix = 0.0;
        
        // MIXER: Sumar todas las notas activas
        for (auto& kv : activeNotes) {
            Note& n = kv.second;
            if (!n.alive) continue;
            mix += n.nextSample(params.sampleRate);          // Generar sample
        }
        
        mix *= params.masterGain;                           // Aplicar volumen
        
        // SOFT-KNEE LIMITER (prevenir clipping)
        const double thr = 0.90, knee = 0.10;
        double a = std::fabs(mix);
        if (a > thr) {
            double t = std::min(1.0, (a - thr) / knee);
            double comp = thr + knee * (t - (t*t*t)/3.0);   // Compresión suave
            mix = (mix < 0.0 ? -comp : comp);
        }
        
        // Hard clipping como último recurso
        mix = std::clamp(mix, -1.0, 1.0);
        
        // Escribir a buffer estéreo
        float s = static_cast<float>(mix);
        out[params.channels * i + 0] = s;      // Left
        if (params.channels > 1) out[params.channels * i + 1] = s;  // Right
        
        // Calcular métricas para VU meters
        if (std::fabs(mix) > peakAbs) peakAbs = std::fabs(mix);
        rmsAcc += mix * mix;
    }
    
    // Actualizar VU meters (thread-safe)
    meterRms.store(std::sqrt(rmsAcc / nFrames), std::memory_order_relaxed);
    meterPeak.store(peakAbs, std::memory_order_relaxed);
    
    // Garbage collection de notas muertas
    std::vector<int> toErase;
    for (auto& kv : activeNotes) {
        if (!kv.second.alive && !kv.second.pressed) toErase.push_back(kv.first);
    }
    for (int k : toErase) activeNotes.erase(k);
}
```

**Sección 8: Sistema de Renderizado UI (líneas 254-350)**
```cpp
void VirtualPiano::drawUI() {
    // Setup viewport y proyección ortogonal
    glViewport(0, 0, w, h);
    glOrtho(0, w, h, 0, -1, 1);                    // 2D projection
    
    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);       // Fondo azul oscuro
    glClear(GL_COLOR_BUFFER_BIT);
    
    drawKeyboard(w, h);                            // Teclado visual
    
    // Barra de volumen
    glColor3f(0.2f, 0.8f, 0.4f);                  // Verde
    drawRect(20, 20, params.masterGain * (w - 40), 12, true);
    
    drawMeters(w, h);                              // VU meters
    
    // Debug info cada 1.5 segundos
    static double lastPrint = 0.0;
    if (glfwGetTime() - lastPrint > 1.5) {
        std::printf("[Piano] vol=%.2f octave=%+d wave=%s\n", ...);
    }
}
```

**Sección 9: Renderizado de Teclado (líneas 288-329)**
```cpp
void VirtualPiano::drawKeyboard(int width, int height) {
    // Teclas blancas (naturales)
    int whites[7] = { GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, 
                      GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J };
    
    for (int i = 0; i < 7; ++i) {
        bool on = activeNotes.find(whites[i]) != activeNotes.end() && 
                  activeNotes[whites[i]].pressed;
        
        glColor3f(on ? 0.9f : 0.95f, on ? 0.9f : 0.95f, on ? 0.3f : 0.95f);
        drawRect(x0 + i * whiteW, y0, whiteW - 2.0f, whiteH, true);
    }
    
    // Teclas negras (sostenidos) - posicionadas entre blancas
    struct BK { int key; int posIndex; };
    BK blacks[] = { {GLFW_KEY_W, 0}, {GLFW_KEY_E, 1}, ... };
    
    for (auto& b : blacks) {
        float bx = x0 + (b.posIndex + 1) * whiteW - blackW * 0.5f;  // Centrar
        // Renderizar con color según estado pressed
    }
}
```

**Sección 10: VU Meters (líneas 331-365)**
```cpp
void VirtualPiano::drawMeters(int width, int height) {
    float rms  = meterRms.load(std::memory_order_relaxed);   // Thread-safe read
    float peak = meterPeak.load(std::memory_order_relaxed);
    
    // Dibujar barras verticales (RMS left, Peak right)
    auto setColor = [](float v){
        if (v >= 0.90f) glColor3f(0.95f, 0.30f, 0.30f);      // Rojo (loud)
        else if (v >= 0.70f) glColor3f(0.95f, 0.80f, 0.30f); // Amarillo (medium)
        else glColor3f(0.30f, 0.85f, 0.40f);                 // Verde (quiet)
    };
    
    setColor(rms);
    drawRect(x + 2.f, y + (barH - rmsH), (barW * 0.5f) - 3.f, rmsH, true);
}
```

---

#### **`src/Note.hpp`** (82 líneas) - **SINTETIZADOR POR NOTA**

**Enumeraciones:**
```cpp
enum class Waveform { Sine = 0, Square = 1, Triangle = 2, Saw = 3 };
enum class EnvStage { Idle, Attack, Decay, Sustain, Release };
```

**Estructura Principal:**
```cpp
struct Note {
    // Oscilador
    double frequency = 440.0;      // Hz
    double phase = 0.0;            // Fase actual [0, 2π]
    double phaseInc = 0.0;         // Δphase por sample
    
    // Estado
    bool pressed = false;          // Input del usuario
    bool alive = false;            // Generando audio
    
    // ADSR Envelope
    EnvStage stage = EnvStage::Idle;
    double env = 0.0;              // Multiplicador [0,1]
    double attackSec  = 0.010;     // 10ms attack
    double decaySec   = 0.060;     // 60ms decay
    double sustainLvl = 0.70;      // 70% sustain level
    double releaseSec = 0.120;     // 120ms release
```

**Método Principal - nextSample():**
```cpp
inline double nextSample(double sampleRate) {
    // 1. Calcular steps de envolvente
    const double aStep = 1.0 / (attackSec * sampleRate);
    
    // 2. State machine ADSR
    switch (stage) {
        case EnvStage::Attack:
            env += aStep;
            if (env >= 1.0) { env = 1.0; stage = EnvStage::Decay; }
            break;
        case EnvStage::Decay:
            env -= dStep * (1.0 - sustainLvl);
            if (env <= sustainLvl) { env = sustainLvl; stage = EnvStage::Sustain; }
            break;
        // ... Release, Sustain
    }
    
    // 3. Avanzar oscilador
    phase += phaseInc;
    if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
    
    // 4. Generar forma de onda
    switch (wave) {
        case Waveform::Sine:     x = std::sin(phase); break;
        case Waveform::Square:   x = (std::sin(phase) >= 0.0 ? 1.0 : -1.0); break;
        case Waveform::Triangle: /* fórmula matemática */ break;
        case Waveform::Saw:      /* rampa diente de sierra */ break;
    }
    
    // 5. Normalización RMS (volumen consistente)
    double g = getRMSGain(wave);              // Factor de corrección
    
    // 6. Aplicar envolvente + headroom
    return amplitude * env * (0.90 * g * x);  // 10% headroom
}
```

---

#### **`cmake/FetchRtAudioPatched.cmake`** (28 líneas)

**Función Principal:**
```cpp
function(fetch_rtaudio_patched)
    // 1. Descargar RtAudio 6.0.1 desde GitHub
    FetchContent_Populate(rtaudio_direct
        URL "https://github.com/thestk/rtaudio/archive/refs/tags/6.0.1.zip"
    )
    
    // 2. PARCHE: Arreglar compatibilidad JACK
    file(READ "${cmake_file}" contents)
    string(REGEX_REPLACE 
        "list\\(APPEND LINKLIBS \\$\\{jack_LIBRARIES\\}\\)"      // Patrón original
        "if(jack_LINK_LIBRARIES)..."                             // Reemplazo condicional
        contents "${contents}"
    )
    file(WRITE "${cmake_file}" "${contents}")
    
    // 3. Configurar flags de compilación
    set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(rtaudio)
endfunction()
```
**Función**: Descarga RtAudio y aplica un parche para solucionar problemas de linkeo con JACK en sistemas modernos.

---

#### **`CMakeLists.txt`** (50 líneas)

**Configuración del Proyecto:**
```cmake
cmake_minimum_required(VERSION 3.15)
project(PianoTiles LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Importar función personalizada de parche
include(FetchRtAudioPatched)

# Configurar APIs de RtAudio para Linux
set(RTAUDIO_API_ALSA  ON  CACHE BOOL "" FORCE)    # ALSA support
set(RTAUDIO_API_PULSE ON  CACHE BOOL "" FORCE)    # PulseAudio support  
set(RTAUDIO_API_JACK  OFF CACHE BOOL "" FORCE)    # Desactivar JACK

fetch_rtaudio_patched()                            # Descargar + parchar
```

**Manejo de GLFW:**
```cmake
find_package(glfw3 QUIET)                         # Buscar en sistema

if (NOT glfw3_FOUND)                              # Si no existe
    set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE)   # X11 backend
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)   # No Wayland
    set(GLFW_BUILD_EXAMPLES OFF ...)                  # Sin extras
    
    FetchContent_Declare(glfw ...)                    # Descargar del source
    FetchContent_MakeAvailable(glfw)
endif()
```

**Target Final:**
```cmake
add_executable(${PROJECT_NAME}                    # PianoTiles executable
  src/main.cpp
  src/VirtualPiano.cpp                           # Note.hpp es header-only
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    rtaudio                                      # Audio engine
    glfw                                         # Windowing + input
    OpenGL::GL                                   # Graphics
    Threads::Threads                             # std::mutex support
)
```

### 🔧 Interacciones Entre Componentes

1. **main.cpp** → crea **VirtualPiano** → llama `initialize()`
2. **VirtualPiano::initialize()** → setup GLFW + RtAudio + mapeo teclas
3. **Callback loop**: GLFW events → `keyCallback()` → modifica `activeNotes`
4. **Audio thread**: RtAudio → `audioCallback()` → lee `activeNotes` → genera audio
5. **Render loop**: `drawUI()` → lee `activeNotes` + `meterRms/Peak` → OpenGL
6. **Note objects**: `nextSample()` → ADSR state machine + oscilador → sample output

### 📊 Flujo de Datos en Tiempo Real

> Ya habiendo repasado los diferentes componentes del proyecto será más fácil comprender su flujo de datos y cómo trabaja cada interacción.

```
[User Keypress] → [GLFW] → [keyCallback] → [activeNotes + mutex] 
                                               ↓
[Speakers] ← [RtAudio] ← [audioCallback] ← [Note::nextSample()]
                                               ↓
[VU Display] ← [drawMeters] ← [atomic meters] ← [RMS/Peak calc]
```

**Puntos Críticos de Sincronización:**
- `std::mutex mtx`: Protege `activeNotes` entre UI thread y audio thread
- `std::atomic<float>`: VU meters thread-safe sin bloquear audio
- Lock-free en audio callback: Solo lee atomics, minimiza `mutex` time

## Disfruten de su Piano Virtual 🎹 
