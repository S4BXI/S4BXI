/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Shared allocations are handled through shared memory segments.
 * Associated data and metadata are used as follows:
 *
 *                                                                    mmap #1
 *    `allocs' map                                                      ---- -.
 *    ----------      shared_data_t               shared_metadata_t   / |  |  |
 * .->| <name> | ---> -------------------- <--.   -----------------   | |  |  |
 * |  ----------      | fd of <name>     |    |   | size of mmap  | --| |  |  |
 * |                  | count (2)        |    |-- | data          |   \ |  |  |
 * `----------------- | <name>           |    |   -----------------     ----  |
 *                    --------------------    |   ^                           |
 *                                            |   |                           |
 *                                            |   |   `allocs_metadata' map   |
 *                                            |   |   ----------------------  |
 *                                            |   `-- | <addr of mmap #1>  |<-'
 *                                            |   .-- | <addr of mmap #2>  |<-.
 *                                            |   |   ----------------------  |
 *                                            |   |                           |
 *                                            |   |                           |
 *                                            |   |                           |
 *                                            |   |                   mmap #2 |
 *                                            |   v                     ---- -'
 *                                            |   shared_metadata_t   / |  |
 *                                            |   -----------------   | |  |
 *                                            |   | size of mmap  | --| |  |
 *                                            `-- | data          |   | |  |
 *                                                -----------------   | |  |
 *                                                                    \ |  |
 *                                                                      ----
 */
#include <array>
#include <cstring>
#include <map>

#include "s4bxi/s4bxi_util.hpp"
#include "s4bxi/s4bxi_shared.h"
#include "s4bxi/s4bxi_xbt_log.h"
#include "xbt/config.hpp"
#include "xbt/file.hpp"

#include <cerrno>

#include <sys/types.h>
#ifndef WIN32
#include <sys/mman.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_shared, "Logging specific to shared memory macros");



/**
 * @brief Uses shm_open to get a temporary shm, and returns its file descriptor.
 */
int s4bxi_temp_shm_get()
{
  constexpr unsigned VAL_MASK = 0xffffffffUL;
  static unsigned prev_val    = VAL_MASK;
  char shmname[32]; // cannot be longer than PSHMNAMLEN = 31 on macOS (shm_open raises ENAMETOOLONG otherwise)
  int fd;

  for (unsigned i = (prev_val + 1) & VAL_MASK; i != prev_val; i = (i + 1) & VAL_MASK) {
    snprintf(shmname, sizeof(shmname), "/s4bxi-buffer-%016x", i);
    fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd != -1 || errno != EEXIST) {
      prev_val = i;
      break;
    }
  }
  if (fd < 0) {
    if (errno == EMFILE) {
      xbt_die("Impossible to create temporary file for memory mapping: %s\n\
The shm_open() system call failed with the EMFILE error code (too many files). \n\n\
This means that you reached the system limits concerning the amount of files per process. \
This is not a surprise if you are trying to virtualize many processes on top of S4BXI. \
Don't panic -- you should simply increase your system limits and try again. \n\n\
First, check what your limits are:\n\
  cat /proc/sys/fs/file-max # Gives you the system-wide limit\n\
  ulimit -Hn                # Gives you the per process hard limit\n\
  ulimit -Sn                # Gives you the per process soft limit\n\
  cat /proc/self/limits     # Displays any per-process limitation (including the one given above)\n\n\
If one of these values is less than the amount of MPI processes that you try to run, then you got the explanation of this error. \
Ask the Internet about tutorials on how to increase the files limit such as: https://rtcamp.com/tutorials/linux/increase-open-files-limit/",
              strerror(errno));
    }
    xbt_die("Impossible to create temporary file for memory mapping. shm_open: %s", strerror(errno));
  }
  XBT_DEBUG("Got temporary shm %s (fd = %d)", shmname, fd);
  if (shm_unlink(shmname) < 0)
    XBT_WARN("Could not early unlink %s. shm_unlink: %s", shmname, strerror(errno));
  return fd;
}

/**
 * @brief Mmap a region of size bytes from temporary shm with file descriptor fd.
 */
void* s4bxi_temp_shm_mmap(int fd, size_t size)
{
  struct stat st;
  xbt_assert(fstat(fd, &st) == 0, "Could not stat fd %d: %s", fd, strerror(errno));
  xbt_assert(static_cast<off_t>(size) <= st.st_size || ftruncate(fd, static_cast<off_t>(size)) == 0,
             "Could not truncate fd %d to %zu: %s", fd, size, strerror(errno));
  void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  xbt_assert(
      mem != MAP_FAILED,
      "Failed to map fd %d with size %zu: %s\n"
      "If you are running a lot of ranks, you may be exceeding the amount of mappings allowed per process.\n"
      "On Linux systems, change this value with sudo sysctl -w vm.max_map_count=newvalue (default value: 65536)\n"
      "Please see https://simgrid.org/doc/latest/Configuring_SimGrid.html#configuring-the-user-code-virtualization for "
      "more information.",
      fd, size, strerror(errno));
  return mem;
}


/** Some location in the source code
 *
 *  This information is used by S4BXI_SHARED_MALLOC to allocate  some shared memory for all simulated processes.
 */

class s4bxi_source_location : public std::string {
  public:
    s4bxi_source_location() = default;
    s4bxi_source_location(const char* filename, int line)
        : std::string(std::string(filename) + ":" + std::to_string(line))
    {
    }
};

struct shared_data_t {
    int fd    = -1;
    int count = 0;
};

std::unordered_map<s4bxi_source_location, shared_data_t, std::hash<std::string>> allocs;
using shared_data_key_type = decltype(allocs)::value_type;

struct shared_metadata_t {
    size_t size;
    size_t allocated_size;
    void* allocated_ptr;
    std::vector<std::pair<size_t, size_t>> private_blocks;
    shared_data_key_type* data;
};

std::map<const void*, shared_metadata_t> allocs_metadata;
std::map<std::string, void*, std::less<>> calls;

#ifndef WIN32
int s4bxi_shared_malloc_bogusfile           = -1;
int s4bxi_shared_malloc_bogusfile_huge_page = -1;
unsigned long s4bxi_shared_malloc_blocksize = 1UL << 20;
#endif

void s4bxi_shared_destroy()
{
    allocs.clear();
    allocs_metadata.clear();
    calls.clear();
}

#ifndef WIN32
static void* shm_map(int fd, size_t size, shared_data_key_type* data)
{
    void* mem = s4bxi_temp_shm_mmap(fd, size);
    shared_metadata_t meta;
    meta.size            = size;
    meta.data            = data;
    meta.allocated_ptr   = mem;
    meta.allocated_size  = size;
    allocs_metadata[mem] = meta;
    XBT_DEBUG("MMAP %zu to %p", size, mem);
    return mem;
}

static void* s4bxi_shared_malloc_local(size_t size, const char* file, int line)
{
    void* mem;
    s4bxi_source_location loc(file, line);
    auto res  = allocs.insert(std::make_pair(loc, shared_data_t()));
    auto data = res.first;
    if (res.second) {
        // The new element was inserted.
        int fd             = s4bxi_temp_shm_get();
        data->second.fd    = fd;
        data->second.count = 1;
        mem                = shm_map(fd, size, &*data);
    } else {
        mem = shm_map(data->second.fd, size, &*data);
        data->second.count++;
    }
    XBT_DEBUG("Shared malloc %zu in %p through %d (metadata at %p)", size, mem, data->second.fd, &*data);
    return mem;
}

// Align functions, from http://stackoverflow.com/questions/4840410/how-to-align-a-pointer-in-c
#define ALIGN_UP(n, align)   (((int64_t)(n) + (int64_t)(align)-1) & -(int64_t)(align))
#define ALIGN_DOWN(n, align) ((int64_t)(n) & -(int64_t)(align))

constexpr unsigned PAGE_SIZE      = 0x1000;
constexpr unsigned HUGE_PAGE_SIZE = 1U << 21;

/* Similar to s4bxi_shared_malloc, but only sharing the blocks described by shared_block_offsets.
 * This array contains the offsets (in bytes) of the block to share.
 * Even indices are the start offsets (included), odd indices are the stop offsets (excluded).
 * For instance, if shared_block_offsets == {27, 42}, then the elements mem[27], mem[28], ..., mem[41] are shared.
 * The others are not.
 */

void* s4bxi_shared_malloc_partial(size_t size, const size_t* shared_block_offsets, int nb_shared_blocks)
{
    std::string huge_page_mount_point = S4BXI_GLOBAL_CONFIG(shared_malloc_hugepage);
    bool use_huge_page                = not huge_page_mount_point.empty();
#ifndef MAP_HUGETLB /* If the system header don't define that mmap flag */
    xbt_assert(not use_huge_page,
               "Huge pages are not available on your system, you cannot use the s4bxi/shared-malloc-hugepage option.");
#endif
    s4bxi_shared_malloc_blocksize = S4BXI_GLOBAL_CONFIG(shared_malloc_blocksize);
    void* mem;
    size_t allocated_size;
    if (use_huge_page) {
        xbt_assert(s4bxi_shared_malloc_blocksize == HUGE_PAGE_SIZE,
                   "the block size of shared malloc should be equal to the size of a huge page.");
        allocated_size = size + 2 * s4bxi_shared_malloc_blocksize;
    } else {
        xbt_assert(s4bxi_shared_malloc_blocksize % PAGE_SIZE == 0,
                   "the block size of shared malloc should be a multiple of the page size.");
        allocated_size = size;
    }

    /* First reserve memory area */
    void* allocated_ptr = mmap(nullptr, allocated_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    xbt_assert(allocated_ptr != MAP_FAILED,
               "Failed to allocate %zuMiB of memory. Run \"sysctl vm.overcommit_memory=1\" as root "
               "to allow big allocations.\n",
               size >> 20);
    if (use_huge_page)
        mem = (void*)ALIGN_UP(allocated_ptr, HUGE_PAGE_SIZE);
    else
        mem = allocated_ptr;

    XBT_DEBUG("global shared allocation. Blocksize %lu", s4bxi_shared_malloc_blocksize);
    /* Create a fd to a new file on disk, make it s4bxi_shared_malloc_blocksize big, and unlink it.
     * It still exists in memory but not in the file system (thus it cannot be leaked). */
    /* Create bogus file if not done already
     * We need two different bogusfiles:
     *    s4bxi_shared_malloc_bogusfile_huge_page is used for calls to mmap *with* MAP_HUGETLB,
     *    s4bxi_shared_malloc_bogusfile is used for calls to mmap *without* MAP_HUGETLB.
     * We cannot use a same file for the two type of calls, since the first one needs to be
     * opened in a hugetlbfs mount point whereas the second needs to be a "classical" file. */
    if (use_huge_page && s4bxi_shared_malloc_bogusfile_huge_page == -1) {
        std::string huge_page_filename          = huge_page_mount_point + "/simgrid-shmalloc-XXXXXX";
        s4bxi_shared_malloc_bogusfile_huge_page = mkstemp((char*)huge_page_filename.c_str());
        XBT_DEBUG("bogusfile_huge_page: %s\n", huge_page_filename.c_str());
        unlink(huge_page_filename.c_str());
    }
    if (s4bxi_shared_malloc_bogusfile == -1) {
        char name[]                   = "/tmp/simgrid-shmalloc-XXXXXX";
        s4bxi_shared_malloc_bogusfile = mkstemp(name);
        XBT_DEBUG("bogusfile         : %s\n", name);
        unlink(name);
        xbt_assert(ftruncate(s4bxi_shared_malloc_bogusfile, s4bxi_shared_malloc_blocksize) == 0,
                   "Could not write bogus file for shared malloc");
    }

    int mmap_base_flag = MAP_FIXED | MAP_SHARED | MAP_POPULATE;
    int mmap_flag      = mmap_base_flag;
    int huge_fd        = use_huge_page ? s4bxi_shared_malloc_bogusfile_huge_page : s4bxi_shared_malloc_bogusfile;
#ifdef MAP_HUGETLB
    if (use_huge_page)
        mmap_flag |= MAP_HUGETLB;
#endif

    XBT_DEBUG("global shared allocation, begin mmap");

    /* Map the bogus file in place of the anonymous memory */
    for (int i_block = 0; i_block < nb_shared_blocks; i_block++) {
        XBT_DEBUG("\tglobal shared allocation, mmap block %d/%d", i_block + 1, nb_shared_blocks);
        size_t start_offset = shared_block_offsets[2 * i_block];
        size_t stop_offset  = shared_block_offsets[2 * i_block + 1];
        xbt_assert(start_offset < stop_offset, "start_offset (%zu) should be lower than stop offset (%zu)",
                   start_offset, stop_offset);
        xbt_assert(stop_offset <= size, "stop_offset (%zu) should be lower than size (%zu)", stop_offset, size);
        if (i_block < nb_shared_blocks - 1)
            xbt_assert(stop_offset < shared_block_offsets[2 * i_block + 2],
                       "stop_offset (%zu) should be lower than its successor start offset (%zu)", stop_offset,
                       shared_block_offsets[2 * i_block + 2]);
        size_t start_block_offset = ALIGN_UP(start_offset, s4bxi_shared_malloc_blocksize);
        size_t stop_block_offset  = ALIGN_DOWN(stop_offset, s4bxi_shared_malloc_blocksize);
        for (size_t offset = start_block_offset; offset < stop_block_offset; offset += s4bxi_shared_malloc_blocksize) {
            XBT_DEBUG("\t\tglobal shared allocation, mmap block offset %zx", offset);
            void* pos       = static_cast<char*>(mem) + offset;
            const void* res = mmap(pos, s4bxi_shared_malloc_blocksize, PROT_READ | PROT_WRITE, mmap_flag, huge_fd, 0);
            xbt_assert(
                res == pos,
                "Could not map folded virtual memory (%s). Do you perhaps need to increase the "
                "size of the mapped file using S4BXI_SHARED_MALLOC_BLOCKSIZE=newvalue (default 1048576) ? "
                "You can also try using  the sysctl vm.max_map_count. "
                "If you are using huge pages, check that you have at least one huge page (/proc/sys/vm/nr_hugepages) "
                "and that the directory you are passing is mounted correctly (mount /path/to/huge -t hugetlbfs -o "
                "rw,mode=0777).",
                strerror(errno));
        }
        size_t low_page_start_offset = ALIGN_UP(start_offset, PAGE_SIZE);
        size_t low_page_stop_offset  = (int64_t)start_block_offset < ALIGN_DOWN(stop_offset, PAGE_SIZE)
                                          ? start_block_offset
                                          : ALIGN_DOWN(stop_offset, PAGE_SIZE);
        if (low_page_start_offset < low_page_stop_offset) {
            XBT_DEBUG("\t\tglobal shared allocation, mmap block start");
            void* pos       = static_cast<char*>(mem) + low_page_start_offset;
            const void* res = mmap(pos, low_page_stop_offset - low_page_start_offset, PROT_READ | PROT_WRITE,
                                   mmap_base_flag, // not a full huge page
                                   s4bxi_shared_malloc_bogusfile, 0);
            xbt_assert(res == pos,
                       "Could not map folded virtual memory (%s). Do you perhaps need to increase the "
                       "size of the mapped file using S4BXI_SHARED_MALLOC_BLOCKSIZE=newvalue (default 1048576) ?"
                       "You can also try using  the sysctl vm.max_map_count",
                       strerror(errno));
        }
        if (low_page_stop_offset <= stop_block_offset) {
            XBT_DEBUG("\t\tglobal shared allocation, mmap block stop");
            size_t high_page_stop_offset = stop_offset == size ? size : ALIGN_DOWN(stop_offset, PAGE_SIZE);
            if (high_page_stop_offset > stop_block_offset) {
                void* pos       = static_cast<char*>(mem) + stop_block_offset;
                const void* res = mmap(pos, high_page_stop_offset - stop_block_offset, PROT_READ | PROT_WRITE,
                                       mmap_base_flag, // not a full huge page
                                       s4bxi_shared_malloc_bogusfile, 0);
                xbt_assert(
                    res == pos,
                    "Could not map folded virtual memory (%s). Do you perhaps need to increase the "
                    "size of the mapped file using --cfg=s4bxi/shared-malloc-blocksize:newvalue (default 1048576) ?"
                    "You can also try using  the sysctl vm.max_map_count",
                    strerror(errno));
            }
        }
    }

    shared_metadata_t newmeta;
    // register metadata for memcpy avoidance
    auto* data             = new shared_data_key_type;
    data->second.fd        = -1;
    data->second.count     = 1;
    newmeta.size           = size;
    newmeta.data           = data;
    newmeta.allocated_ptr  = allocated_ptr;
    newmeta.allocated_size = allocated_size;
    if (shared_block_offsets[0] > 0) {
        newmeta.private_blocks.emplace_back(0, shared_block_offsets[0]);
    }
    int i_block;
    for (i_block = 0; i_block < nb_shared_blocks - 1; i_block++) {
        newmeta.private_blocks.emplace_back(shared_block_offsets[2 * i_block + 1],
                                            shared_block_offsets[2 * i_block + 2]);
    }
    if (shared_block_offsets[2 * i_block + 1] < size) {
        newmeta.private_blocks.emplace_back(shared_block_offsets[2 * i_block + 1], size);
    }
    allocs_metadata[mem] = newmeta;

    XBT_DEBUG("global shared allocation, allocated_ptr %p - %p", allocated_ptr,
              (void*)(((uint64_t)allocated_ptr) + allocated_size));
    XBT_DEBUG("global shared allocation, returned_ptr  %p - %p", mem, (void*)(((uint64_t)mem) + size));

    return mem;
}

void* s4bxi_shared_malloc_intercept(size_t size, const char* file, int line)
{
    if (S4BXI_GLOBAL_CONFIG(auto_shared_malloc_thresh) == 0 || size < S4BXI_GLOBAL_CONFIG(auto_shared_malloc_thresh)) {
        void* ptr = ::operator new(size);
        return ptr;
    } else {
        return s4bxi_shared_malloc(size, file, line);
    }
}

void* s4bxi_shared_calloc_intercept(size_t num_elm, size_t elem_size, const char* file, int line)
{
    if (S4BXI_GLOBAL_CONFIG(auto_shared_malloc_thresh) == 0 || elem_size * num_elm < S4BXI_GLOBAL_CONFIG(auto_shared_malloc_thresh)) {
        void* ptr = ::operator new(elem_size* num_elm);
        memset(ptr, 0, elem_size * num_elm);
        return ptr;
    } else {
        return s4bxi_shared_malloc(elem_size * num_elm, file, line);
    }
}

void* s4bxi_shared_malloc(size_t size, const char* file, int line)
{
    if (size > 0 && S4BXI_GLOBAL_CONFIG(shared_malloc) == (int) SharedMallocType::LOCAL) {
        return s4bxi_shared_malloc_local(size, file, line);
    } else if (S4BXI_GLOBAL_CONFIG(shared_malloc) == (int) SharedMallocType::GLOBAL) {
        int nb_shared_blocks                             = 1;
        const std::array<size_t, 2> shared_block_offsets = {{0, size}};
        return s4bxi_shared_malloc_partial(size, shared_block_offsets.data(), nb_shared_blocks);
    }
    XBT_DEBUG("Classic allocation of %zu bytes", size);
    return ::operator new(size);
}

int s4bxi_is_shared(const void* ptr, std::vector<std::pair<size_t, size_t>>& private_blocks, size_t* offset)
{
    private_blocks.clear(); // being paranoid
    if (allocs_metadata.empty())
        return 0;
    if (S4BXI_GLOBAL_CONFIG(shared_malloc) == (int) SharedMallocType::LOCAL ||
        S4BXI_GLOBAL_CONFIG(shared_malloc) == (int) SharedMallocType::GLOBAL) {
        auto low = allocs_metadata.lower_bound(ptr);
        if (low != allocs_metadata.end() && low->first == ptr) {
            private_blocks = low->second.private_blocks;
            *offset        = 0;
            return 1;
        }
        if (low == allocs_metadata.begin())
            return 0;
        low--;
        if (ptr < (char*)low->first + low->second.size) {
            xbt_assert(ptr > (char*)low->first, "Oops, there seems to be a bug in the shared memory metadata.");
            *offset        = ((uint8_t*)ptr) - ((uint8_t*)low->first);
            private_blocks = low->second.private_blocks;
            return 1;
        }
        return 0;
    } else {
        return 0;
    }
}

std::vector<std::pair<size_t, size_t>> shift_and_frame_private_blocks(const std::vector<std::pair<size_t, size_t>>& vec,
                                                                      size_t offset, size_t buff_size)
{
    std::vector<std::pair<size_t, size_t>> result;
    for (auto const& block : vec) {
        auto new_block = std::make_pair(std::min(std::max((size_t)0, block.first - offset), buff_size),
                                        std::min(std::max((size_t)0, block.second - offset), buff_size));
        if (new_block.second > 0 && new_block.first < buff_size)
            result.push_back(new_block);
    }
    return result;
}

std::vector<std::pair<size_t, size_t>> merge_private_blocks(const std::vector<std::pair<size_t, size_t>>& src,
                                                            const std::vector<std::pair<size_t, size_t>>& dst)
{
    std::vector<std::pair<size_t, size_t>> result;
    unsigned i_src = 0;
    unsigned i_dst = 0;
    while (i_src < src.size() && i_dst < dst.size()) {
        std::pair<size_t, size_t> block;
        if (src[i_src].second <= dst[i_dst].first) {
            i_src++;
        } else if (dst[i_dst].second <= src[i_src].first) {
            i_dst++;
        } else { // src.second > dst.first && dst.second > src.first → the blocks are overlapping
            block = std::make_pair(std::max(src[i_src].first, dst[i_dst].first),
                                   std::min(src[i_src].second, dst[i_dst].second));
            result.push_back(block);
            if (src[i_src].second < dst[i_dst].second)
                i_src++;
            else
                i_dst++;
        }
    }
    return result;
}

void s4bxi_shared_free(void* ptr)
{
    if (S4BXI_GLOBAL_CONFIG(shared_malloc) == (int) SharedMallocType::LOCAL) {
        auto meta = allocs_metadata.find(ptr);
        if (meta == allocs_metadata.end()) {
            ::operator delete(ptr);
            return;
        }
        shared_data_t* data = &meta->second.data->second;
        if (munmap(meta->second.allocated_ptr, meta->second.allocated_size) < 0) {
            XBT_WARN("Unmapping of fd %d failed: %s", data->fd, strerror(errno));
        }
        data->count--;
        if (data->count <= 0) {
            close(data->fd);
            allocs.erase(allocs.find(meta->second.data->first));
            allocs_metadata.erase(ptr);
            XBT_DEBUG("Shared free - Local - with removal - of %p", ptr);
        } else {
            XBT_DEBUG("Shared free - Local - no removal - of %p, count = %d", ptr, data->count);
        }

    } else if (S4BXI_GLOBAL_CONFIG(shared_malloc) == (int) SharedMallocType::GLOBAL) {
        auto meta = allocs_metadata.find(ptr);
        if (meta != allocs_metadata.end()) {
            meta->second.data->second.count--;
            XBT_DEBUG("Shared free - Global - of %p", ptr);
            munmap(ptr, meta->second.size);
            if (meta->second.data->second.count == 0) {
                delete meta->second.data;
                allocs_metadata.erase(ptr);
            }
        } else {
            ::operator delete(ptr);
            return;
        }

    } else {
        XBT_DEBUG("Classic deallocation of %p", ptr);
        ::operator delete(ptr);
    }
}
#endif

int s4bxi_shared_known_call(const char* func, const char* input)
{
    std::string loc = std::string(func) + ":" + input;
    return calls.find(loc) != calls.end();
}

void* s4bxi_shared_get_call(const char* func, const char* input)
{
    std::string loc = std::string(func) + ":" + input;

    return calls.at(loc);
}

void* s4bxi_shared_set_call(const char* func, const char* input, void* data)
{
    std::string loc = std::string(func) + ":" + input;
    calls[loc]      = data;
    return data;
}
