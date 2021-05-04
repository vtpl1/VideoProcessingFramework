/*
 * Vtype_MIDT.h
 *
 *  Created on: 25-Jul-2013
 *      Author: Dipak Bhattacharyya
 */

#ifndef VTYPE_MIDT_H_
#define VTYPE_MIDT_H_

#include "Vtype.h"

union HostEndianness
{
	HostEndianness() : i(1)
	{
	}

	int i;
	char c[sizeof(int)];



	bool isBig() const { return c[0] == 0; }
	bool isLittle() const { return c[0] != 0; }
};


template <typename T, bool wantBig>
class FixedEndian
{
	T rep;

	static T swap(const T& arg)
	{
		if (HostEndianness().isBig() == wantBig) {
			// printf("\n no swap");
			return arg;
		}
		else
		{
			// printf("\n swap");
			T ret;

			char* dst = reinterpret_cast<char*>(&ret);
			const char* src = reinterpret_cast<const char*>(&arg + 1);

			for (unsigned int i = 0; i < sizeof(T); i++)
				*dst++ = *--src;

			return ret;
		}
	}

public:
	FixedEndian() {}
	FixedEndian(const T& t) : rep(swap(t)) {/* printf("\n Copy Cons()"); */}
	operator T() const { /*printf("\n operator T()");*/ return swap(rep); }
};


template <typename T>
class BigEndian : public FixedEndian<T, true>
{
public:
	BigEndian() {}
	BigEndian(const T& t) : FixedEndian<T, true>(t) { }
};

template <typename T>
class LittleEndian : public FixedEndian<T, false>
{
public:
	LittleEndian() {}
	LittleEndian(const T& t) : FixedEndian<T, false>(t) { }
};

typedef Char8						BChar8;
typedef UChar8						BUChar8;
typedef BigEndian<Int16>			BInt16;
typedef BigEndian<UInt16>			BUInt16;
typedef BigEndian<Int32>			BInt32;
typedef BigEndian<UInt32>			BUInt32;
typedef BigEndian<Long32>			BLong32;
typedef BigEndian<ULong32>			BULong32;
typedef BigEndian<Int64>			BInt64;
typedef BigEndian<UInt64>			BUInt64;
typedef BigEndian<Float32>			BFloat32;
typedef BigEndian<Float64>			BFloat64;

typedef Char8						LChar8;
typedef UChar8						LUChar8;
typedef LittleEndian<Int16>			LInt16;
typedef LittleEndian<UInt16>		LUInt16;
typedef LittleEndian<Int32>			LInt32;
typedef LittleEndian<UInt32>		LUInt32;
typedef LittleEndian<Long32>		LLong32;
typedef LittleEndian<ULong32>		LULong32;
typedef LittleEndian<Int64>			LInt64;
typedef LittleEndian<UInt64>		LUInt64;
typedef LittleEndian<Float32>		LFloat32;
typedef LittleEndian<Float64>		LFloat64;

#endif /* VTYPE_MIDT_H_ */
