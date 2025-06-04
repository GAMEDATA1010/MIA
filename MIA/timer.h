#ifndef TIMER_H
#define TIMER_H

#include <chrono>   // For std::chrono::high_resolution_clock and duration_cast
#include <string>   // For std::string
#include <iostream> // For std::cout

/**
 * @class Timer
 * @brief A utility class for timing code execution segments.
 *
 * This class provides methods to start a timer, capture elapsed time
 * between points, and log the captured duration.
 */
class Timer {
public:
    /**
     * @brief Constructor for the Timer class.
     *
     * Automatically calls the start() method upon initialization
     * to set the initial time points.
     */
    Timer() {
        start();
    }

    /**
     * @brief Starts or resets the timer.
     *
     * Sets both m_T1 and m_T2 to the current high-resolution time point.
     * Initializes the segment name to "Start" and difference to 0.
     */
    void start() {
        m_T1 = std::chrono::high_resolution_clock::now();
        m_T2 = m_T1; // Initialize T2 to be the same as T1
        m_name = "Start";
        m_diff = 0; // Initial difference is 0
    }

    /**
     * @brief Captures the elapsed time since the last capture or start point.
     *
     * Moves the previous m_T2 to m_T1, records the new current time in m_T2,
     * calculates the difference in milliseconds, and stores the provided name.
     *
     * @param name A string describing the code segment being timed.
     * @return The elapsed time in milliseconds for the captured segment.
     */
    long long capture(std::string name) {
        m_T1 = m_T2; // Move the previous end time to the start of the new segment
        m_T2 = std::chrono::high_resolution_clock::now(); // Record the new end time
        m_name = std::move(name); // Store the name for this segment
        m_diff = std::chrono::duration_cast<std::chrono::milliseconds>(m_T2 - m_T1).count();
        return m_diff;
    }

    /**
     * @brief Logs the name of the last captured segment and its duration.
     *
     * Outputs the information to the console in the format:
     * "SegmentName took X ms"
     */
    void log() const {
        std::cout << m_name << " took " << m_diff << " ms" << std::endl;
    }

private:
    // Stores the start time point of the current segment
    std::chrono::high_resolution_clock::time_point m_T1;
    // Stores the end time point of the current segment
    std::chrono::high_resolution_clock::time_point m_T2;
    // Stores the name of the current timed segment
    std::string m_name;
    // Stores the calculated duration of the last captured segment in milliseconds
    long long m_diff;
};

#endif // TIMER_H

