# Piano Virtual (C++ · GLFW · RtAudio)

Un sintetizador de piano virtual en tiempo real que permite tocar música usando el teclado QWERTY de la computadora. Implementado en C++ con renderizado OpenGL (GLFW) y síntesis de audio de baja latencia (RtAudio).

## 🎹 Características Principales

- **Polifonía completa** - Toca múltiples notas simultáneamente
- **Síntesis en tiempo real** - 4 formas de onda (Sine, Square, Triangle, Saw)
- **Envolvente ADSR** - Ataques y decaimientos suaves por nota
- **Baja latencia** - < 20ms usando buffer de 512 frames @ 44.1kHz
- **VU Meters** - Visualización RMS y Peak en tiempo real
- **Interfaz visual** - Teclado de piano interactivo con feedback visual
- **Control dinámico** - Volumen, octavas y formas de onda en vivo
- **Efecto Chorus** - Espacialización estéreo con modulación LFO

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
- **Chorus:** `C` (Alternar efecto espacial)
- **Salir:** `ESC`

## 🏗️ Arquitectura y Decisiones de Diseño

### Arquitectura de Audio
```
[Teclado QWERTY] → [Note Objects] → [Mixer] → [Chorus/Effects] → [Soft Limiter] → [RtAudio] → [Speakers]
```

**Flujo de procesamiento:**
1. **Input**: Callbacks de GLFW capturan eventos de teclado
2. **Síntesis**: Cada nota genera samples usando osciladores + ADSR
3. **Mixing**: Suma de todas las voces activas por frame
4. **Effects**: Aplicación opcional de chorus estéreo
5. **Processing**: Soft-knee limiting para prevenir clipping
6. **Output**: Buffer estéreo float32 enviado a RtAudio

### Decisiones Técnicas Clave

#### **Latencia y Performance**
- **Sample Rate**: 44.1kHz fijo (estándar de audio)
- **Buffer Size**: 512 frames (≈11.6ms de latencia)
- **Threading**: Audio callback en thread de alta prioridad (RT)
- **Lock Strategy**: Mutex mínimo para sincronización estado UI ↔ Audio

#### **Síntesis de Audio**
- **ADSR por Nota**: Attack=10ms, Decay=60ms, Sustain=70%, Release=150ms
- **Normalización por Forma**: Cada waveform optimizada para volumen consistente
- **Headroom**: 20% de margen para prevenir saturación
- **Polifonía**: Sin límite artificial (limitado por CPU)

#### **Efectos de Audio**
- **Chorus Estéreo**: Dual LFO (0.8Hz L, 1.2Hz R) con delay modulado
- **Delay Base**: 18ms ± 6ms de modulación por canal
- **Interpolación**: Linear entre samples para suavidad
- **Mix Level**: 70% dry + 30% wet para balance natural

#### **Compatibilidad Multiplataforma**
- **Linux**: PulseAudio (preferido) → ALSA (desactivado por defecto)
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

# Compilar
mkdir build && cd build
cmake .. && make -j
./PianoTiles
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
| Polyphony | Unlimited* | *Limitado por CPU (~20-50 voces típico) |
| Chorus Delay | 18ms ± 6ms | Rango óptimo para espacialización |

## 🧬 Estructura del Código y Explicación de Archivos

```
PianoTiles/
├── CMakeLists.txt                    # Sistema de build principal
├── README.md                         # Documentación del proyecto
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
> **Función**: Entry point minimalista que instancia la aplicación y maneja el ciclo de vida completo del programa.

---

#### **`src/VirtualPiano.hpp`** - **INTERFAZ PRINCIPAL**

**Imports y Configuración:**
```cpp
#ifndef GLFW_INCLUDE_NONE            
#define GLFW_INCLUDE_NONE
#endif

