/*

Copyright (c) 2008-2016, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TORRENT_DISK_BUFFER_HOLDER_HPP_INCLUDED
#define TORRENT_DISK_BUFFER_HOLDER_HPP_INCLUDED

#include "libtorrent/config.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/disk_io_job.hpp" // for block_cache_reference
#include "libtorrent/span.hpp"

#include <memory>

namespace libtorrent
{
	struct disk_io_thread;
	struct disk_observer;
	struct disk_buffer_holder;

	struct TORRENT_EXTRA_EXPORT buffer_allocator_interface
	{
		virtual void free_disk_buffer(char* b) = 0;
		virtual void reclaim_blocks(span<block_cache_reference> refs) = 0;
		virtual disk_buffer_holder allocate_disk_buffer(char const* category) = 0;
		virtual disk_buffer_holder allocate_disk_buffer(bool& exceeded
			, std::shared_ptr<disk_observer> o
			, char const* category) = 0;
	protected:
		~buffer_allocator_interface() {}
	};

	// The disk buffer holder acts like a ``unique_ptr`` that frees a disk buffer
	// when it's destructed, unless it's released. ``release`` returns the disk
	// buffer and transfers ownership and responsibility to free it to the caller.
	//
	// ``get()`` returns the pointer without transferring ownership. If
	// this buffer has been released, ``get()`` will return nullptr.
	struct TORRENT_EXTRA_EXPORT disk_buffer_holder
	{
		// internal
		disk_buffer_holder(buffer_allocator_interface& alloc, char* buf) noexcept;

		disk_buffer_holder& operator=(disk_buffer_holder&&) noexcept;
		disk_buffer_holder(disk_buffer_holder&&) noexcept;

		disk_buffer_holder& operator=(disk_buffer_holder const&) = delete;
		disk_buffer_holder(disk_buffer_holder const&) = delete;

		// construct a buffer holder that will free the held buffer
		// using a disk buffer pool directly (there's only one
		// disk_buffer_pool per session)
		disk_buffer_holder(buffer_allocator_interface& alloc
			, block_cache_reference const& ref, char* buf) noexcept;

		// frees any unreleased disk buffer held by this object
		~disk_buffer_holder();

		// return the held disk buffer and clear it from the
		// holder. The responsibility to free it is passed on
		// to the caller
		char* release() noexcept;

		// return a pointer to the held buffer
		char* get() const noexcept { return m_buf; }

		// set the holder object to hold the specified buffer
		// (or nullptr by default). If it's already holding a
		// disk buffer, it will first be freed.
		void reset(char* buf = 0);
		void reset(block_cache_reference const& ref, char* buf);

		// swap pointers of two disk buffer holders.
		void swap(disk_buffer_holder& h) noexcept
		{
			TORRENT_ASSERT(h.m_allocator == m_allocator);
			std::swap(h.m_buf, m_buf);
			std::swap(h.m_ref, m_ref);
		}

		block_cache_reference ref() const noexcept { return m_ref; }

		// implicitly convertible to true if the object is currently holding a
		// buffer
		explicit operator bool() const noexcept { return m_buf != nullptr; }

	private:

		buffer_allocator_interface* m_allocator;
		char* m_buf;
		block_cache_reference m_ref;
	};

}

#endif
