#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#if defined(__APPLE__)
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif

#include <GLFW/glfw3.h>
#include <memory> 
#include <RtAudio.h>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>
#include <atomic>
#include "Note.hpp"

struct SynthParams {
    double sampleRate = 44100.0;
    unsigned int bufferFrames = 512;
    unsigned int channels = 2;
    double masterGain = 0.5;
    int    octaveOffset = 0;
    Waveform waveform = Waveform::Sine;
    bool   chorus = false;
};

class VirtualPiano {
public:
    bool initialize();
    void run();
    void cleanup();

private:
    GLFWwindow* window = nullptr;
    std::unique_ptr<RtAudio> audio;  
    SynthParams params;

    std::unordered_map<int, double> keyToFreqBase;
    std::unordered_map<int, Note> activeNotes;
    std::mutex mtx;

    std::atomic<float> meterRms{0.0f};
    std::atomic<float> meterPeak{0.0f};

    std::vector<float> delayL, delayR;
    size_t delayIdx = 0;

    void setupKeymap();
    static void s_keyCallback(GLFWwindow* w, int key, int sc, int action, int mods);
    void keyCallback(int key, int action);
    static int s_audioCallback(void* outputBuffer, void* inputBuffer,
                               unsigned int nBufferFrames, double streamTime,
                               RtAudioStreamStatus status, void* userData);
    int audioCallback(float* out, unsigned int nFrames);

    void drawUI();
    void drawKeyboard(int width, int height);
    void drawMeters(int width, int height);
};
