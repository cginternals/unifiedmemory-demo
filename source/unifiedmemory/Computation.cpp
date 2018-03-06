
#include "Computation.h"

#include <cassert>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <vector>

#include <glbinding/gl/gl.h>

#include "common.h"


namespace
{


const auto summationGroupSize = 16;
const auto averagingGroupSize = 16;
const auto transferGroupSize = 16;
const auto numSimdComponents = 4;


}


Computation::Computation()
: m_headerRead(false)
, m_numSensors(0)
, m_currentTime(0)
, m_stream(nullptr)
, m_dataBuffer(nullptr)
, m_averageBuffer(nullptr)
, m_minBuffer(nullptr)
, m_maxBuffer(nullptr)
, m_numIterations(0)
, m_sumProgram(0)
, m_sumShader(0)
, m_averageProgram(0)
, m_averageShader(0)
, m_strideLocation(0)
, m_countLocation(0)

, m_currentIndex(0)
, m_availablePoints(0)
, m_currentPingPongBuffer(0)
{
}

std::uint32_t Computation::numSensors() const
{
    return m_numSensors;
}

bool Computation::dataValid() const
{
    return m_headerRead && m_numSensors > 0;
}

std::uint64_t Computation::currentTime() const
{
    return m_currentTime;
}

std::uint32_t Computation::availablePoints() const
{
    return m_availablePoints;
}

std::uint32_t Computation::currentIndex() const
{
    return m_currentIndex;
}

void Computation::initialize(std::istream * stream, unshared_gpu_vector<float> * dataBuffer, unified_read_vector<float> * averageBuffer, unshared_gpu_vector<float> * minBuffer, unshared_gpu_vector<float> * maxBuffer, std::size_t numIterations)
{
    m_stream = stream;
    m_dataBuffer = dataBuffer;
    m_averageBuffer = averageBuffer;
    m_minBuffer = minBuffer;
    m_maxBuffer = maxBuffer;
    m_numIterations = numIterations;

    m_sumShader = gl::glCreateShader(gl::GL_COMPUTE_SHADER);
    m_sumProgram = gl::glCreateProgram();
    m_averageShader = gl::glCreateShader(gl::GL_COMPUTE_SHADER);
    m_averageProgram = gl::glCreateProgram();
    m_transferShader = gl::glCreateShader(gl::GL_COMPUTE_SHADER);
    m_transferProgram = gl::glCreateProgram();

    gl::glAttachShader(m_sumProgram, m_sumShader);
    gl::glAttachShader(m_averageProgram, m_averageShader);
    gl::glAttachShader(m_transferProgram, m_transferShader);

    setShaderSourceAndCompile(m_sumShader, dataPath() + "/unifiedmemory/shaders/sumcomputation.comp");
    auto success = checkForCompilationError(m_sumShader, "sum shader");

    if (!success)
    {
        return;
    }

    setShaderSourceAndCompile(m_averageShader, dataPath() + "/unifiedmemory/shaders/averagecomputation.comp");
    success &= checkForCompilationError(m_averageShader, "average shader");

    if (!success)
    {
        return;
    }

    setShaderSourceAndCompile(m_transferShader, dataPath() + "/unifiedmemory/shaders/linedatatransfer.comp");
    success &= checkForCompilationError(m_transferShader, "transfer shader");

    if (!success)
    {
        return;
    }

    gl::glLinkProgram(m_sumProgram);

    success &= checkForLinkerError(m_sumProgram, "sum program");

    if (!success)
    {
        return;
    }

    gl::glLinkProgram(m_averageProgram);

    success &= checkForLinkerError(m_averageProgram, "average program");

    if (!success)
    {
        return;
    }

    gl::glLinkProgram(m_transferProgram);

    success &= checkForLinkerError(m_transferProgram, "transfer program");

    if (!success)
    {
        return;
    }

    m_countLocation = gl::glGetUniformLocation(m_sumProgram, "u_count");
    m_strideLocation = gl::glGetUniformLocation(m_sumProgram, "u_stride");
    m_availablePointsLocation = gl::glGetUniformLocation(m_sumProgram, "u_availablePoints");

    assert(m_countLocation == gl::glGetUniformLocation(m_averageProgram, "u_count"));
    assert(m_strideLocation == gl::glGetUniformLocation(m_averageProgram, "u_stride"));
    assert(m_availablePointsLocation == gl::glGetUniformLocation(m_averageProgram, "u_availablePoints"));

    assert(m_countLocation == gl::glGetUniformLocation(m_transferProgram, "u_count"));
    assert(m_strideLocation == gl::glGetUniformLocation(m_transferProgram, "u_stride"));
}

