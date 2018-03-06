
#include "Rendering.h"

#include <vector>
#include <future>
#include <fstream>

#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>

#include "common.h"


namespace
{


const auto numColorComponents = 3;


}


Rendering::Rendering()
: m_fbo(0)

, m_plotVAO(0)
, m_averageVAO(0)

, m_plotProgram(0)
, m_plotVertexShader(0)
, m_plotFragmentShader(0)

, m_averageProgram(0)
, m_averageVertexShader(0)
, m_averageGeometryShader(0)
, m_averageFragmentShader(0)

, m_plotVertexOffsetLocation(0)
, m_plotSensorCountLocation(0)
, m_plotCountLocation(0)
, m_plotColorLocation(0)
, m_plotCurrentSensorLocation(0)

, m_renderingInitialized(false)

, m_stride(0)
, m_sensorCount(0)
, m_pointCount(0)
, m_currentIndex(0)

, m_width(0)
, m_height(0)

, m_dataBuffer(nullptr)
, m_averageBuffer(nullptr)
, m_minBuffer(nullptr)
, m_maxBuffer(nullptr)
, m_pixelBuffer(nullptr)
{
}

Rendering::~Rendering()
{
}

void Rendering::initialize(unshared_gpu_vector<float> * dataBuffer, unified_read_vector<float> * averageBuffer, unshared_gpu_vector<float> * minBuffer, unshared_gpu_vector<float> * maxBuffer, unified_read_vector<unsigned char> * pixelBuffer)
{
    m_dataBuffer = dataBuffer;
    m_averageBuffer = averageBuffer;
    m_minBuffer = minBuffer;
    m_maxBuffer = maxBuffer;
    m_pixelBuffer = pixelBuffer;

    gl::glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    gl::glCreateVertexArrays(1, &m_plotVAO);
    gl::glCreateVertexArrays(1, &m_averageVAO);

    m_plotProgram = gl::glCreateProgram();
    m_plotVertexShader = gl::glCreateShader(gl::GL_VERTEX_SHADER);
    m_plotFragmentShader = gl::glCreateShader(gl::GL_FRAGMENT_SHADER);

    m_averageProgram = gl::glCreateProgram();
    m_averageVertexShader = gl::glCreateShader(gl::GL_VERTEX_SHADER);
    m_averageGeometryShader = gl::glCreateShader(gl::GL_GEOMETRY_SHADER);
    m_averageFragmentShader = gl::glCreateShader(gl::GL_FRAGMENT_SHADER);

    gl::glAttachShader(m_plotProgram, m_plotVertexShader);
    gl::glAttachShader(m_plotProgram, m_plotFragmentShader);

    gl::glAttachShader(m_averageProgram, m_averageVertexShader);
    gl::glAttachShader(m_averageProgram, m_averageGeometryShader);
    gl::glAttachShader(m_averageProgram, m_averageFragmentShader);

    setShaderSourceAndCompile(m_plotVertexShader, dataPath() + "/unifiedmemory/shaders/plot.vert");
    auto success = checkForCompilationError(m_plotVertexShader, "plot vertex shader");

    if (!success)
    {
        return;
    }

    setShaderSourceAndCompile(m_plotFragmentShader, dataPath() + "/unifiedmemory/shaders/plot.frag");
    success &= checkForCompilationError(m_plotFragmentShader, "plot fragment shader");

    if (!success)
    {
        return;
    }

    setShaderSourceAndCompile(m_averageVertexShader, dataPath() + "/unifiedmemory/shaders/average.vert");
    success = checkForCompilationError(m_averageVertexShader, "average vertex shader");

    if (!success)
    {
        return;
    }

    setShaderSourceAndCompile(m_averageGeometryShader, dataPath() + "/unifiedmemory/shaders/average.geom");
    success = checkForCompilationError(m_averageGeometryShader, "average geometry shader");

    if (!success)
    {
        return;
    }

    setShaderSourceAndCompile(m_averageFragmentShader, dataPath() + "/unifiedmemory/shaders/average.frag");
    success &= checkForCompilationError(m_averageFragmentShader, "average fragment shader");

    if (!success)
    {
        return;
    }

    gl::glLinkProgram(m_plotProgram);

    success &= checkForLinkerError(m_plotProgram, "plot program");

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

    m_plotVertexOffsetLocation = gl::glGetUniformLocation(m_plotProgram, "u_vertexOffset");
    m_plotCountLocation = gl::glGetUniformLocation(m_plotProgram, "u_count");
    m_plotSensorCountLocation = gl::glGetUniformLocation(m_plotProgram, "u_sensorCount");
    m_plotColorLocation = gl::glGetUniformLocation(m_plotProgram, "u_color");
    m_plotCurrentSensorLocation = gl::glGetUniformLocation(m_plotProgram, "u_sensor");

    m_averageSensorCountLocation = gl::glGetUniformLocation(m_averageProgram, "u_sensorCount");
    m_averageColorLocation = gl::glGetUniformLocation(m_averageProgram, "u_color");
    m_averageCurrentSensorLocation = gl::glGetUniformLocation(m_averageProgram, "u_sensor");
}

void Rendering::uninitialize()
{
    gl::glDeleteShader(m_plotVertexShader);
    gl::glDeleteShader(m_averageVertexShader);
    gl::glDeleteShader(m_plotFragmentShader);
    gl::glDeleteProgram(m_plotProgram);
    gl::glDeleteProgram(m_averageProgram);
    gl::glDeleteVertexArrays(1, &m_plotVAO);
    gl::glDeleteVertexArrays(1, &m_averageVAO);
}

