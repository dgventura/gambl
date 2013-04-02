
// Simple run-time allocated array. Can only be resized once.

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef RUNTIME_ARRAY_H
#define RUNTIME_ARRAY_H

#include <algorithm>
#include <cstddef>
#include <cassert>
#include "util.h"

// Base class for classes whose objects can't be copied or assigned to
class noncopyable {
private:
	noncopyable( const noncopyable& );
	noncopyable& operator = ( const noncopyable& );
public:
	noncopyable() { }
};

template<class T>
class runtime_array : private noncopyable {
	T* begin_;
	T* end_;
	std::size_t size_;
	void init( std::size_t );
public:
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T&          reference;
	typedef const T&    const_reference;
	typedef T*          iterator;
	typedef const T*    const_iterator;
	
	runtime_array()                 : begin_( NULL ), end_( NULL ), size_( 0 ) { }
	explicit runtime_array( size_type n ) { init( n ); }
	~runtime_array();
	
	reference operator [] ( size_type i ) {
		assert( i < size_ );
		return begin_ [i];
	}
	
	const_reference operator [] ( size_type i ) const {
		assert( i < size_ );
		return begin_ [i];
	}
	
	void clear();
	
	void resize( size_type n )      { assert( !begin_ ); init( n ); }
	size_type size() const          { return size_; }
	
	const_iterator begin() const    { return begin_; }
	const_iterator end() const      { return end_; }
	
	iterator begin()                { return begin_; }
	iterator end()                  { return end_; }
	
	void swap( runtime_array& );
};

template<class T>
void runtime_array<T>::init( size_type n ) {
	size_ = n;
	begin_ = new T [n];
	end_ = begin_ + size_;
}

template<class T>
inline void runtime_array<T>::clear() {
	delete [] begin_;
	begin_ = NULL;
	end_ = NULL;
	size_ = 0;
}

template<class T>
inline void runtime_array<T>::swap( runtime_array& other ) {
	std::swap( begin_, other.begin_ );
	std::swap( end_, other.end_ );
	std::swap( size_, other.size_ );
}

template<class T>
runtime_array<T>::~runtime_array() {
	delete [] begin_;
}

#endif

