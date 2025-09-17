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

