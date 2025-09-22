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
#include <RtAudio.h>
#include <unordered_map>
#include <mutex>
#include "Note.hpp"

struct SynthParams {
    double sampleRate = 44100.0;
    int bufferFrames = 512;
    int channels = 2;
    double masterGain = 0.5;
    int octaveOffset = 0;
    Waveform waveform = Waveform::Sine;
    bool chorus = false;
};

class VirtualPiano {
public:
    bool initialize();
    void run();
    void cleanup();

private:
    GLFWwindow* window = nullptr;
    RtAudio audio;
    SynthParams params;

    std::unordered_map<int, double> keyFrequencies;
    std::unordered_map<int, Note> activeNotes;
    std::mutex notesMutex;

    float currentRMS = 0.0f;
    float currentPeak = 0.0f;

    std::vector<float> delayBufferL, delayBufferR;
    int delayIndex = 0;

    void setupKeyMapping();
    void handleKeyEvent(int key, int action);
    int processAudio(float* outputBuffer, int frameCount);
    void renderUI();
    void drawKeyboard(int w, int h);
    void drawVUMeters(int w, int h);

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static int audioCallback(void* outputBuffer, void* inputBuffer,
                           unsigned int nBufferFrames, double streamTime,
                           RtAudioStreamStatus status, void* userData);
};