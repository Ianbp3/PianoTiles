#if defined(__SSE__)
  #include <xmmintrin.h>
  #include <pmmintrin.h>
#endif
#include <vector>
#include "VirtualPiano.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>

static void drawRect(float x, float y, float w, float h, bool filled=true) {
    if (filled) glBegin(GL_QUADS);
    else        glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

bool VirtualPiano::initialize() {
    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW init failed\n");
        return false;
    }
    window = glfwCreateWindow(900, 300, "Piano Virtual - QWERTY", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "GLFW window creation failed\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, &VirtualPiano::s_keyCallback);
    glfwSwapInterval(1);

    #if defined(__SSE__)
      _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
      _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    #endif

    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);

    RtAudio::Api chosen = RtAudio::UNSPECIFIED;
    if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_PULSE) != apis.end()) {
        chosen = RtAudio::LINUX_PULSE;
    } else if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_ALSA) != apis.end()) {
        chosen = RtAudio::LINUX_ALSA;
    }
    audio = std::make_unique<RtAudio>(chosen);

    if (audio->getDeviceCount() < 1) {
        std::fprintf(stderr, "No audio devices found via RtAudio.\n");
        return false;
    }

    RtAudio::StreamParameters oParams;
    oParams.deviceId = audio->getDefaultOutputDevice();
    oParams.nChannels = params.channels;
    oParams.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.streamName = "PianoVirtual";
    options.numberOfBuffers = 2;
    options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME;

    RtAudioFormat fmt = RTAUDIO_FLOAT32;

    try {
        audio->openStream(&oParams, nullptr, fmt,
                          static_cast<unsigned int>(params.sampleRate),
                          &params.bufferFrames,
                          &VirtualPiano::s_audioCallback, this, &options);
        audio->startStream();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "RtAudio std::exception: %s\n", e.what());
        return false;
    } catch (...) {
        std::fprintf(stderr, "RtAudio: excepción no identificada\n");
        return false;
    }

    setupKeymap();

    size_t delaySamples = static_cast<size_t>(params.sampleRate * 0.02);
    delayL.assign(delaySamples, 0.0f);
    delayR.assign(delaySamples, 0.0f);
    delayIdx = 0;

    return true;
}

void VirtualPiano::setupKeymap() {
    keyToFreqBase = {
        {GLFW_KEY_A, 261.63}, 
        {GLFW_KEY_W, 277.18}, 
        {GLFW_KEY_S, 293.66}, 
        {GLFW_KEY_E, 311.13}, 
        {GLFW_KEY_D, 329.63}, 
        {GLFW_KEY_F, 349.23}, 
        {GLFW_KEY_T, 369.99}, 
        {GLFW_KEY_G, 392.00}, 
        {GLFW_KEY_Y, 415.30}, 
        {GLFW_KEY_H, 440.00}, 
        {GLFW_KEY_U, 466.16}, 
        {GLFW_KEY_J, 493.88}  
    };
}

void VirtualPiano::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawUI();
        glfwSwapBuffers(window);
    }
}

