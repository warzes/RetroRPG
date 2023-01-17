#pragma once

#include <assert.h>
#include <string.h>

template <class T>
class Vector
{
public:
	Vector() = default;
	Vector(size_t size)
	{
		m_capacity = size;
		m_array = new T[m_capacity];
		m_size = 0;
	}

	Vector(const Vector& other) { copyVector(other); }
	~Vector() { delete[] m_array; m_array = nullptr; }

	Vector& operator=(const Vector& other)
	{
		if( this == &other ) return *this;
		copyVector(other);
		return *this;
	}

	bool push_back(const T& element)
	{
		if( m_size >= m_capacity )
			grow();

		m_array[m_size] = element;
		m_size++;
		return true;
	}

	void pop()
	{
		if( m_size == 0 ) return;
		m_size--;
	}

	unsigned int size() const { return m_size; }
	unsigned int capacity() const { return m_capacity; }

	void resize(size_t size)
	{
		// пока сношу все данные, в будущем сохранять валидность
		delete[] m_array;
		m_array = new T[size];
		m_capacity = size;
		m_size = 0;
	}

	void clear()
	{
		delete[] m_array;
		m_capacity = 1u;
		m_array = new T[m_capacity];
		m_size = 0;
	}

	T& operator[](unsigned index)
	{
		assert(index < m_size);
		return m_array[index];
	}
	const T& operator[](unsigned index) const
	{
		assert(index < m_size);
		return m_array[index];
	}

private:
	void grow()
	{
		if( m_capacity == 0 ) m_capacity = 1;
		m_capacity *= 2;
		T* newVec = new T[m_capacity];
		memcpy(newVec, m_array, sizeof(T) * m_size);
		delete[] m_array;
		m_array = newVec;
	}
	void copyVector(const Vector<T>& other)
	{
		// TODO: если размер имеющегося массива больше или равен other массиву, то не удалять, а только копировать в память и менять capacity и size
		delete[] m_array;
		m_array = new T[other.capacity];
		memcpy(m_array, other.vect, sizeof(T) * m_size);
		capacity = other.capacity;
		size = other.size;
	}

	T* m_array = nullptr;
	size_t m_capacity;
	size_t m_size;
};