#if defined(__APPLE__)               
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif
```

> **Configuración multiplataforma**: Headers adaptados según el sistema operativo para máxima compatibilidad.

**Estructura de Parámetros:**
```cpp
struct SynthParams {                 
    double sampleRate = 44100.0;     
    int bufferFrames = 512;          
    int channels = 2;                
    double masterGain = 0.5;         // Volumen inicial más conservador
    int octaveOffset = 0;            
    Waveform waveform = Waveform::Sine; 
    bool chorus = false;             // Nuevo: control de efectos
};
```

> **Centralización de configuración**: Todos los parámetros globales del sintetizador en una estructura, facilitando ajustes y experimentación.

**Clase VirtualPiano:**
- **Miembros de estado**: `window` (GLFW), `audio` (RtAudio), `params`, `keyFrequencies` (mapeo optimizado)
- **Gestión de notas**: `activeNotes` (hash map para acceso O(1)), `notesMutex` (sincronización)
- **VU Meters**: `currentRMS/Peak` para visualización en tiempo real
- **Efectos**: `delayBufferL/R`, `delayIndex` para implementación de chorus
- **Callbacks estáticos**: Wrappers para interfaz C de GLFW/RtAudio

---

#### **`src/VirtualPiano.cpp`** - **IMPLEMENTACIÓN PRINCIPAL**

**Sección 1: Utilidades de Renderizado**
```cpp
static void drawRect(float x, float y, float w, float h, bool filled=true) {
    if (filled) glBegin(GL_QUADS);    // OpenGL legacy mode
    else        glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);                 // Definir vértices del rectángulo
    // ...
    glEnd();
}
```
> **Abstracción OpenGL**: Función helper que simplifica el dibujo de rectángulos, tanto rellenos como contornos.

**Sección 2: Inicialización Principal**
```cpp
bool VirtualPiano::initialize() {
    // Setup GLFW window y contexto OpenGL
    glfwInit();
    window = glfwCreateWindow(700, 300, "Piano Tiles", nullptr, nullptr);
    glfwSetKeyCallback(window, &VirtualPiano::keyCallback);
    
    // Configuración RtAudio con parámetros optimizados
    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = audio.getDefaultOutputDevice();
    outputParams.nChannels = params.channels;
    
    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_MINIMIZE_LATENCY;     // Prioridad en latencia
    options.numberOfBuffers = 2;                 // Double buffering
    
    // Inicializar buffers para efectos
    int delaySamples = (int)(params.sampleRate * 0.04);  // 40ms max delay
    delayBufferL.resize(delaySamples, 0.0f);
    delayBufferR.resize(delaySamples, 0.0f);
}
```

> **Inicialización completa**: Setup de window, audio engine y buffers de efectos en una función centralizada.

**Sección 3: Mapeo de Teclas Simplificado**
```cpp
void VirtualPiano::setupKeyMapping() {
    keyFrequencies = {
        {GLFW_KEY_A, 261.63},    // C4
        {GLFW_KEY_W, 277.18},    // C#4
        {GLFW_KEY_S, 293.66},    // D4
        // ... más mapeos QWERTY → Frecuencias Hz
    };
}
```
> **Mapeo directo**: Hash map que relaciona teclas QWERTY con frecuencias musicales, eliminando cálculos complejos en tiempo real.

**Sección 4: Manejo de Eventos Unificado**
```cpp
void VirtualPiano::handleKeyEvent(int key, int action) {
    if (action == GLFW_PRESS) {
        // Controles especiales (volumen, octavas, waveforms, efectos)
        if (key == GLFW_KEY_RIGHT) params.masterGain += 0.05;
        if (key == GLFW_KEY_C) params.chorus = !params.chorus;  // Toggle chorus
        
        // Activar notas musicales
        auto it = keyFrequencies.find(key);
        if (it != keyFrequencies.end()) {
            std::lock_guard<std::mutex> lock(notesMutex);
            Note& note = activeNotes[key];
            note.startNote(it->second * std::pow(2.0, params.octaveOffset), 
                          params.sampleRate, params.waveform);
        }
    }
    // GLFW_RELEASE: stopNote() para envolvente release
}
```

> **Gestión centralizada**: Todos los eventos de teclado procesados en una función, separando controles de parámetros de notas musicales.

**Sección 5: Audio Callback - MOTOR DEL SINTETIZADOR**
```cpp
int VirtualPiano::processAudio(float* outputBuffer, int frameCount) {
    std::fill(outputBuffer, outputBuffer + frameCount * params.channels, 0.0f);
    std::lock_guard<std::mutex> lock(notesMutex);
    
    for (int frame = 0; frame < frameCount; ++frame) {
        double mixedSample = 0.0;
        
        // MIXER: Sumar todas las notas activas
        for (auto& [key, note] : activeNotes) {
            if (note.isActive()) {
                mixedSample += note.getSample();
            }
        }
        
        mixedSample *= params.masterGain;
        
        // SOFT-KNEE LIMITER (prevenir clipping)
        double absLevel = std::abs(mixedSample);
        if (absLevel > 0.9) {
            double excess = (absLevel - 0.9) / 0.1;
            double compressed = 0.9 + 0.1 * (excess - (excess*excess*excess)/3.0);
            mixedSample = (mixedSample < 0) ? -compressed : compressed;
        }
        
        // CHORUS EFFECT (cuando está activado)
        if (params.chorus && params.channels >= 2) {
            applyChorusEffect(mixedSample, frame, outputBuffer);
        } else {
            // Salida directa estéreo/mono
            outputBuffer[frame * params.channels + 0] = mixedSample;
            if (params.channels > 1) {
                outputBuffer[frame * params.channels + 1] = mixedSample;
            }
        }
        
        // Calcular métricas RMS/Peak para VU meters
        updateVUMeters(mixedSample);
    }
    
    // Garbage collection de notas inactivas
    cleanupInactiveNotes();
}
```

> **Motor de audio**: Función principal que procesa cada frame de audio, combinando síntesis, efectos, limitación y métricas.

**Sección 6: Implementación de Chorus**
```cpp
// Dentro del audio callback cuando chorus está activo
if (params.chorus && params.channels >= 2 && !delayBufferL.empty()) {
    static double lfoPhaseL = 0.0, lfoPhaseR = 0.0;
    const double lfoFreqL = 0.8, lfoFreqR = 1.2;           // Frecuencias LFO diferentes
    
    const float baseDelay = 18.0f;                         // Delay base en ms
    const float depthDelay = 6.0f;                         // Profundidad modulación
    
    // Calcular delays modulados por LFO
    float delayTimeL = baseDelay + depthDelay * std::sin(lfoPhaseL);
    float delayTimeR = baseDelay + depthDelay * std::sin(lfoPhaseR);
    
    // Escritura circular en buffers
    delayBufferL[delayIndex] = sample;
    delayBufferR[delayIndex] = sample;
    
    // Lectura interpolada de delays
    float delayedL = readDelay(delayBufferL, delayIndex, delaySamplesL);
    float delayedR = readDelay(delayBufferR, delayIndex, delaySamplesR);
    
    // Mix dry/wet y salida estéreo
    const float dryLevel = 0.7f, wetLevel = 0.3f;
    outputBuffer[frame*2+0] = dryLevel*sample + wetLevel*delayedL;
    outputBuffer[frame*2+1] = dryLevel*sample + wetLevel*delayedR;
}
```

> **Chorus estéreo**: Implementación de efecto espacial usando dual LFO con frecuencias ligeramente diferentes para crear movimiento estéreo natural.

**Sección 7: Sistema de Renderizado UI**
```cpp
void VirtualPiano::renderUI() {
    // Setup viewport y proyección ortogonal 2D
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glOrtho(0, w, h, 0, -1, 1);
    
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);       // Fondo azul oscuro
    glClear(GL_COLOR_BUFFER_BIT);
    
    drawKeyboard(w, h);                           // Teclado visual interactivo
    drawVUMeters(w, h);                           // Medidores de nivel
    
    // Barra de volumen dinámica
    glColor3f(0.2f, 0.8f, 0.4f);
    drawRect(20, 20, (float)(params.masterGain * (w-40)), 15, true);
    
    // Debug info periódico
    static double lastPrint = 0.0;
    if (glfwGetTime() - lastPrint > 2.0) {
        printStatusInfo();
        lastPrint = glfwGetTime();
    }
}
```

> **Interfaz visual**: Sistema de renderizado 2D que combina teclado interactivo, medidores de nivel y controles visuales.

**Sección 8: Renderizado de Teclado Mejorado**
```cpp
void VirtualPiano::drawKeyboard(int width, int height) {
    // Teclas blancas con estado visual
    int whiteKeys[] = { GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, 
                        GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J };
    
    bool whitePressed[7] = {false};
    {
        std::lock_guard<std::mutex> lock(notesMutex);
        for (int i = 0; i < 7; ++i) {
            auto it = activeNotes.find(whiteKeys[i]);
            whitePressed[i] = (it != activeNotes.end() && it->second.isPressed());
        }
    }
    
    // Renderizar teclas blancas con feedback visual
    for (int i = 0; i < 7; ++i) {
        bool pressed = whitePressed[i];
        glColor3f(pressed ? 0.9f : 0.95f, pressed ? 0.9f : 0.95f, 
                  pressed ? 0.3f : 0.95f);  // Amarillo cuando presionado
        drawRect(x + i * keyWidth, y, keyWidth - 2.0f, keyHeight, true);
    }
    
    // Teclas negras con posicionamiento preciso
    struct BlackKey { int keyCode; int position; };
    BlackKey blackKeys[] = { {GLFW_KEY_W, 0}, {GLFW_KEY_E, 1}, 
                            {GLFW_KEY_T, 3}, {GLFW_KEY_Y, 4}, {GLFW_KEY_U, 5} };
    
    for (auto& bk : blackKeys) {
        bool pressed = checkBlackKeyPressed(bk.keyCode);
        float x = calculateBlackKeyPosition(bk.position);
        renderBlackKey(x, y, pressed);
    }
}
```

> **Teclado visual avanzado**: Renderizado preciso de teclas blancas y negras con feedback visual en tiempo real y posicionamiento matemático correcto.

**Sección 9: VU Meters con Código de Color**
```cpp
void VirtualPiano::drawVUMeters(int w, int h) {
    float rms = std::clamp(currentRMS, 0.0f, 1.0f);
    float peak = std::clamp(currentPeak, 0.0f, 1.0f);
    
    // Función lambda para colores dinámicos
    auto setMeterColor = [](float level) {
        if (level >= 0.9f) glColor3f(0.95f, 0.3f, 0.3f);      // Rojo (loud)
        else if (level >= 0.7f) glColor3f(0.95f, 0.8f, 0.3f); // Amarillo (medium)
        else glColor3f(0.3f, 0.85f, 0.4f);                    // Verde (quiet)
    };
    
    // Dual meters: RMS (left) y Peak (right)
    setMeterColor(rms);
    drawRect(meterX + 2.0f, meterY + (meterHeight - rmsHeight), 
             (meterWidth * 0.5f) - 3.0f, rmsHeight, true);
    
    setMeterColor(peak);
    drawRect(meterX + (meterWidth * 0.5f) + 1.0f, meterY + (meterHeight - peakHeight), 
             (meterWidth * 0.5f) - 3.0f, peakHeight, true);
}
```

> **Medidores profesionales**: VU meters duales con código de colores que indican niveles de señal RMS y Peak para monitoreo profesional.

---

#### **`src/Note.hpp`** - **SINTETIZADOR POR NOTA OPTIMIZADO**

**Enumeraciones y Constantes:**
```cpp
enum class Waveform { Sine = 0, Square = 1, Triangle = 2, Saw = 3 };
enum class EnvelopeState { Attack, Decay, Sustain, Release, Idle };