void Rendering::render()
{
    static const auto colors = std::array<glm::vec3, 8>({{
        glm::vec3(228,26,28) / glm::vec3(255),
        glm::vec3(55,126,184) / glm::vec3(255),
        glm::vec3(77,175,74) / glm::vec3(255),
        glm::vec3(152,78,163) / glm::vec3(255),
        glm::vec3(255,127,0) / glm::vec3(255),
        glm::vec3(255,255,51) / glm::vec3(255),
        glm::vec3(166,86,40) / glm::vec3(255),
        glm::vec3(247,129,191) / glm::vec3(255)
    }});

    const auto numIterations = m_dataBuffer->size() / m_sensorCount;

    if (!m_renderingInitialized)
    {
        gl::glDisable(gl::GL_DEPTH_TEST);

        gl::glUseProgram(m_plotProgram);

        gl::glUniform1i(m_plotSensorCountLocation, m_sensorCount);
        gl::glUniform1i(m_plotCountLocation, numIterations);

        //gl::glUseProgram(0);
        gl::glUseProgram(m_averageProgram);

        gl::glUniform1i(m_averageSensorCountLocation, m_sensorCount);

        gl::glUseProgram(0);

        gl::glPointSize(m_sensorCount > 8 ? 2.0f : 4.0f);

        gl::glBindVertexArray(m_plotVAO);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, m_dataBuffer->gpuIdentifier());
        gl::glEnableVertexAttribArray(0);

        gl::glVertexAttribPointer(0, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(float), 0);

        gl::glBindVertexArray(0);

        gl::glBindVertexArray(m_averageVAO);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, m_averageBuffer->gpuIdentifier());
        gl::glEnableVertexAttribArray(0);

        gl::glVertexAttribPointer(0, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(float), 0);

        gl::glBindVertexArray(0);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);

        m_renderingInitialized = true;
    }

    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, m_fbo);
    gl::glClear(gl::GL_COLOR_BUFFER_BIT);

    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, m_minBuffer->gpuIdentifier());
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, m_maxBuffer->gpuIdentifier());

    gl::glUseProgram(m_averageProgram);
    gl::glBindVertexArray(m_averageVAO);

    for (auto i = std::uint32_t(0); i < m_sensorCount; ++i)
    {
        gl::glUniform1i(m_averageCurrentSensorLocation, i);
        gl::glUniform3fv(m_averageColorLocation, 1, glm::value_ptr(colors[i % colors.size()]));

        gl::glDrawArrays(gl::GL_POINTS, i, 1);
    }

    gl::glUseProgram(m_plotProgram);
    gl::glBindVertexArray(m_plotVAO);

    for (auto i = std::uint32_t(0); i < m_sensorCount; ++i)
    {
        gl::glUniform1i(m_plotCurrentSensorLocation, i);
        gl::glUniform3fv(m_plotColorLocation, 1, glm::value_ptr(colors[i % colors.size()]));

        // From current until end
        gl::glUniform1i(m_plotVertexOffsetLocation, -m_currentIndex);
        gl::glDrawArrays(gl::GL_LINE_STRIP, numIterations*i + m_currentIndex, m_pointCount - m_currentIndex);

        // From start until current
        gl::glUniform1i(m_plotVertexOffsetLocation, m_pointCount - m_currentIndex);
        gl::glDrawArrays(gl::GL_LINE_STRIP, numIterations*i + 0, m_currentIndex);

        // From last to current
        if (m_currentIndex + 8 < numIterations && m_pointCount == numIterations)
        {
            const auto range = std::array<gl::GLuint, 2> {{ numIterations*(i+1) - 1, numIterations*(i+0) + 0 }};
            gl::glUniform1i(m_plotVertexOffsetLocation, -m_currentIndex-1);
            gl::glDrawElements(gl::GL_LINE_STRIP, 2, gl::GL_UNSIGNED_INT, range.data());
        }
    }

    gl::glBindVertexArray(0);
    gl::glUseProgram(0);

    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, 0);
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, 0);

    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
}

void Rendering::captureImage()
{
    gl::glMemoryBarrier(gl::GL_FRAMEBUFFER_BARRIER_BIT | gl::GL_PIXEL_BUFFER_BARRIER_BIT);
    gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, m_pixelBuffer->gpuIdentifier());

    gl::glReadBuffer(gl::GL_BACK_LEFT);
    gl::glReadnPixels(0, 0, m_width, m_height, gl::GL_RGB, gl::GL_UNSIGNED_BYTE, m_pixelBuffer->size(), nullptr);

    gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, 0);

    m_pixelBuffer->startWait();
}

void Rendering::resize(int width, int height)
{
    m_width = width;
    m_height = height;

    m_pixelBuffer->resize(m_width * m_height * numColorComponents * sizeof(gl::GLubyte));

    gl::glViewport(0, 0, m_width, m_height);
}

void Rendering::updateGeometry(int stride, int sensorCount, int pointCount, int currentIndex)
{
    m_stride = stride;
    m_sensorCount = sensorCount;
    m_pointCount = pointCount;
    m_currentIndex = currentIndex;
}

void Rendering::saveCapturedImage(std::string && basename)
{
    const auto filename = std::move(basename) + "." + std::to_string(m_width) + "." + std::to_string(m_height) + ".rgb.ub.raw";

    m_pixelBuffer->wait();

    std::async(std::launch::async, [this, filename=std::move(filename)]() {
        std::fstream stream(filename, std::fstream::out | std::fstream::binary);

        stream.write(reinterpret_cast<const char *>(m_pixelBuffer->data()), m_pixelBuffer->size());

        stream.close();
    });
}