void VirtualPiano::cleanup() {
    try {
        if (audio && audio->isStreamRunning()) audio->stopStream();
        if (audio && audio->isStreamOpen())    audio->closeStream();
    } catch (...) {}
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void VirtualPiano::s_keyCallback(GLFWwindow* w, int key, int sc, int action, int mods) {
    auto* self = static_cast<VirtualPiano*>(glfwGetWindowUserPointer(w));
    if (self) self->keyCallback(key, action);
}

void VirtualPiano::keyCallback(int key, int action) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, 1);
            return;
        }
        if (key == GLFW_KEY_RIGHT) {
            params.masterGain = std::min(1.0, params.masterGain + 0.05);
            return;
        }
        if (key == GLFW_KEY_LEFT) {
            params.masterGain = std::max(0.0, params.masterGain - 0.05);
            return;
        }

        if (key == GLFW_KEY_UP)   { params.octaveOffset = std::min(3,  params.octaveOffset + 1); return; }
        if (key == GLFW_KEY_DOWN) { params.octaveOffset = std::max(-3, params.octaveOffset - 1); return; }

        if (key == GLFW_KEY_1) { params.waveform = Waveform::Sine;     return; }
        if (key == GLFW_KEY_2) { params.waveform = Waveform::Square;   return; }
        if (key == GLFW_KEY_3) { params.waveform = Waveform::Triangle; return; }
        if (key == GLFW_KEY_4) { params.waveform = Waveform::Saw;      return; }

        if (key == GLFW_KEY_C) { params.chorus = !params.chorus; return; }

        auto it = keyToFreqBase.find(key);
        if (it != keyToFreqBase.end()) {
            std::lock_guard<std::mutex> lock(mtx);
            Note& n = activeNotes[key];
            n.alive = true;
            n.pressed = true;
            n.stage = EnvStage::Attack;
            n.env = 0.0;

            double freq = it->second * std::pow(2.0, params.octaveOffset);
            n.setFrequency(freq, params.sampleRate);
            n.wave = params.waveform;
            n.amplitude = 0.5;  
        }
    } else if (action == GLFW_RELEASE) {
        auto it = keyToFreqBase.find(key);
        if (it != keyToFreqBase.end()) {
            std::lock_guard<std::mutex> lock(mtx);
            auto nIt = activeNotes.find(key);
            if (nIt != activeNotes.end()) {
                nIt->second.pressed = false;
                nIt->second.stage = EnvStage::Release;
            }
        }
    }
}


int VirtualPiano::s_audioCallback(void* outputBuffer, void* inputBuffer,
                                  unsigned int nBufferFrames, double /*streamTime*/,
                                  RtAudioStreamStatus status, void* userData) {
    if (status) std::fprintf(stderr, "[XRUN] RtAudio=%u\n", status);
    auto* self = static_cast<VirtualPiano*>(userData);
    return self->audioCallback(static_cast<float*>(outputBuffer), nBufferFrames);
}
int VirtualPiano::audioCallback(float* out, unsigned int nFrames) {
    std::fill(out, out + nFrames * params.channels, 0.0f);

    std::lock_guard<std::mutex> lock(mtx);

    double rmsAcc = 0.0;
    double peakAbs = 0.0;

    for (unsigned int i = 0; i < nFrames; ++i) {
        double mix = 0.0;
        for (auto& kv : activeNotes) {
            Note& n = kv.second;
            if (!n.alive) continue;
            mix += n.nextSample(params.sampleRate);
        }

        mix *= params.masterGain;

        const double thr  = 0.90;  
        const double knee = 0.10;  
        double a = std::fabs(mix);
        if (a > thr) {
            double t = std::min(1.0, (a - thr) / knee);   
            double comp = thr + knee * (t - (t*t*t)/3.0);
            mix = (mix < 0.0 ? -comp : comp);
        }
        if (mix > 1.0) mix = 1.0;
        else if (mix < -1.0) mix = -1.0;

        float s = static_cast<float>(mix);
        out[params.channels * i + 0] = s;
        if (params.channels > 1) out[params.channels * i + 1] = s;

        double aa = (mix >= 0.0) ? mix : -mix;
        if (aa > peakAbs) peakAbs = aa;
        rmsAcc += mix * mix;
    }

    float rms = static_cast<float>(std::sqrt(rmsAcc / std::max(1u, nFrames)));
    if (rms < 0.0f) rms = 0.0f; else if (rms > 1.0f) rms = 1.0f;

    float pk = static_cast<float>(peakAbs);
    if (pk < 0.0f) pk = 0.0f; else if (pk > 1.0f) pk = 1.0f;

    meterRms.store(rms, std::memory_order_relaxed);
    meterPeak.store(pk, std::memory_order_relaxed);

    std::vector<int> toErase;
    toErase.reserve(activeNotes.size());
    for (auto& kv : activeNotes) {
        const Note& n = kv.second;
        if (!n.alive && !n.pressed) toErase.push_back(kv.first);
    }
    for (int k : toErase) activeNotes.erase(k);

    return 0;
}

