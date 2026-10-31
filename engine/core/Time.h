#pragma once

#include <algorithm>
#include <chrono>

namespace br {

class Stopwatch {
public:
    Stopwatch() : m_start(std::chrono::steady_clock::now()) {}
    void reset() { m_start = std::chrono::steady_clock::now(); }
    double elapsedSeconds() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
    }
    double elapsedMilliseconds() const { return elapsedSeconds() * 1000.0; }

private:
    std::chrono::steady_clock::time_point m_start;
};

// Fixed-timestep accumulator ("Fix Your Timestep"). Simulation (physics, AI,
// gameplay) advances in constant steps; rendering interpolates between the last
// two simulation states using alpha().
class FixedTimestep {
public:
    explicit FixedTimestep(double stepSeconds = 1.0 / 60.0, int maxStepsPerFrame = 5)
        : m_step(stepSeconds), m_maxSteps(maxStepsPerFrame) {}

    // Feeds real frame time; returns the number of fixed steps to run now.
    int advance(double frameSeconds) {
        // Clamp huge hitches (debugger, window drag) so we never spiral.
        m_accumulator += std::min(frameSeconds, m_step * m_maxSteps);
        int steps = 0;
        while (m_accumulator >= m_step && steps < m_maxSteps) {
            m_accumulator -= m_step;
            ++steps;
        }
        return steps;
    }

    double step() const { return m_step; }
    float alpha() const { return static_cast<float>(m_accumulator / m_step); }
    void reset() { m_accumulator = 0.0; }

private:
    double m_step;
    int m_maxSteps;
    double m_accumulator = 0.0;
};

} // namespace br
