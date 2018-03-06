
#include <glbinding/gl/gl.h>

template <typename T>
gl::GLuint unified_vector<T>::gpuIdentifier() const
{
    auto it = allocator_type::s_gpuBufferMap.find(this->data());

    if (it == allocator_type::s_gpuBufferMap.end())
    {
        return 0;
    }

    return it->second;
}

template <typename T>
void unified_vector<T>::startWait() const
{
    gl::glMemoryBarrier(gl::GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    m_sync = gl::glFenceSync(gl::GL_SYNC_GPU_COMMANDS_COMPLETE, gl::UnusedMask::GL_UNUSED_BIT);
}

template <typename T>
void unified_vector<T>::wait() const
{
    gl::glClientWaitSync(m_sync, gl::SyncObjectMask::GL_SYNC_FLUSH_COMMANDS_BIT, static_cast<gl::GLuint64>(-1));
    gl::glDeleteSync(m_sync);
}

template <typename T>
void unified_vector<T>::flush() const
{
    const auto buffer = gpuIdentifier();
    gl::glFlushMappedNamedBufferRange(buffer, 0, sizeof(T) * this->size());
}

template <typename T>
void unified_vector<T>::flush(std::size_t start, std::size_t end) const
{
    const auto buffer = gpuIdentifier();
    gl::glFlushMappedNamedBufferRange(buffer, sizeof(T) * start, sizeof(T) * (end - start));
}


template <typename T>
gl::GLuint unified_synchronized_vector<T>::gpuIdentifier() const
{
    auto it = allocator_type::s_gpuBufferMap.find(this->data());

    if (it == allocator_type::s_gpuBufferMap.end())
    {
        return 0;
    }

    return it->second;
}


template <typename T>
gl::GLuint unified_read_vector<T>::gpuIdentifier() const
{
    auto it = allocator_type::s_gpuBufferMap.find(this->data());

    if (it == allocator_type::s_gpuBufferMap.end())
    {
        return 0;
    }

    return it->second;
}

template <typename T>
void unified_read_vector<T>::startWait() const
{
    gl::glMemoryBarrier(gl::GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    m_sync = gl::glFenceSync(gl::GL_SYNC_GPU_COMMANDS_COMPLETE, gl::UnusedMask::GL_UNUSED_BIT);
}

template <typename T>
void unified_read_vector<T>::wait() const
{
    gl::glClientWaitSync(m_sync, gl::SyncObjectMask::GL_SYNC_FLUSH_COMMANDS_BIT, static_cast<gl::GLuint64>(-1));
    gl::glDeleteSync(m_sync);
}


template <typename T>
gl::GLuint unified_write_vector<T>::gpuIdentifier() const
{
    auto it = allocator_type::s_gpuBufferMap.find(this->data());

    if (it == allocator_type::s_gpuBufferMap.end())
    {
        return 0;
    }

    return it->second;
}

template <typename T>
void unified_write_vector<T>::flush() const
{
    const auto buffer = gpuIdentifier();
    gl::glFlushMappedNamedBufferRange(buffer, 0, sizeof(T) * this->size());
}

template <typename T>
void unified_write_vector<T>::flush(std::size_t start, std::size_t end) const
{
    const auto buffer = gpuIdentifier();
    gl::glFlushMappedNamedBufferRange(buffer, sizeof(T) * start, sizeof(T) * (end - start));
}


template <typename T>
gl::GLuint unshared_gpu_vector<T>::gpuIdentifier() const
{
    auto it = allocator_type::s_gpuBufferMap.find(this->data());

    if (it == allocator_type::s_gpuBufferMap.end())
    {
        return 0;
    }

    return it->second;
}
