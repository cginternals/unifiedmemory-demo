
#include <iostream>
#include <thread>
#include <algorithm>
#include <sstream>
#include <random>

#include <GLFW/glfw3.h> 

#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glbinding-aux/types_to_string.h>

#include "Computation.h"
#include "Rendering.h"


namespace
{


Computation computation;
Rendering rendering;

const auto canvasWidth  = 1440;                  // in pixel
const auto canvasHeight = 900;                   // in pixel
const auto fullScreen   = false;                 // start application in fullscreen
const auto windowTitle  = "Unified Memory Demo"; // Title of the window

// "The size callback ... which is called when the window is resized."
// http://www.glfw.org/docs/latest/group__window.html#gaa40cd24840daa8c62f36cafc847c72b6
void resizeCallback(GLFWwindow * /*window*/, int width, int height)
{
    rendering.resize(width, height);
}

void keyCallback(GLFWwindow * window, int key, int /*scancode*/, int /*action*/, int /*mods*/)
{
    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}


// "In case a GLFW function fails, an error is reported to the GLFW 
// error callback. You can receive these reports with an error
// callback." http://www.glfw.org/docs/latest/quick.html#quick_capture_error
void errorCallback(int errnum, const char * errmsg)
{
    std::cerr << errnum << ": " << errmsg << std::endl;
}


} // namespace


void mainGL(GLFWwindow * window, std::istream * cin, bool saveScreenshots, bool printLog)
{
    glbinding::Binding::initialize(glfwGetProcAddress, false);

#ifndef NDEBUG
    glbinding::setAfterCallback([](const glbinding::FunctionCall & /*functionCall*/) {
        gl::GLenum error = glbinding::Binding::GetError.directCall();

        if (error != gl::GL_NO_ERROR)
        {
            std::cerr << "OpenGL Error " << std::hex << error << std::dec << std::endl;

            throw error;
        }
    });
#endif

    glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After, { "glGetError" });

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glfwSwapInterval(0);

    unshared_gpu_vector<float> dataBuffer;
    unified_read_vector<float> averageBuffer;
    unshared_gpu_vector<float> minBuffer;
    unshared_gpu_vector<float> maxBuffer;
    unified_read_vector<unsigned char> pixelBuffer;

    computation.initialize(cin, &dataBuffer, &averageBuffer, &minBuffer, &maxBuffer, 1024);
    rendering.initialize(&dataBuffer, &averageBuffer, &minBuffer, &maxBuffer, &pixelBuffer);
    rendering.resize(width, height);

    auto nextScreenshotTimestamp = std::uint64_t(0);
    while (!glfwWindowShouldClose(window)) // main loop
    {
        //        CPU <- unified: Load image from unified
        // DSK <- CPU       : Store image to disk
        if (computation.currentTime() > nextScreenshotTimestamp && saveScreenshots)
        {
            rendering.saveCapturedImage("screenshot" + std::to_string(computation.currentTime()));
            nextScreenshotTimestamp = computation.currentTime() + 1000000; // each second
        }

        //        CPU       : Handle window events
        glfwPollEvents();

        //        CPU       : Process sensor changes
        //        CPU -> unified: Update sensor data on unified
        const auto numNewEvents = computation.processEvents();

        //               unified: Compute average
        //        CPU <- unified: Transfer average
        //        CPU       : Write average to log
        //std::clog << numNewEvents << std::endl;
        if (numNewEvents > 0)
        {
            computation.computeAverage();

            if (!averageBuffer.empty() && printLog)
            {
                averageBuffer.wait();
                std::clog << computation.currentTime() << ";";
                std::clog << std::accumulate(averageBuffer.begin()+1, averageBuffer.begin()+computation.numSensors(), std::to_string(averageBuffer.front()), [](const std::string & res, float value) {
                    return res + ";" + std::to_string(value);
                }) << std::endl;
            }
        }

        //               unified: Render overview image
        rendering.updateGeometry(averageBuffer.size(), computation.numSensors(), computation.availablePoints(), computation.currentIndex());
        rendering.render();

        if (computation.currentTime() > nextScreenshotTimestamp && saveScreenshots)
        {
            rendering.captureImage();
        }

        //        CPU -> unified: Swap buffers
        glfwSwapBuffers(window);
    }
}


int main(int argc, char ** argv)
{
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    std::clog.tie(nullptr);
    std::cerr.tie(nullptr);

    const auto emulate = (argc >= 2 && std::string(argv[1]) == std::string("-e"))
        || (argc >= 3 && std::string(argv[2]) == std::string("-e"))
        || (argc >= 4 && std::string(argv[3]) == std::string("-e"));
    const auto saveScreenshots = (argc >= 2 && std::string(argv[1]) == std::string("-s"))
        || (argc >= 3 && std::string(argv[2]) == std::string("-s"))
        || (argc >= 4 && std::string(argv[3]) == std::string("-s"));
    const auto outputLog = (argc >= 2 && std::string(argv[1]) == std::string("-v"))
        || (argc >= 3 && std::string(argv[2]) == std::string("-v"))
        || (argc >= 4 && std::string(argv[3]) == std::string("-v"));

    auto input = &std::cin;
    std::unique_ptr<std::istream> emulation;
    if (emulate)
    {
        auto stream = new std::stringstream();

        std::mt19937_64 generator;
        std::normal_distribution<double> distribution(100.0,5.0);

        for (auto i = 0; i < 10000; ++i)
        {
            *stream << i;
            for (auto j = 0; j < 4; ++j)
            {
                 *stream << ';' << distribution(generator);
            }
            *stream << '\n';
        }

        emulation.reset(stream);
        input = stream;
    }

    glfwSetErrorCallback(errorCallback);

    if (!glfwInit())
    {
        return 1;
    }

    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow * window = nullptr;

    if (fullScreen)
    {
        const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

        window = glfwCreateWindow(mode->width, mode->height, windowTitle, glfwGetPrimaryMonitor(), nullptr);
    }
    else
    {
        window = glfwCreateWindow(canvasWidth, canvasHeight, windowTitle, nullptr, nullptr);
    }

    if (!window)
    {
        glfwTerminate();

        return 2;
    }

    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    glfwMakeContextCurrent(window);

    mainGL(window, input, saveScreenshots, outputLog);

    glfwMakeContextCurrent(nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
