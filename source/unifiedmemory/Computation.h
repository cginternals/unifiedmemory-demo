
#pragma once


#include <iosfwd>
#include <string>

#include <glbinding/gl/types.h>

#include "unified_vector.h"


class Computation
{
public:
    Computation();

    void initialize(std::istream * stream, unshared_gpu_vector<float> * dataBuffer, unified_read_vector<float> * averageBuffer, unshared_gpu_vector<float> * minBuffer, unshared_gpu_vector<float> * maxBuffer, std::size_t numIterations);
    void uninitialize();

    std::uint32_t processEvents();
    void computeAverage();

    bool dataValid() const;

    std::uint32_t numSensors() const;
    std::uint64_t currentTime() const;
    std::uint32_t availablePoints() const;
    std::uint32_t currentIndex() const;

protected:
    bool m_headerRead;
    std::uint32_t m_numSensors;
    std::uint64_t m_currentTime;
    std::istream * m_stream;
    unshared_gpu_vector<float> * m_dataBuffer;
    unified_read_vector<float> * m_averageBuffer;
    unshared_gpu_vector<float> * m_minBuffer;
    unshared_gpu_vector<float> * m_maxBuffer;
    std::size_t m_numIterations;
    gl::GLuint m_sumProgram;
    gl::GLuint m_sumShader;
    gl::GLuint m_averageProgram;
    gl::GLuint m_averageShader;
    gl::GLuint m_transferProgram;
    gl::GLuint m_transferShader;
    gl::GLint m_strideLocation;
    gl::GLint m_countLocation;
    gl::GLint m_availablePointsLocation;

protected:
    void processLine(const std::string & line);
    void transferLine();

private:
    std::size_t m_currentIndex;
    std::size_t m_availablePoints;
    std::size_t m_currentPingPongBuffer;
    unshared_gpu_vector<float> m_subaverageBuffer;
    unshared_gpu_vector<float> m_subminBuffer;
    unshared_gpu_vector<float> m_submaxBuffer;
    std::array<unified_write_vector<float>, 32> m_pingPongBuffer;
};
