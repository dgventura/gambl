
// Fairly basic substitute for std::vector<T> where T is a POD type
// (no constructor, destructor, or copy assignment operator), no initialization
// performed on new elements.

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef POD_VECTOR_H
#define POD_VECTOR_H

#include <cstddef>

class pod_vector_impl {
public:
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	
	void init( size_type sizeof_t );
	void destruct();
	
	size_type size() const          { return size_; }
	size_type capacity() const      { return capacity_; }
	bool empty() const              { return size_ == 0; }
	
	void reserve( size_type );
	void pop_back()                 { resize( size_ - 1 ); }
	void clear()                    { end = begin; size_ = 0; }
	
protected:
	char* begin;
	char* end;
	size_type size_;
	size_type capacity_;
	
	void erase( void* first, void* last );
	void resize( size_type );
	void swap_( pod_vector_impl& );
	
	// reserve space and return pointer to beginning
	void* insert( void*, size_type );
private:
	size_type sizeof_t;
	void expand( size_type new_size );
	void check_iter_( void* );
	void check_iter( void* );
};

template<class T>
class pod_vector : public pod_vector_impl
{
	typedef pod_vector_impl impl;
	
	// T must be a POD type
	union t_must_be_pod {
		T pod;
		int i;
	};
public:
	typedef T           value_type;
	typedef T*          pointer;
	typedef const T*    const_pointer;
	typedef T&          reference;
	typedef const T&    const_reference;
	typedef pointer     iterator;
	typedef const_pointer const_iterator;
	
	iterator begin()                { return (T*) impl::begin; }
	iterator end()                  { return (T*) impl::end; }
	
	const_iterator begin() const    { return (T*) impl::begin; }
	const_iterator end() const      { return (T*) impl::end; }
	
	size_type max_size() const      { return size_type (~0) / sizeof (T); }
	
	reference operator [] ( size_type n ) {
		assert( n < size() );
		return ((T*) impl::begin) [n];
	}
	
	const_reference operator [] ( size_type n ) const {
		assert( n < size() );
		return ((T*) impl::begin) [n];
	}
	
	pod_vector() {
		impl::init( sizeof (T) );
	}
	
	pod_vector( size_type n ) {
		impl::init( sizeof (T) );
		impl::resize( n );
	}
	
	~pod_vector()                   { impl::destruct(); }
	
	void swap( pod_vector& other )  { impl::swap_( other ); }
	
	reference front()               { return ((T*) impl::begin) [0]; }
	reference back()                { return ((T*) impl::end) [-1]; }
	
	const_reference front() const   { return ((T*) impl::begin) [0]; }
	const_reference back() const    { return ((T*) impl::end) [-1]; }
	
	void push_back( const T& t )                    { insert( end(), t ); }
	
	void insert( iterator p, const T& t )           { *(T*) impl::insert( p, 1 ) = t; }
	
	template<class Iter>
	inline void insert( iterator pos, Iter first, Iter last ) {
		std::copy( first, last, (T*) impl::insert( pos, std::distance( first, last ) ) );
	}
	
	void erase( iterator p )                        { impl::erase( p, p + 1 ); }
	void erase( iterator first, iterator last )     { impl::erase( first, last ); }
	
	void resize( size_type n )                      { impl::resize( n ); }
};

#endif

