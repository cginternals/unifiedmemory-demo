
#include <cstdlib>
#include <iostream>
#include <typeinfo>

template <class T, class unified_flags>
std::map<typename unified_allocator<T, unified_flags>::const_pointer, gl::GLuint> unified_allocator<T, unified_flags>::s_gpuBufferMap;

template <class T, class unified_flags>
typename unified_allocator<T, unified_flags>::pointer
unified_allocator<T, unified_flags>::allocate(size_type n, typename std::allocator<void>::const_pointer hint)
{
    static const auto mapFlags = unified_flags::mapFlags;
    static const auto storageFlags = unified_flags::storageFlags;

    (void)hint;

    const auto byteSize = n * sizeof(T);

    gl::GLuint bufferId = 0;
    gl::glCreateBuffers(1, &bufferId);

    gl::glNamedBufferStorage(bufferId, byteSize, nullptr, storageFlags);

    auto res = pointer(nullptr);

    if ((gl::GL_MAP_PERSISTENT_BIT & mapFlags) != gl::MapBufferAccessMask::GL_NONE_BIT)
    {
        res = reinterpret_cast<pointer>(gl::glMapNamedBufferRange(bufferId, 0, byteSize, mapFlags));
    }
    else
    {
        res = reinterpret_cast<pointer>(malloc(byteSize));
    }

    s_gpuBufferMap[res] = bufferId;

    return res;
}

template <class T, class unified_flags>
void
unified_allocator<T, unified_flags>::deallocate(pointer p, size_type)
{
    static const auto mapFlags = unified_flags::mapFlags;

    auto bufferId = s_gpuBufferMap[p];

    if (bufferId > 0)
    {
        if ((gl::GL_MAP_PERSISTENT_BIT & mapFlags) != gl::MapBufferAccessMask::GL_NONE_BIT)
        {
            gl::glUnmapNamedBuffer(bufferId);
        }
        else
        {
            free(p);
        }

        gl::glDeleteBuffers(1, &bufferId);
    }
}
