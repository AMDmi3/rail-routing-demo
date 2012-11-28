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

#ifndef LAZYINIT_ARRAY_HH
#define LAZYINIT_ARRAY_HH

#include <memory>

/**
 * Fixed-length vector with lazy initialization
 */
template <typename T, int EltPerPage = 16>
class lazyinit_array {
private:
	/* data, stored as in plain vector */
	std::unique_ptr<T[]> data_;

	/* bitmask of initialized pages */
	std::unique_ptr<unsigned char[]> page_initialized_;

	/* array size */
	size_t size_;

	/* default value to initialize vector with */
	T default_;

private:
	inline static size_t GetBitmapSize(size_t size) {
		return ((size + EltPerPage - 1)/EltPerPage + 7) / 8;
	}

public:
	typedef lazyinit_array<T, EltPerPage> self;

public:
	lazyinit_array(size_t size, T def)
		: data_(new T[size])
		, page_initialized_(new unsigned char[GetBitmapSize(size)])
		, size_(size)
		, default_(def) {
		std::fill(page_initialized_.get(), page_initialized_.get() + GetBitmapSize(size), 0);
	}

	lazyinit_array(const self& other)
		: data_(new T[other.size_])
		, page_initialized_(new unsigned char[GetBitmapSize(other.size_)])
		, size_(other.size_)
		, default_(other.default_) {
		std::copy(other.data_.get(), other.data_.get() + other.size_, data_.get());
		std::copy(page_initialized_.get(), page_initialized_.get() + GetBitmapSize(other.size_), page_initialized_.get());
	}

	~lazyinit_array() {
	}

	self& operator=(const self& other) {
		data_.reset(new T[other.size_]);
		page_initialized_.reset(new unsigned char[GetBitmapSize(other.size_)]);
		size_ = other.size_;
		default_ = other.default_;
		std::copy(other.data_.get(), other.data_.get() + other.size_, data_.get());
		std::copy(page_initialized_.get(), page_initialized_.get() + GetBitmapSize(other.size_), page_initialized_.get());
		return *this;
	}

	T& operator[](size_t n) {
		const size_t npage = n / EltPerPage;
		if (!(page_initialized_[npage / 8] & (1 << (npage % 8)))) {
			T* const page_start = data_.get() + npage * EltPerPage;
			std::fill(page_start, std::min(page_start + EltPerPage, data_.get() + size_), default_);

			page_initialized_[npage / 8] |= (1 << (npage % 8));
		}
		return *(data_.get() + n);
	}

	const T& operator[](size_t n) const {
		const size_t npage = n / EltPerPage;
		if (!(page_initialized_[npage / 8] & (1 << (npage % 8))))
			return default_; /* clever, huh? */
		return *(data_.get() + n);
	}

	size_t size() const {
		return size_;
	}
};

#endif
