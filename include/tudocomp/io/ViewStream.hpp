#ifndef _INCLUDED_VIEW_STREAM_HPP
#define _INCLUDED_VIEW_STREAM_HPP

#include <iostream>
#include <memory>
#include <utility>
#include <streambuf>

namespace tudocomp {
namespace io {

/// A wrapper around a istream that reads from
/// a existing memory buffer.
class ViewStream {
    struct membuf: public std::streambuf {
        inline membuf(char* begin, size_t size) {
            setg(begin, begin, begin + size);
        }

        virtual inline std::streampos seekpos(std::streampos sp,
                                              std::ios_base::openmode which) override
        {
            auto begin = eback();
            auto end = egptr();
            if ((size_t(begin) + sp) > size_t(end)) {
                return std::streampos(std::streamoff(-1));
            }
            auto current = begin + sp;
            setg(begin, current, end);
            return sp;
        }

        virtual inline std::streampos seekoff(std::streamoff off,
                                              std::ios_base::seekdir way,
                                              std::ios_base::openmode which) override
        {
            auto begin = eback();
            auto current = gptr();
            auto end = egptr();
            auto size = end - begin;

            std::streampos abs_pos = current - begin;

            if (way == std::ios_base::beg) {
                abs_pos = off;
            } else if (way == std::ios_base::cur) {
                abs_pos += off;
            } else /* way == std::ios_base::end */ {
                abs_pos = size - off;
            }
            return seekpos(abs_pos, which);
        }

    };

    char* m_begin;
    size_t m_size;
    std::unique_ptr<membuf> m_mb;
    std::unique_ptr<std::istream> m_stream;

public:
    inline ViewStream(char* begin, size_t size):
        m_begin(begin),
        m_size(size),
        m_mb(std::make_unique<membuf>(membuf { begin, size })),
        m_stream(std::unique_ptr<std::istream>(new std::istream(&*m_mb))) {}

    ViewStream(const ViewStream& other):
        ViewStream(other.m_begin, other.m_size) {}

    ViewStream(ViewStream&& other):
        m_begin(other.m_begin),
        m_size(other.m_size),
        m_mb(std::move(other.m_mb)),
        m_stream(std::move(other.m_stream)) {}

    std::istream& stream() {
        return *m_stream;
    }
};

}}

#endif

