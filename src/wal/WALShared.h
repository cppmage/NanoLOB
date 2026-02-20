#pragma once


#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>
#include <new>
#include <cstddef>

using namespace boost;

namespace lob {

	
	static const char* WAL_FILE_NAME = "wal_buffer.bin";

	template<size_t power_of_two = 20>
	class WALShared {
	private:
		static constexpr size_t WAL_DATA_SIZE = (1ULL << power_of_two);
		static constexpr size_t mask = WAL_DATA_SIZE - 1;

		struct WALHeader {
			alignas(std::hardware_destructive_interference_size) std::atomic<size_t> head{ 0 };
			alignas(std::hardware_destructive_interference_size) std::atomic<size_t> tail{ 0 };
		};
		static constexpr size_t HEADER_SIZE = sizeof(WALHeader);
		

		interprocess::managed_mapped_file mfile;

		WALHeader* header;
		std::byte* buffer;
	public:
		WALShared(const char* filename = WAL_FILE_NAME) : mfile(interprocess::open_or_create, filename, WAL_DATA_SIZE + HEADER_SIZE + 1024), header(nullptr), buffer(nullptr){
			
			header = mfile.find_or_construct<WALHeader>("Header")();

			

			buffer = mfile.find_or_construct<std::byte>("Data")[WAL_DATA_SIZE]();

			for (size_t i = 0; i < WAL_DATA_SIZE; i += 4096) {
				volatile std::byte b = buffer[i];
				(void)b;
			}

		}
	private:
		bool try_push(const std::byte* src, size_t len) {

			size_t t = header->tail.load(std::memory_order::acquire);
			size_t h = header->head.load(std::memory_order::acquire);

			if (WAL_DATA_SIZE < len + (t - h)) [[unlikely]] {
				return false;
			}

			size_t pos = t & mask;

			size_t space_until_end = WAL_DATA_SIZE - pos;
			if (len <= space_until_end) {
				std::memcpy(buffer + pos, src, len);
			}
			else {
				std::memcpy(buffer + pos, src, space_until_end);
				std::memcpy(buffer, src + space_until_end, len - space_until_end);
			}
			header->tail.fetch_add(len, std::memory_order_release);
			return true;
		}

		bool try_pop(std::byte* dst, size_t len) {
			size_t t = header->tail.load(std::memory_order::acquire);
			size_t h = header->head.load(std::memory_order::acquire);
			if ((t - h) < len)[[unlikely]] {
				return false;
			}

			size_t pos = h & mask;
			size_t space_until_end = WAL_DATA_SIZE - pos;

			if (len <= space_until_end) {
				std::memcpy(dst, buffer + pos, len);
			}
			else {
				std::memcpy(dst, buffer + pos, space_until_end);
				std::memcpy(dst + space_until_end, buffer, len - space_until_end);
			}

			header->head.fetch_add(len, std::memory_order_release);

			return true;

		}

	public:
		template<typename T>
		bool try_push_object(const T& obj) {
			static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable!");

			return try_push(reinterpret_cast<const std::byte*>(&obj), sizeof(T));
		}
		template<typename T>
		bool try_push_batch(const T* ptr, size_t n) {

			return try_push(reinterpret_cast<const std::byte*>(ptr), sizeof(T)*n);
		}

		template<typename T>
		bool try_pop_object(T& obj) {
			static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable!");

			return try_pop(reinterpret_cast<std::byte*>(&obj), sizeof(T));
		}
		void clear() {
			header->head = 0;
			header->tail = 0;
		}

	};

	using WALQueue = WALShared<27>;
	using WALSnapshotStatsQueue = WALShared<23>;
}