class Note {
private:
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double TWO_PI = 2.0 * PI;
```

> **Definiciones precisas**: Constantes matemáticas y enumeraciones que definen el comportamiento del sintetizador por nota.

**Estado del Oscilador:**
```cpp
// Oscilador
double frequency = 440.0;      // Hz
double phase = 0.0;            // Fase actual [0, 2π]
double phaseIncrement = 0.0;   // Δphase por sample

// Estado de la nota
bool pressed = false;          // Input del usuario
bool active = false;           // Generando audio
```

> **Gestión de estado**: Variables que controlan tanto el oscilador como el estado de vida de cada nota.

**Sistema ADSR Mejorado:**
```cpp
// Envelope (ADSR)
EnvelopeState envelopeState = EnvelopeState::Idle;
double envelopeLevel = 0.0;

// Timing ajustado para mayor expresividad
double attackTime = 0.01;      // 10ms - ataque rápido
double decayTime = 0.06;       // 60ms - decay musical
double sustainLevel = 0.7;     // 70% - sustain natural
double releaseTime = 0.15;     // 150ms - release más largo
```

> **ADSR musical**: Parámetros de envolvente ajustados para sonido más expresivo y natural.

**Generación de Formas de Onda:**
```cpp
double generateWaveform() {
    switch (currentWaveform) {
        case Waveform::Sine:
            return std::sin(phase);
            
        case Waveform::Square:
            return (std::sin(phase) >= 0.0) ? 1.0 : -1.0;
            
        case Waveform::Triangle: {
            double t = phase / TWO_PI;
            return 4.0 * std::abs(t - std::floor(t + 0.5)) - 1.0;
        }
        
        case Waveform::Saw: {
            double t = phase / TWO_PI;
            return 2.0 * (t - std::floor(t + 0.5));
        }
    }
}
```

> **Osciladores matemáticos**: Implementación pura de formas de onda usando funciones matemáticas optimizadas.

**Máquina de Estados ADSR:**
```cpp
void updateEnvelope(double sampleRate) {
    double stepSize = 1.0 / sampleRate;
    
    switch (envelopeState) {
        case EnvelopeState::Attack:
            envelopeLevel += stepSize / attackTime;
            if (envelopeLevel >= 1.0) {
                envelopeLevel = 1.0;
                envelopeState = EnvelopeState::Decay;
            }
            break;
        // ... otros estados
    }
}
```

> **State machine**: Control preciso de la envolvente ADSR usando una máquina de estados para transiciones suaves.

**Función Principal getSample():**
```cpp
double getSample() {
    if (!active) return 0.0;
    
    updateEnvelope(44100.0);              // Actualizar envolvente
    double waveValue = generateWaveform(); // Generar forma de onda
    
    // Avanzar fase del oscilador
    phase += phaseIncrement;
    if (phase >= TWO_PI) phase -= TWO_PI;
    
    // Aplicar envolvente y amplitud con headroom
    return amplitude * envelopeLevel * waveValue * 0.8;  // 20% headroom
}
```

> **Síntesis completa**: Función que combina oscilación, envolvente y control de amplitud en cada sample.

---

#### **`CMakeLists.txt`** - **SISTEMA DE BUILD MULTIPLATAFORMA**

**Configuración del Proyecto:**
```cmake
cmake_minimum_required(VERSION 3.15)
project(PianoTiles LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Configuración de build por defecto
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
```

> **Setup moderno**: CMake 3.15+ con C++17 y configuración Release por defecto para performance óptimo.

**Configuración RtAudio por Plataforma:**
```cmake
# APIs específicas por plataforma para latencia óptima
if(WIN32)
    set(RTAUDIO_API_WASAPI ON CACHE BOOL "" FORCE)      # Windows: WASAPI
elseif(APPLE)
    set(RTAUDIO_API_CORE ON CACHE BOOL "" FORCE)        # macOS: CoreAudio
else() # Linux
    set(RTAUDIO_API_PULSE ON CACHE BOOL "" FORCE)       # Linux: PulseAudio preferido
    set(RTAUDIO_API_ALSA OFF CACHE BOOL "" FORCE)       # ALSA desactivado
endif()

set(RTAUDIO_API_JACK OFF CACHE BOOL "" FORCE)          # JACK desactivado globalmente
```

> **Optimización por plataforma**: Selección automática de APIs de audio nativas para cada sistema operativo.

**Gestión Automática de Dependencias:**
```cmake
# GLFW: Buscar en sistema, sino descargar
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW not found in system, fetching from source")
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)
endif()

# RtAudio: Siempre desde source para control de versión
FetchContent_Declare(rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG 6.0.1
)
FetchContent_MakeAvailable(rtaudio)
```

> **Dependencias híbridas**: Sistema inteligente que usa librerías del sistema cuando están disponibles, sino las descarga automáticamente.

**Target y Linkeo Final:**
```cmake
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/VirtualPiano.cpp
    # Note.hpp es header-only
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    rtaudio                           # Motor de audio
    ${GLFW_TARGET}                    # Sistema de ventanas
    OpenGL::GL                        # Renderizado gráfico
    Threads::Threads                  # Soporte para std::mutex
)

