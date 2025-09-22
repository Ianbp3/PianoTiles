#pragma once
#include <cmath>
#include <algorithm>

enum class Waveform { Sine = 0, Square = 1, Triangle = 2, Saw = 3 };
enum class EnvelopeState { Attack, Decay, Sustain, Release, Idle };

class Note {
private:
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double TWO_PI = 2.0 * PI;
    
    // Oscillator
    double frequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    
    // State
    bool pressed = false;
    bool active = false;
    
    // Envelope (ADSR)
    EnvelopeState envelopeState = EnvelopeState::Idle;
    double envelopeLevel = 0.0;
    
    // Envelope timing (in seconds)
    double attackTime = 0.01;   // 10ms
    double decayTime = 0.06;    // 60ms
    double sustainLevel = 0.7;  // 70%
    double releaseTime = 0.15;  // 150ms
    
    // Current waveform and amplitude
    Waveform currentWaveform = Waveform::Sine;
    double amplitude = 0.5;
    
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
        return 0.0;
    }
    
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
                
            case EnvelopeState::Decay:
                envelopeLevel -= stepSize * (1.0 - sustainLevel) / decayTime;
                if (envelopeLevel <= sustainLevel) {
                    envelopeLevel = sustainLevel;
                    envelopeState = EnvelopeState::Sustain;
                }
                break;
                
            case EnvelopeState::Sustain:
                envelopeLevel = sustainLevel;
                break;
                
            case EnvelopeState::Release:
                envelopeLevel -= stepSize * sustainLevel / releaseTime;
                if (envelopeLevel <= 0.0) {
                    envelopeLevel = 0.0;
                    envelopeState = EnvelopeState::Idle;
                    active = false;
                }
                break;
                
            case EnvelopeState::Idle:
                envelopeLevel = 0.0;
                break;
        }
    }

public:
    void startNote(double freq, double sampleRate, Waveform waveform) {
        frequency = freq;
        currentWaveform = waveform;
        phaseIncrement = TWO_PI * frequency / sampleRate;
        
        pressed = true;
        active = true;
        envelopeState = EnvelopeState::Attack;
        envelopeLevel = 0.0;
        phase = 0.0;
    }
    
    void stopNote() {
        pressed = false;
        if (envelopeState != EnvelopeState::Idle) {
            envelopeState = EnvelopeState::Release;
        }
    }
    
    double getSample() {
        if (!active) return 0.0;
        
        // Update envelope
        updateEnvelope(44100.0); // Assuming 44.1kHz sample rate
        
        // Generate waveform
        double waveValue = generateWaveform();
        
        // Advance phase
        phase += phaseIncrement;
        if (phase >= TWO_PI) {
            phase -= TWO_PI;
        }
        
        // Apply envelope and amplitude
        return amplitude * envelopeLevel * waveValue * 0.8; // 20% headroom
    }
    
    bool isActive() const { return active; }
    bool isPressed() const { return pressed; }
};