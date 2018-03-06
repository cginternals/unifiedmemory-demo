
#pragma once

#include <glbinding/gl/types.h>

#include "unified_vector.h"


class Rendering
{
public:
    Rendering();
    ~Rendering();

    void initialize(unshared_gpu_vector<float> * dataBuffer, unified_read_vector<float> * averageBuffer, unshared_gpu_vector<float> * minBuffer, unshared_gpu_vector<float> * maxBuffer, unified_read_vector<unsigned char> * pixelBuffer);
    void uninitialize();

    void render();
    void captureImage();

    void resize(int width, int height);
    void updateGeometry(int stride, int sensorCount, int pointCount, int currentIndex);

    void saveCapturedImage(std::string && basename);

protected:
    const gl::GLuint m_fbo;

    gl::GLuint m_plotVAO;
    gl::GLuint m_averageVAO;

    gl::GLuint m_plotProgram;
    gl::GLuint m_plotVertexShader;
    gl::GLuint m_plotFragmentShader;

    gl::GLuint m_averageProgram;
    gl::GLuint m_averageVertexShader;
    gl::GLuint m_averageGeometryShader;
    gl::GLuint m_averageFragmentShader;

    gl::GLint m_plotVertexOffsetLocation;
    gl::GLint m_plotSensorCountLocation;
    gl::GLint m_plotCountLocation;
    gl::GLint m_plotColorLocation;
    gl::GLint m_plotCurrentSensorLocation;

    gl::GLint m_averageSensorCountLocation;
    gl::GLint m_averageColorLocation;
    gl::GLint m_averageCurrentSensorLocation;

    bool m_renderingInitialized;

    std::uint32_t m_stride;
    std::uint32_t m_sensorCount;
    std::uint32_t m_pointCount;
    std::uint32_t m_currentIndex;

    gl::GLint m_width;
    gl::GLint m_height;

    unshared_gpu_vector<float> * m_dataBuffer;
    unified_read_vector<float> * m_averageBuffer;
    unshared_gpu_vector<float> * m_minBuffer;
    unshared_gpu_vector<float> * m_maxBuffer;
    unified_read_vector<unsigned char> * m_pixelBuffer;
};
