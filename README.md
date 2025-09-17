```markdown
# Piano Virtual (C++ · GLFW · RtAudio)

Proyecto de **síntesis en tiempo real** que implementa un piano virtual controlado con el teclado QWERTY.  
Renderiza una interfaz sencilla con **GLFW/OpenGL** y genera audio con **RtAudio**.  
Cumple con los requisitos del enunciado: **44.1 kHz**, **buffer de 512 frames**, **latencia baja (< 20 ms con doble buffer)**, **estéreo**, **float32**, polifonía, **envolvente ADSR**, formas de onda, control de volumen y salida con `ESC`.

> Más adelante integraremos aquí la **descripción de diseño y arquitectura** por archivo (cuando vayamos sección por sección).

---

## Características

- **Polifonía** (varias notas simultáneas).
- **Síntesis** por oscilador: Sine, Square, Triangle, Saw.
- **ADSR** (Attack, Decay, Sustain, Release) por nota para ataques/colas suaves.
- **Frecuencias**: C4..B4 (12 notas) mapeadas a teclas QWERTY.
- **VU meter** en UI (RMS + Peak) para visualizar nivel de salida.
- **Soft-knee** cerca de 0 dB para evitar recorte duro (mejor sonido).
- **Latencia** baja manteniendo **SR = 44.1 kHz** y **buffer = 512**.
- **Compatibilidad WSLg** (Windows Subsystem for Linux con PulseAudio).
- **CMake**: integra **RtAudio (Fetch + parche)** y **GLFW** (sistema o Fetch).

---

## Estructura del proyecto

```

PianoTiles/
├── CMakeLists.txt
├── cmake/
│   └── FetchRtAudioPatched.cmake
└── src/
├── main.cpp
├── VirtualPiano.hpp
├── VirtualPiano.cpp
└── Note.hpp

````

- **`Note.hpp`**: definición de una nota (oscilador + **ADSR** + forma de onda).
- **`VirtualPiano.hpp/.cpp`**: ventana GLFW, mapeo de teclas, **callback de audio** (RtAudio), render de UI (teclado + **VU**), estado y sincronización.
- **`main.cpp`**: arranque de la app y bucle principal.
- **`cmake/FetchRtAudioPatched.cmake`**: descarga RtAudio 6.x, aplica un ajuste de compatibilidad (JACK) y expone el target `rtaudio` para enlazar.

> En Linux la app **prefiere PulseAudio** y cae a ALSA si no está disponible.

---

## Dependencias

### Comunes
- **CMake ≥ 3.15**
- **C++17** (GCC/Clang/MSVC)
- **OpenGL** (headers de “desktop”)
- **GLFW 3.x** (sistema o descargado por CMake)
- **RtAudio 6.x** (descargado por CMake con parche local)

### Linux (Ubuntu / Debian / WSLg)
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
  libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libasound2-dev libpulse-dev libasound2-plugins pulseaudio-utils
# (Wayland opcional)
# sudo apt install -y libwayland-dev wayland-protocols libxkbcommon-dev
# (GLFW del sistema, opcional si no quieres compilarlo con FetchContent)
# sudo apt install -y libglfw3-dev
````

* **WSLg/Windows 11**: debería existir `PULSE_SERVER=unix:/mnt/wslg/PulseServer`.

### Windows (MSVC)

* **Visual Studio 2022** con CMake y C++17.
* **Opcional**: vcpkg para instalar GLFW si no usas FetchContent.
* Backends de RtAudio recomendados en Windows:

  ```cmake
  set(RTAUDIO_API_WASAPI ON  CACHE BOOL "" FORCE)   # Windows
  set(RTAUDIO_API_DS     OFF CACHE BOOL "" FORCE)
  set(RTAUDIO_API_ASIO   OFF CACHE BOOL "" FORCE)   # requiere SDK ASIO
  set(RTAUDIO_API_ALSA   OFF CACHE BOOL "" FORCE)
  set(RTAUDIO_API_PULSE  OFF CACHE BOOL "" FORCE)
  set(RTAUDIO_API_JACK   OFF CACHE BOOL "" FORCE)
  ```

  > En el código, solo fuerza `LINUX_PULSE/ALSA` bajo `#ifdef __linux__`.

### macOS