void Computation::uninitialize()
{
    gl::glDeleteShader(m_sumShader);
    gl::glDeleteShader(m_averageShader);
    gl::glDeleteShader(m_transferShader);
    gl::glDeleteProgram(m_sumProgram);
    gl::glDeleteProgram(m_averageProgram);
    gl::glDeleteProgram(m_transferProgram);
}

std::uint32_t Computation::processEvents()
{
    auto numEventsProcessed = std::uint32_t(0);

    std::string line;
    while (*m_stream && m_stream->rdbuf()->in_avail())
    {
        std::getline(*m_stream, line);

        processLine(line);
        ++numEventsProcessed;
    }

    return numEventsProcessed;
}

void Computation::processLine(const std::string & line)
{
    if (!m_headerRead)
    {
        // First field is point in time
        m_numSensors = std::count(line.begin(), line.end(), ';');

        m_dataBuffer->resize(m_numIterations * m_numSensors);
        m_averageBuffer->resize(m_numSensors);
        m_minBuffer->resize(m_numSensors);
        m_maxBuffer->resize(m_numSensors);
        m_subaverageBuffer.resize(m_numIterations * m_numSensors / 2);
        m_subminBuffer.resize(m_numIterations * m_numSensors / 2);
        m_submaxBuffer.resize(m_numIterations * m_numSensors / 2);
        for (auto & buffer : m_pingPongBuffer)
        {
            buffer.resize(m_numSensors);
        }

        m_headerRead = true;
    }

    auto current = const_cast<char *>(line.data());

    m_currentTime = std::strtoll(current, &current, 10);

    for (auto i = std::uint32_t(0); i < m_numSensors; ++i)
    {
        assert(current - const_cast<char *>(line.data()) < static_cast<std::int64_t>(line.size()));

        const auto value = std::strtof(current+1, &current);
        const auto index = i;
        m_pingPongBuffer[m_currentPingPongBuffer].at(index) = value;
    }

    m_pingPongBuffer[m_currentPingPongBuffer].flush();
    transferLine();

    m_availablePoints = std::max(m_availablePoints, m_currentIndex+1);
    m_currentIndex = (m_currentIndex + 1) % m_numIterations;
}

void Computation::computeAverage()
{
    gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT);

    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, m_dataBuffer->gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, m_dataBuffer->gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 2, m_dataBuffer->gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 3, m_subaverageBuffer.gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 4, m_subminBuffer.gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 5, m_submaxBuffer.gpuIdentifier());

    gl::glUseProgram(m_sumProgram);

    gl::glUniform1i(m_strideLocation, m_numSensors);
    gl::glUniform1i(m_availablePointsLocation, m_availablePoints);

    const auto numSensorGroups = float(m_numSensors) / averagingGroupSize;
    auto iterations = m_numIterations / 2 / numSimdComponents;
    while (iterations > 0)
    {
        gl::glUniform1i(m_countLocation, iterations);

        const auto numDataPointGroups = float(iterations) / summationGroupSize;
        gl::glDispatchComputeGroupSizeARB(std::ceil(numDataPointGroups), std::ceil(numSensorGroups), 1, summationGroupSize, averagingGroupSize, 1);

        gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, m_subaverageBuffer.gpuIdentifier());
        gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, m_subminBuffer.gpuIdentifier());
        gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 2, m_submaxBuffer.gpuIdentifier());

        gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT);

        iterations /= 2;
    }

    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 3, m_averageBuffer->gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 4, m_minBuffer->gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 5, m_maxBuffer->gpuIdentifier());

    gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT);

    gl::glUseProgram(m_averageProgram);

    gl::glUniform1i(m_strideLocation, m_numSensors);
    gl::glUniform1i(m_countLocation, m_availablePoints);
    gl::glUniform1i(m_availablePointsLocation, static_cast<int>(std::ceil(m_availablePoints / (m_numIterations / 4.0f))));

    gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT);

    gl::glDispatchComputeGroupSizeARB(std::ceil(numSensorGroups), 1, 1, averagingGroupSize, 1, 1);

    gl::glUseProgram(0);

    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, 0);
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, 0);

    gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT);

    m_averageBuffer->startWait();
}

void Computation::transferLine()
{
    gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT);

    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, m_dataBuffer->gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, m_pingPongBuffer[m_currentPingPongBuffer].gpuIdentifier());

    m_currentPingPongBuffer = (m_currentPingPongBuffer + 1) % m_pingPongBuffer.size();

    gl::glUseProgram(m_transferProgram);

    gl::glUniform1i(m_strideLocation, m_numIterations);
    gl::glUniform1i(m_countLocation, m_currentIndex);

    const auto numSensorGroups = float(m_numSensors) / transferGroupSize;
    gl::glDispatchComputeGroupSizeARB(std::ceil(numSensorGroups), 1, 1, transferGroupSize, 1, 1);

    gl::glUseProgram(0);

    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, 0);
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, 0);

    gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT);
}
