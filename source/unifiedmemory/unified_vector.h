
#pragma once


#include <vector>

#include "unified_allocator.h"


template <typename T, typename Alloc = std::allocator<T>>
using unshared_cpu_vector = std::vector<T, Alloc>;


template <typename T>
class unified_vector : public std::vector<T, unified_allocator<T, read_write_flags>>
{
public:
    using allocator_type = unified_allocator<T, read_write_flags>;

    using std::vector<T, allocator_type>::vector;

    gl::GLuint gpuIdentifier() const;
    void flush() const;
    void flush(std::size_t start, std::size_t end) const;
    void startWait() const;
    void wait() const;

protected:
    mutable gl::GLsync m_sync;
};

template <typename T>
class unified_synchronized_vector : public std::vector<T, unified_allocator<T, coherent_flags>>
{
public:
    using allocator_type = unified_allocator<T, coherent_flags>;

    using std::vector<T, allocator_type>::vector;

    gl::GLuint gpuIdentifier() const;
};

template <typename T>
class unified_read_vector : public std::vector<T, unified_allocator<T, read_flags>>
{
public:
    using allocator_type = unified_allocator<T, read_flags>;

    using std::vector<T, allocator_type>::vector;

    gl::GLuint gpuIdentifier() const;
    void startWait() const;
    void wait() const;

protected:
    mutable gl::GLsync m_sync;
};

template <typename T>
class unified_write_vector : public std::vector<T, unified_allocator<T, write_flags>>
{
public:
    using allocator_type = unified_allocator<T, write_flags>;

    using std::vector<T, allocator_type>::vector;

    gl::GLuint gpuIdentifier() const;
    void flush() const;
    void flush(std::size_t start, std::size_t end) const;
};

template <typename T>
class unshared_gpu_vector : public std::vector<T, unified_allocator<T, no_map_flags>>
{
public:
    using allocator_type = unified_allocator<T, no_map_flags>;

    using std::vector<T, allocator_type>::vector;

    gl::GLuint gpuIdentifier() const;
};


#include "unified_vector.inl"