# Frameworks específicos de macOS
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "-framework Cocoa" "-framework IOKit" "-framework CoreVideo"
    )
endif()
```

> **Linkeo completo**: Configuración final que enlaza todas las dependencias necesarias con soporte específico para cada plataforma.

### 🔧 Interacciones Entre Componentes

1. **main.cpp** → crea **VirtualPiano** → llama `initialize()`
2. **VirtualPiano::initialize()** → setup GLFW + RtAudio + mapeo teclas + buffers efectos
3. **Event loop**: GLFW events → `handleKeyEvent()` → modifica `activeNotes` con mutex
4. **Audio thread**: RtAudio → `processAudio()` → lee `activeNotes` → genera audio + efectos
5. **Render loop**: `renderUI()` → lee `activeNotes` + `currentRMS/Peak` → OpenGL
6. **Note objects**: `getSample()` → ADSR state machine + oscilador → sample output
7. **Effects chain**: Chorus → dual LFO + delay buffers → espacialización estéreo

### 📊 Flujo de Datos en Tiempo Real

```
[User Keypress] → [GLFW] → [handleKeyEvent] → [activeNotes + mutex] 
                                                   ↓
[Speakers] ← [RtAudio] ← [processAudio] ← [Note::getSample() + Chorus]
                                                   ↓
