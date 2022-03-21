//===================================================================
// File: circular_buffer.cpp
//
// Desc: A Circular Buffer implementation in C++.
// From: https://gist.github.com/edwintcloud/d547a4f9ccaf7245b06f0e8782acefaa
//
// Copyright © 2019 Edwin Cloud. All rights reserved.
//
//===================================================================

#include "pch.h"

//-------------------------------------------------------------------
// Circular_Buffer (Class)
//     We will implement the buffer with a templated class so
//     the buffer can be a buffer of specified type.
//-------------------------------------------------------------------
template <class T>
class Circular_Buffer {
private:
	std::unique_ptr<T[]> buffer; // Using a smart pointer is safer (and we don't have to implement a destructor)
	size_t head = 0;             // size_t is an unsigned long
	size_t tail = 0;
	size_t max_size;
	T empty_item; // We will use this to clear data
public:
	// Create a new Circular_Buffer.
	Circular_Buffer(size_t max_size)
	    : buffer(std::unique_ptr<T[]>(new T[max_size])), max_size(max_size){};

	// Add an item to this circular buffer.
	void enqueue(T item) {

		// If buffer is full, throw an error
		if (is_full()) {
			throw std::runtime_error("buffer is full");
		}

		// Insert item at back of buffer
		buffer[tail] = item;

		// Increment tail
		tail = (tail + 1) % max_size;
	}

	// Remove an item from this circular buffer and return it.
	T dequeue() {

		// If buffer is empty, throw an error
		if (is_empty()) {
			throw std::runtime_error("buffer is empty");
		}

		// Get item at head
		T item = buffer[head];

		// Set item at head to be empty
		T empty;
		buffer[head] = empty_item;

		// Move head foward
		head = (head + 1) % max_size;

		// Return item
		return item;
	}

	// Return the item at the front of this circular buffer.
	T front() {
		return buffer[head];
	}

	// Return true if this circular buffer is empty, and false otherwise.
	bool is_empty() {
		return head == tail;
	}

	// Return true if this circular buffer is full, and false otherwise.
	bool is_full() {
		return tail == (head - 1) % max_size;
	}

	// Return the size of this circular buffer.
	size_t size() {
		if (tail >= head) {
			return tail - head;
		}
		return max_size - head - tail;
	}
};