* Xcode Command Line Tools o Homebrew (`cmake`, `glfw`).
* RtAudio usa **CoreAudio** (actívalo en CMake si es necesario).
* El `CMakeLists.txt` enlaza frameworks (`Cocoa`, `IOKit`, `CoreVideo`) según plataforma.

---

## Compilación y ejecución

### Linux / WSLg

```bash
git clone <este-proyecto>
cd PianoTiles
mkdir build && cd build
cmake ..
cmake --build . -j
./PianoTiles
```

**Ajuste opcional WSLg (sin cambiar 44.1k/512):**

```bash
PULSE_LATENCY_MSEC=20 ./PianoTiles   # prueba 20–30 si oyes pops
```

### Windows (MSVC)

```powershell
# Abrir "x64 Native Tools Command Prompt for VS 2022"
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
.\build\Release\PianoTiles.exe
```

### macOS

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./PianoTiles
```

---

## Controles (teclado)

**Notas (C4..B4):**
`A  W  S  E  D  F  T  G  Y  H  U  J`

**Volumen master:** `+` / `-`
**Octava:** `↑` / `↓`
**Forma de onda:** `1` Sine, `2` Square, `3` Triangle, `4` Saw
**Salir:** `ESC`
**(Reserva)** Chorus `C` (activación para futuro extra).

> El **VU** se muestra como dos barras verticales en el borde derecho: **RMS** (izquierda) y **Peak** (derecha).

---

## Ejecución en tiempo real y latencia

* **Parámetros exigidos:**

  * `sampleRate = 44100 Hz`, `bufferFrames = 512` (≈ 11.6 ms por bloque).
  * Se persigue **< 20 ms**: se usa **doble buffer** y flags de baja latencia en RtAudio.
* **Flags de RtAudio** (en el código):
  `RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME`.
* **WSLg/Pulse**: si escuchas “pops”, lanza con `PULSE_LATENCY_MSEC=20` (no cambia SR ni buffer; ajusta el cliente Pulse).

---

## Funcionalidades de audio

* **ADSR por nota**: ataque/decay/sustain/release parametrizados para suavizar inicio/fin.
* **Formas**: seno, cuadrada, triangular, sierra (con normalización RMS opcional para igualar loudness).
* **Mezcla**: suma de voces por bloque, **soft-knee** cerca de 0 dB para evitar recorte duro.
* **Estéreo**: L/R con la misma señal (esta versión; extensible a efectos estéreo).

---

## Solución de problemas (FAQ)

**“No audio devices found!”**

* En Linux/WSLg instala plugins Pulse para ALSA y utilidades:

  ```bash
  sudo apt install -y libasound2-plugins pulseaudio-utils
  ```
* Verifica que WSLg exporte `PULSE_SERVER`:

  ```bash
  echo $PULSE_SERVER   # debería ser unix:/mnt/wslg/PulseServer
  ```

**Faltan headers OpenGL (`GL/gl.h`)**

```bash
sudo apt install -y libgl1-mesa-dev
```

**GLFW por X11: falta Xrandr / Wayland / etc.**

```bash
sudo apt install -y libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
# (Wayland) libwayland-dev wayland-protocols libxkbcommon-dev
```

**Pops / clics ocasionales**

* Observa si aparecen mensajes `[XRUN]` (underruns).
* Cierra aplicaciones pesadas, evita `printf` frecuentes.
* Prueba `PULSE_LATENCY_MSEC=20` o `30` en WSLg.

**Windows no compila audio**

* Activa **WASAPI** en CMake y no fuerces APIs de Linux en el código.

---

## Próximos pasos (diseño / arquitectura)

* Diagrama de bloques (UI, motor de audio, sincronización).
* Descripción de **`Note.hpp`** (oscilador + ADSR), **`VirtualPiano`** (callback, mixer, UI), modelo de **threading** y acceso concurrente.
* Decisiones: **soft-knee**, normalización por forma, flags de latencia, elección de backend según plataforma, tips WSLg.
* Extensiones: **chorus**, delay, reverb, VU con peak-hold.

---

## Licencia

Uso académico. (Ajusta aquí la licencia según requiera tu curso o tus preferencias.)

---

## Agradecimientos

* **RtAudio** — Gary P. Scavone & colaboradores
* **GLFW** — Marcus Geelnard, Camilla Löwy & contrib.

```

¿Quieres que agregue una sección inicial de **requisitos del enunciado** (checkbox) o prefieres que lo dejemos así de compacto?
::contentReference[oaicite:0]{index=0}
```
