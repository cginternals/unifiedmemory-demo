
#pragma once


#include <cstddef>

#include <map>

#include <glbinding/gl/gl.h>


class coherent_flags
{
public:
    static constexpr auto mapFlags = gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT | gl::GL_MAP_COHERENT_BIT;
    static constexpr auto storageFlags = gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT | gl::GL_MAP_COHERENT_BIT;
};

class read_write_flags
{
public:
    static constexpr auto mapFlags = gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT | gl::GL_MAP_FLUSH_EXPLICIT_BIT;
    static constexpr auto storageFlags = gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT;
};

class read_flags
{
public:
    static constexpr auto mapFlags = gl::GL_MAP_READ_BIT | gl::GL_MAP_PERSISTENT_BIT;
    static constexpr auto storageFlags = gl::GL_MAP_READ_BIT | gl::GL_MAP_PERSISTENT_BIT;
};

class write_flags
{
public:
    static constexpr auto mapFlags = gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT | gl::GL_MAP_FLUSH_EXPLICIT_BIT | gl::GL_MAP_UNSYNCHRONIZED_BIT;
    static constexpr auto storageFlags = gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT;
};

class no_map_flags
{
public:
    static constexpr auto mapFlags = gl::GL_NONE_BIT;
    static constexpr auto storageFlags = gl::GL_NONE_BIT;
};


template <class T, class unified_flags>
class unified_allocator
{
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template <class U>
    struct rebind
    {
        using other = unified_allocator<U, unified_flags>;
    };

    inline unified_allocator() throw() {}
    inline unified_allocator(const unified_allocator&) throw() {}

    template <class U>
    inline unified_allocator(const unified_allocator<U, unified_flags>&) throw() {}

    inline ~unified_allocator() throw() {}

    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) const { return &r; }

    pointer allocate(size_type n, typename std::allocator<void>::const_pointer hint = 0);
    inline void deallocate(pointer p, size_type);

    inline void construct(pointer p, const_reference value) { new (p) value_type(value); }
    inline void destroy(pointer p) { p->~value_type(); }

    inline size_type max_size() const throw() { return size_type(-1) / sizeof(T); }

    inline bool operator==(const unified_allocator&) { return true; }
    inline bool operator!=(const unified_allocator& rhs) { return !operator==(rhs); }

    static std::map<const_pointer, gl::GLuint> s_gpuBufferMap;
};


#include "unified_allocator.inl"
