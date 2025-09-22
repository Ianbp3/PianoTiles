#include <VirtualPiano.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

static void drawRect(float x, float y, float w, float h, bool filled = true){
    if (filled) glBegin(GL_QUADS);
    else glBegin(GL_LINE_LOOP);

    glVertex2f(x,y);
    glVertex2f(x+w, y);
    glVertex2f(x+w, y+h);
    glVertex2f(x, y+h);
    glEnd();
}

bool VirtualPiano::initialize(){
    if(!glfwInit()){
        std::cout << "Failed to initialize GLFW\n";
        return false;
    }

    window = glfwCreateWindow(700, 300, "Piano Tiles", nullptr, nullptr);

    if(!window){
        std::cout << "Failed to create window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, &VirtualPiano::keyCallback);
    glfwSwapInterval(1);

    if(audio.getDeviceCount() < 1){
        std::cout << "No audio devices found\n";
        return false;
    }

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = audio.getDefaultOutputDevice();
    outputParams.nChannels = params.channels;
    outputParams.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_MINIMIZE_LATENCY;
    options.numberOfBuffers = 2;

    try{
        audio.openStream(&outputParams, nullptr, RTAUDIO_FLOAT32, (unsigned int)params.sampleRate, (unsigned int*)&params.bufferFrames, &VirtualPiano::audioCallback, this, &options);
        audio.startStream();
    }catch(const std::exception& e){
        std::cout << "RtAudio Error: " << e.what() << '\n';
        return false;
    }

    setupKeyMapping();

    int delaySamples = (int)(params.sampleRate * 0.04);
    delayBufferL.resize(delaySamples, 0.0f);
    delayBufferR.resize(delaySamples, 0.0f);
    delayIndex = 0;

    return true;
}

void VirtualPiano::run(){
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        renderUI();
        glfwSwapBuffers(window);
    }
}

void VirtualPiano::cleanup(){
    try{
        if(audio.isStreamRunning()) audio.stopStream();
        if(audio.isStreamOpen()) audio.closeStream();
    }catch(...){}

    if(window){
        glfwDestroyWindow(window);

    }
    glfwTerminate();
}