void VirtualPiano::drawUI() {
    int w, h; 
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawKeyboard(w, h);

    glColor3f(0.2f, 0.8f, 0.4f);
    drawRect(20, 20, (float)(params.masterGain * (w - 40)), 12, true);
    glColor3f(1,1,1);
    drawRect(20, 20, (float)(w - 40), 12, false);

    drawMeters(w, h);

    static double lastPrint = 0.0;
    double now = glfwGetTime();
    if (now - lastPrint > 1.5) {
        lastPrint = now;
        const char* wf = (params.waveform == Waveform::Sine) ? "Sine" :
                         (params.waveform == Waveform::Square) ? "Square" :
                         (params.waveform == Waveform::Triangle) ? "Triangle" : "Saw";
        std::printf("[Piano] vol=%.2f  octave=%+d  wave=%s  chorus=%s\n",
            params.masterGain, params.octaveOffset, wf, params.chorus ? "ON" : "OFF");
        std::fflush(stdout);
    }
}

void VirtualPiano::drawKeyboard(int width, int height) {
    float x0 = 40.0f;
    float y0 = 60.0f;
    float whiteW = (width - 80.0f) / 7.0f;
    float whiteH = height - y0 - 40.0f;

    int whites[7] = { GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J };
    for (int i = 0; i < 7; ++i) {
        int key = whites[i];
        bool on = false;
        { std::lock_guard<std::mutex> lock(mtx);
          auto it = activeNotes.find(key);
          on = (it != activeNotes.end() && it->second.pressed);
        }
        glColor3f(on ? 0.9f : 0.95f, on ? 0.9f : 0.95f, on ? 0.3f : 0.95f);
        drawRect(x0 + i * whiteW, y0, whiteW - 2.0f, whiteH, true);
        glColor3f(0,0,0);
        drawRect(x0 + i * whiteW, y0, whiteW - 2.0f, whiteH, false);
    }

    float blackW = whiteW * 0.6f;
    float blackH = whiteH * 0.6f;
    struct BK { int key; int posIndex; };
    BK blacks[] = {
        {GLFW_KEY_W, 0}, 
        {GLFW_KEY_E, 1}, 
        {GLFW_KEY_T, 3}, 
        {GLFW_KEY_Y, 4}, 
        {GLFW_KEY_U, 5}  
    };
    for (auto& b : blacks) {
        int i = b.posIndex;
        float bx = x0 + (i + 1) * whiteW - blackW * 0.5f;
        float by = y0;
        bool on = false;
        { std::lock_guard<std::mutex> lock(mtx);
          auto it = activeNotes.find(b.key);
          on = (it != activeNotes.end() && it->second.pressed);
        }
        if (on) glColor3f(0.1f, 0.1f, 0.1f);
        else     glColor3f(0.02f, 0.02f, 0.02f);
        drawRect(bx, by, blackW, blackH, true);
        glColor3f(0.0f, 0.0f, 0.0f);
        drawRect(bx, by, blackW, blackH, false);
    }
}

void VirtualPiano::drawMeters(int width, int height) {
    float rms  = meterRms.load(std::memory_order_relaxed);
    float peak = meterPeak.load(std::memory_order_relaxed);
    if (rms  < 0.f) rms  = 0.f; else if (rms  > 1.f) rms  = 1.f;
    if (peak < 0.f) peak = 0.f; else if (peak > 1.f) peak = 1.f;

    const float barW = 20.f;
    const float barH = (float)height - 120.f;
    const float x    = (float)width - 40.f;
    const float y    = 60.f;

    glColor3f(0.15f, 0.15f, 0.18f);
    drawRect(x, y, barW, barH, true);
    glColor3f(0,0,0);
    drawRect(x, y, barW, barH, false);

    auto setColor = [](float v){
        if (v >= 0.90f) glColor3f(0.95f, 0.30f, 0.30f);   
        else if (v >= 0.70f) glColor3f(0.95f, 0.80f, 0.30f);
        else glColor3f(0.30f, 0.85f, 0.40f); 
    };

    float rmsH = barH * rms;
    setColor(rms);
    drawRect(x + 2.f, y + (barH - rmsH), (barW * 0.5f) - 3.f, rmsH, true);

    float pkH = barH * peak;
    setColor(peak);
    drawRect(x + (barW * 0.5f) + 1.f, y + (barH - pkH), (barW * 0.5f) - 3.f, pkH, true);

    glColor3f(0,0,0);
    drawRect(x + (barW * 0.5f), y + 2.f, 1.f, barH - 4.f, true);
}