[VU Display] ← [drawVUMeters] ← [currentRMS/Peak] ← [RMS/Peak calc]
```

**Puntos Críticos de Sincronización:**
- `std::mutex notesMutex`: Protege `activeNotes` entre UI thread y audio thread
- Lock scope mínimo: Solo durante modificación/lectura de notas activas
- Lock-free VU meters: Variables simples `currentRMS/Peak` sin atomics
- Chorus buffers: Thread-safe por diseño (solo audio thread los modifica)

### 📝 Notas de Desarrollo

**Decisiones de Diseño Clave:**
1. **Header-only Note.hpp**: Permite inlining completo del hot path de síntesis
2. **Mutex granular**: Un solo mutex para `activeNotes`, resto lock-free
3. **OpenGL Legacy**: Compatible con hardware más antiguo, fácil debugging
4. **FetchContent**: Builds reproducibles sin dependencias del sistema
5. **Platform-specific APIs**: Latencia óptima en cada sistema operativo

**Lecciones Aprendidas:**
- **Audio thread**: Nunca bloquear, minimizar allocations dinámicas
- **Visual feedback**: 60 FPS es crucial para percepción de responsividad
- **Cross-platform**: Cada OS tiene quirks únicos en audio/graphics
- **Buffer sizing**: Trade-off fundamental latencia vs. estabilidad
- **User experience**: Controles intuitivos > funcionalidad compleja

### 🎹 ¡Disfruta de tu Piano Virtual Mejorado!

**¡Experimenta con los diferentes waveforms, activa el chorus, y crea tu propia música usando solo tu teclado QWERTY!** 🎵