void VirtualPiano::setupKeyMapping(){
        keyFrequencies = {
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

void VirtualPiano::handleKeyEvent(int key, int action){
    if(action == GLFW_PRESS) {
        if(key == GLFW_KEY_ESCAPE){
            glfwSetWindowShouldClose(window, 1);
            return;
        }

        if (key == GLFW_KEY_RIGHT){
            params.masterGain = std::min(1.0, params.masterGain + 0.05);
            return;
        }

        if (key == GLFW_KEY_LEFT){
            params.masterGain = std::max(0.0, params.masterGain - 0.05);
            return;
        }

        if (key == GLFW_KEY_UP){
            params.octaveOffset = std::min(3, params.octaveOffset + 1);
            return;
        }

        if (key == GLFW_KEY_DOWN){
            params.octaveOffset = std::max(-3, params.octaveOffset - 1);
            return;
        }

        if (key == GLFW_KEY_1){
            params.waveform = Waveform::Sine;
            return;
        }

        if (key == GLFW_KEY_2){
            params.waveform = Waveform::Square;
            return;
        }

        if (key == GLFW_KEY_3){
            params.waveform = Waveform::Triangle;
            return;
        }

        if (key == GLFW_KEY_4){
            params.waveform = Waveform::Saw;
            return;
        }

        if (key == GLFW_KEY_C){
            params.chorus = !params.chorus;
            return;
        }

        auto it = keyFrequencies.find(key);
        if(it != keyFrequencies.end()){
            std::lock_guard<std::mutex> lock(notesMutex);
            Note& note = activeNotes[key];
            note.startNote(it->second*std::pow(2.0, params.octaveOffset), params.sampleRate, params.waveform);
        }
    }else if(action == GLFW_RELEASE){
        auto it = keyFrequencies.find(key);
        if(it != keyFrequencies.end()){
            std::lock_guard<std::mutex> lock(notesMutex);
            auto noteIt = activeNotes.find(key);
            if (noteIt != activeNotes.end()){
                noteIt->second.stopNote();
            }
        }
    }
}

int VirtualPiano::processAudio(float* outputBuffer, int frameCount){
    std::fill(outputBuffer, outputBuffer+frameCount*params.channels, 0.0f);

    std::lock_guard<std::mutex> lock(notesMutex);

    double rmsAccumulator = 0.0;
    double peakLevel = 0.0;

    for(int frame = 0; frame < frameCount; ++frame){
        double mixedSample = 0.0;

        for(auto& [key, note] : activeNotes){
            if(note.isActive()){
                mixedSample += note.getSample();
            }
        }

        mixedSample *= params.masterGain;

        double absLevel = std::abs(mixedSample);
        if (absLevel > 0.9) {
            double excess = (absLevel - 0.9) / 0.1;
            double compressed = 0.9 + 0.1 * (excess - (excess * excess * excess) / 3.0);
            mixedSample = (mixedSample < 0) ? -compressed : compressed;
        }
        
        mixedSample = std::clamp(mixedSample, -1.0, 1.0);
        
        float sample = static_cast<float>(mixedSample);

        if(params.chorus && params.channels >= 2 && !delayBufferL.empty()){
            static double lfoPhaseL = 0.0, lfoPhaseR = 0.0;
            const double lfoFreqL = 0.8, lfoFreqR = 1.2;
            const double lfoStepL = 2.0 * M_PI * lfoFreqL / params.sampleRate;
            const double lfoStepR = 2.0 * M_PI * lfoFreqR / params.sampleRate;

            const float baseDelay = 18.0f;
            const float depthDelay = 6.0f;
            float delayTimeL = baseDelay + depthDelay * std::sin(lfoPhaseL);
            float delayTimeR = baseDelay + depthDelay * std::sin(lfoPhaseR);

            float delaySamplesL = delayTimeL * params.sampleRate/1000.0f;
            float delaySamplesR = delayTimeR * params.sampleRate/1000.0f;

            delayBufferL[delayIndex] = sample;
            delayBufferR[delayIndex] = sample;

            auto readDelay = [](const std::vector<float>& buffer, int writeIdx, float delaySamps) -> float {
                int bufSize = buffer.size();
                float readPos = writeIdx - delaySamps;
                while (readPos < 0) readPos+=bufSize;

                int idx0 = (int)readPos;
                int idx1 = (idx0 + 1)%bufSize;
                float frac = readPos - idx0;

                return buffer[idx0] + (buffer[idx1] - buffer[idx0])*frac;
            };

            float delayedL = readDelay(delayBufferL, delayIndex, delaySamplesL);
            float delayedR = readDelay(delayBufferR, delayIndex, delaySamplesR);

            const float dryLevel = 0.7f;
            const float wetLevel = 0.3f;

            outputBuffer[frame*2+0] = dryLevel*sample + wetLevel*delayedL;
            outputBuffer[frame*2+1] = dryLevel*sample + wetLevel*delayedR;

            lfoPhaseL += lfoStepL;
            lfoPhaseR += lfoStepR;

            if (lfoPhaseL >= 2.0 * M_PI) lfoPhaseL -= 2.0 * M_PI;
            if (lfoPhaseR >= 2.0 * M_PI) lfoPhaseR -= 2.0 * M_PI;

            delayIndex = (delayIndex+1)%delayBufferL.size();

            double stereoSample = (outputBuffer[frame * 2 + 0] + outputBuffer[frame * 2 + 1]) * 0.5;
            double absSample = std::abs(stereoSample);
            if (absSample > peakLevel) peakLevel = absSample;
            rmsAccumulator += stereoSample * stereoSample;  
        }else{
            outputBuffer[frame*params.channels + 0] = sample;
            if(params.channels > 1){
                outputBuffer[frame*params.channels + 1] = sample;
            }

            double absSample = std::abs(mixedSample);
            if (absSample > peakLevel) peakLevel = absSample;
            rmsAccumulator += mixedSample * mixedSample;
        }
    }

    currentRMS = std::sqrt(rmsAccumulator/frameCount);
    currentPeak = peakLevel;

    std::vector<int> keysToRemove;
    for (auto& [key, note] : activeNotes){
        if(!note.isActive()){
            keysToRemove.push_back(key);
        }
    }
    for (int key : keysToRemove){
        activeNotes.erase(key);
    }

    return 0;
}

void VirtualPiano::renderUI(){
    int w, h;

    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawKeyboard(w, h);
    drawVUMeters(w, h);

    glColor3f(0.2f, 0.8f, 0.4f);
    drawRect(20,20, (float)(params.masterGain* (w-40)), 15, true);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawRect(20, 20, (float)(w - 40), 15, false);

    static double lastPrint = 0.0;
    double currentTime = glfwGetTime();
    if (currentTime - lastPrint > 2.0) {
        lastPrint = currentTime;
        const char* waveNames[] = {"Sine", "Square", "Triangle", "Saw"};
        std::cout << "[Piano] Volume: " << params.masterGain
                  << " | Octave: " << params.octaveOffset 
                  << " | Wave: " << waveNames[(int)params.waveform]
                  << " | Chorus: " << (params.chorus ? "ON" : "OFF") << "\n";
    }
}

void VirtualPiano::drawKeyboard(int w, int h){
    const float keyboardX = 50.0f;
    const float keyboardY = 60.0f;
    const float whiteKeyWidth = (w - 100.0f) / 7.0f;
    const float whiteKeyHeight = h - keyboardY - 40.0f;
    const float blackKeyWidth = whiteKeyWidth * 0.6f;
    const float blackKeyHeight = whiteKeyHeight * 0.65f;
    
    int whiteKeys[] = {GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_F, 
                       GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J};
    
    bool whitePressed[7] = {false};
    {
        std::lock_guard<std::mutex> lock(notesMutex);
        for (int i = 0; i < 7; ++i) {
            auto it = activeNotes.find(whiteKeys[i]);
            whitePressed[i] = (it != activeNotes.end() && it->second.isPressed());
        }
    }
    
    for (int i = 0; i < 7; ++i) {
        bool pressed = whitePressed[i];
        glColor3f(pressed ? 0.9f : 0.95f, pressed ? 0.9f : 0.95f, pressed ? 0.3f : 0.95f);
        
        float x = keyboardX + i * whiteKeyWidth;
        drawRect(x, keyboardY, whiteKeyWidth - 2.0f, whiteKeyHeight, true);
        
        glColor3f(0.0f, 0.0f, 0.0f);
        drawRect(x, keyboardY, whiteKeyWidth - 2.0f, whiteKeyHeight, false);
    }

    struct BlackKey { int keyCode; int position; };
    BlackKey blackKeys[] = {
        {GLFW_KEY_W, 0}, {GLFW_KEY_E, 1}, {GLFW_KEY_T, 3}, 
        {GLFW_KEY_Y, 4}, {GLFW_KEY_U, 5}
    };
    
    for (auto& bk : blackKeys) {
        bool pressed = false;
        {
            std::lock_guard<std::mutex> lock(notesMutex);
            auto it = activeNotes.find(bk.keyCode);
            pressed = (it != activeNotes.end() && it->second.isPressed());
        }
        
        float x = keyboardX + (bk.position + 1) * whiteKeyWidth - blackKeyWidth * 0.5f;
        
        glColor3f(pressed ? 0.3f : 0.1f, pressed ? 0.3f : 0.1f, pressed ? 0.3f : 0.1f);
        drawRect(x, keyboardY, blackKeyWidth, blackKeyHeight, true);
        
        glColor3f(0.0f, 0.0f, 0.0f);
        drawRect(x, keyboardY, blackKeyWidth, blackKeyHeight, false);
    }
}

void VirtualPiano::drawVUMeters(int w, int h){
    const float meterWidth = 25.0f;
    const float meterHeight = h - 120.0f;
    const float meterX = w - 50.0f;
    const float meterY = 60.0f;

    float rms = std::clamp(currentRMS, 0.0f, 1.0f);
    float peak = std::clamp(currentPeak, 0.0f, 1.0f);
    
    glColor3f(0.15f, 0.15f, 0.18f);
    drawRect(meterX, meterY, meterWidth, meterHeight, true);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(meterX, meterY, meterWidth, meterHeight, false);
    
    auto setMeterColor = [](float level) {
        if (level >= 0.9f) glColor3f(0.95f, 0.3f, 0.3f);      
        else if (level >= 0.7f) glColor3f(0.95f, 0.8f, 0.3f); 
        else glColor3f(0.3f, 0.85f, 0.4f);                    
    };
    
    float rmsHeight = meterHeight * rms;
    setMeterColor(rms);
    drawRect(meterX + 2.0f, meterY + (meterHeight - rmsHeight), 
             (meterWidth * 0.5f) - 3.0f, rmsHeight, true);
    
    float peakHeight = meterHeight * peak;
    setMeterColor(peak);
    drawRect(meterX + (meterWidth * 0.5f) + 1.0f, meterY + (meterHeight - peakHeight), 
             (meterWidth * 0.5f) - 3.0f, peakHeight, true);
    
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(meterX + (meterWidth * 0.5f), meterY + 2.0f, 1.0f, meterHeight - 4.0f, true);
}

void VirtualPiano::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
    auto* piano = static_cast<VirtualPiano*>(glfwGetWindowUserPointer(window));
    if (piano) piano->handleKeyEvent(key, action);
}

int VirtualPiano::audioCallback(void* outputBuffer, void*, unsigned int nBufferFrames, double, RtAudioStreamStatus status, void* userData){
    auto* piano = static_cast<VirtualPiano*>(userData);
    return piano->processAudio(static_cast<float*>(outputBuffer), nBufferFrames);
}