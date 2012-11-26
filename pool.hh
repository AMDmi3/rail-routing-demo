/*
 * Copyright (C) 2012 Dmitry Marakasov
 *
 * This file is part of rail routing demo.
 *
 * rail routing demo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rail routing demo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rail routing demo.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef POOL_H
#define POOL_H

#include <vector>
#include <stdexcept>

template<class T>
class pool {
private:
	typedef std::vector<T*> ChunkVector;
	ChunkVector chunks_;
	size_t last_chunk_free_;
	const size_t chunk_size_;

private:
	T* GetFreeSpace(size_t count) {
		if (count > chunk_size_)
			throw std::length_error("number of items exceed pool chunk size");

		if (last_chunk_free_ < count) {
			T* new_chunk = new T[chunk_size_];
			try {
				chunks_.push_back(new_chunk);
			} catch (...) {
				delete[] new_chunk;
				throw;
			}
			last_chunk_free_ = chunk_size_;
		}

		return chunks_.back() + (chunk_size_ - last_chunk_free_);
	}

public:
	pool(int chunk_size = 1024) : last_chunk_free_(0), chunk_size_(chunk_size) {
	}

	~pool() {
		for (typename ChunkVector::iterator i = chunks_.begin(); i != chunks_.end(); i++)
			delete[] *i;
	}

	T* alloc(size_t count) {
		T* where;
		new(where = GetFreeSpace(count)) T;
		last_chunk_free_ -= count;
		return where;
	}

	bool empty() const {
		return chunks_.empty();
	}

	void clear() {
		for (typename ChunkVector::iterator i = chunks_.begin(); i != chunks_.end(); i++)
			delete[] *i;

		chunks_.clear();
		last_chunk_free_ = 0;
	}

	void swap(pool<T>& other) {
		other.chunks_.swap(chunks_);
		std::swap(last_chunk_free_, other.last_chunk_free_);
		std::swap(const_cast<int&>(chunk_size_), const_cast<int&>(other.chunk_size_));
	}
};

#endif
