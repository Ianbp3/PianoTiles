#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

enum class Waveform { Sine = 0, Square = 1, Triangle = 2, Saw = 3 };
enum class EnvStage { Idle, Attack, Decay, Sustain, Release };

namespace dspconst {
    constexpr double PI        = 3.14159265358979323846;
    constexpr double TWO_PI    = 6.28318530717958647692;
    constexpr double INV_SQRT2 = 0.70710678118654752440;
}

struct Note {
    double frequency = 440.0;
    double phase     = 0.0;
    double phaseInc  = 0.0;

    double amplitude = 0.5;

    bool   pressed = false;
    bool   alive   = false;

    EnvStage stage = EnvStage::Idle;
    double env     = 0.0;

    double attackSec  = 0.010;
    double decaySec   = 0.060;
    double sustainLvl = 0.70;
    double releaseSec = 0.5;

    Waveform wave = Waveform::Sine;

    void setFrequency(double f, double sampleRate) {
        frequency = f;
        phaseInc  = dspconst::TWO_PI * frequency / sampleRate;
    }

    inline double nextSample(double sampleRate) {
        const double aStep = (attackSec  > 0.0) ? 1.0 / (attackSec  * sampleRate) : 1.0;
        const double dStep = (decaySec   > 0.0) ? 1.0 / (decaySec   * sampleRate) : 1.0;
        const double rStep = (releaseSec > 0.0) ? 1.0 / (releaseSec * sampleRate) : 1.0;

        switch (stage) {
            case EnvStage::Attack:
                env += aStep;
                if (env >= 1.0) { env = 1.0; stage = EnvStage::Decay; }
                break;
            case EnvStage::Decay: {
                const double dec = dStep * (1.0 - sustainLvl);
                env -= dec;
                if (env <= sustainLvl) { env = sustainLvl; stage = EnvStage::Sustain; }
                break;
            }
            case EnvStage::Sustain:
                env = sustainLvl;
                break;
            case EnvStage::Release:
                env -= rStep;
                if (env <= 0.0) { env = 0.0; stage = EnvStage::Idle; alive = false; }
                break;
            case EnvStage::Idle:
            default:
                env = 0.0;
                break;
        }

        phase += phaseInc;
        if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;

        double x = 0.0;
        switch (wave) {
            case Waveform::Sine:     x = std::sin(phase); break;
            case Waveform::Square:   x = (std::sin(phase) >= 0.0 ? 1.0 : -1.0); break;
                case Waveform::Triangle: {
                    const double t = phase / dspconst::TWO_PI;
                    x = 4.0 * std::fabs(t - std::floor(t + 0.5)) - 1.0;
                    break;
                }
                case Waveform::Saw: {
                    const double t = phase / dspconst::TWO_PI;
                    x = 2.0 * (t - std::floor(t + 0.5));
                    break;
                }
            }

            double g = 1.0;
            switch (wave) {
                case Waveform::Square:   g = dspconst::INV_SQRT2;          break;
                case Waveform::Triangle: g = std::sqrt(1.5);               break;
                case Waveform::Saw:      g = std::sqrt(1.5);               break;
                case Waveform::Sine:     default: g = 1.0;                 break;
            }
            constexpr double headroom = 0.90;

            return amplitude * env * (headroom * g * x);
        }
    };
