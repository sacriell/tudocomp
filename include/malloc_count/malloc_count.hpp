#ifndef _INCLUDED_MALLOC_COUNT_HPP_
#define _INCLUDED_MALLOC_COUNT_HPP_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cstdint>

#define MEMBLOCK_MAGIC 0xFEDCBA9876543210

using ulong = unsigned long;

namespace malloc_count {

    inline ulong current_time_millis() {
        timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);

        return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
    }

    struct phase_t {
        size_t id;

        ulong time_start;
        ulong time_end;
        ssize_t mem_off;
        ssize_t mem_current;
        ssize_t mem_peak;
    };

    struct block_header_t {
        size_t magic;
        size_t phase_id;
        size_t size;
    };

    void begin_phase();
    phase_t end_phase();
    void pause_phase();
    void resume_phase();
}

extern "C" void* __libc_malloc(size_t);
extern "C" void __libc_free(void*);
extern "C" void* __libc_realloc(void*, size_t);

// We want to override these, but they are already defined in cstdlib,
// and might have implementation-specific throws declarations,
// so don't repeat the declarations manually here
//
// extern void* malloc(size_t size);
// extern void free(void* ptr);
// extern void* realloc(void* ptr, size_t size);
// extern void* calloc(size_t num, size_t size);

#endif
