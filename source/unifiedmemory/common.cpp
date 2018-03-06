
#include "common.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <utility>
#include <algorithm>

#include <glm/vec4.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>

#include <glbinding/gl/gl.h>

#include <cpplocate/cpplocate.h>
#include <cpplocate/utils.h>

using namespace gl;


namespace
{


std::string determineDataPath()
{
    std::string path = cpplocate::locatePath("data/unifiedmemory", "share/unifiedmemory", nullptr);
    if (path.empty()) path = "./data";
    else              path = path + "/data";

    return path;
}


} // namespace


// Read raw binary file into a char vector (probably the fastest way).
std::vector<char> rawFromFile(const std::string & filePath)
{
    auto stream = std::ifstream(filePath, std::ios::in | std::ios::binary | std::ios::ate);

    if (!stream)
    {
        std::cerr << "Reading from file '" << filePath << "' failed." << std::endl;
        return std::vector<char>();
    }

    stream.seekg(0, std::ios::end);

    const auto size = stream.tellg();
    auto raw = std::vector<char>(size);

    stream.seekg(0, std::ios::beg);
    stream.read(raw.data(), size);

    return raw;
}

std::vector<float> rawFromFileF(const std::string &filePath)
{
    auto stream = std::ifstream(filePath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    if (!stream)
    {
        std::cerr << "Reading from file '" << filePath << "' failed." << std::endl;
        return std::vector<float>();
    }

    stream.seekg(0, std::ios::end);

    const auto size = stream.tellg();
    auto raw = std::vector<float>(size / sizeof(float));

    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char *>(raw.data()), (size / sizeof(float)) * sizeof(float));

    return raw;
}

std::string textFromFile(const std::string & filePath)
{
    const auto text = rawFromFile(filePath);
    return std::string(text.begin(), text.end());
}

void setShaderSourceAndCompile(const GLuint shader, const std::string & filename)
{
    const auto shaderSource = textFromFile(filename);
    const auto shaderSource_ptr = shaderSource.c_str();
    if (shaderSource_ptr)
        gl::glShaderSource(shader, 1, &shaderSource_ptr, 0);

    gl::glCompileShader(shader);
}

bool checkForCompilationError(GLuint shader, const std::string & identifier)
{
    auto success = static_cast<GLint>(GL_FALSE);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != static_cast<GLint>(GL_FALSE))
        return true;

    auto length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    std::vector<char> log(length);

    glGetShaderInfoLog(shader, length, &length, log.data());

    std::cerr
        << "Compiler error in " << identifier << ":" << std::endl
        << std::string(log.data(), length) << std::endl;

    return false;
}

bool rawToFile(const std::string & filePath, const std::vector<char> & raw)
{
    auto stream = std::ofstream(filePath, std::ios::out | std::ios::binary);

    if (!stream)
    {
        std::cerr << "Writing to file '" << filePath << "' failed." << std::endl;
        return false;
    }

    stream.write(raw.data(), raw.size());

    return true;
}

bool checkForLinkerError(GLuint program, const std::string & identifier)
{
    auto success = static_cast<GLint>(GL_FALSE);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != static_cast<GLint>(GL_FALSE))
        return true;

    auto length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    std::vector<char> log(length);

    glGetProgramInfoLog(program, length, &length, log.data());

    std::cerr
        << "Linker error in " << identifier << ":" << std::endl
        << std::string(log.data(), length) << std::endl;

    return false;

}

const std::string & dataPath()
{
    static const auto path = determineDataPath();

    return path;
}
