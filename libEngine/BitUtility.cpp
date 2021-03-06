#include "BitUtility.h"



int BitUtility::CountTrailingZeros(uint32 n)
{
	int result = 0;
	if( (n & 0x0000ffff) == 0 ) { result += 16; n >>= 16; }
	if( (n & 0x000000ff) == 0 ) { result += 8; n >>= 8; }
	if( (n & 0x0000000f) == 0 ) { result += 4; n >>= 4; }
	if( (n & 0x00000004) == 0 ) { result += 2; n >>= 2; }
	if( (n & 0x00000002) == 0 ) { result += 1; n >>= 1; }
	return result + (n & 0x1);
}

int BitUtility::CountLeadingZeros(uint32 n)
{
	int result = 0;
	if( (n & 0xffff0000) != 0 ) { result += 16; n <<= 16; }
	if( (n & 0xff000000) != 0 ) { result += 8; n <<= 8; }
	if( (n & 0xf0000000) != 0 ) { result += 4; n <<= 4; }
	if( (n & 0xc0000000) != 0 ) { result += 2; n <<= 2; }
	if( (n & 0x80000000) != 0 ) { result += 1; }
	return result;
}


template< class T >
int BitCountSetGentic(T x)
{
	int count = 0;
	for( ; x; ++count )
		x &= x - 1;
	return count;
}

#define TWO(c)     (0x1u << (c))
#define MASK(c)    (((unsigned int)(-1)) / (TWO(TWO(c)) + 1u))
#define COUNT(x,c) ((x) & MASK(c)) + (((x) >> (TWO(c))) & MASK(c))

int BitUtility::CountSet(uint8 x)
{
	x = COUNT(x, 0);
	x = COUNT(x, 1);
	x = COUNT(x, 2);
	return x;
}


int BitUtility::CountSet(uint16 x)
{
	x = COUNT(x, 0);
	x = COUNT(x, 1);
	x = COUNT(x, 2);
	x = COUNT(x, 3);
	return x;

}

int BitUtility::CountSet(uint32 x)
{
	x = COUNT(x, 0);
	x = COUNT(x, 1);
	x = COUNT(x, 2);
	x = COUNT(x, 3);
	x = COUNT(x, 4);
	return x;
}


int BitUtility::CountSet(uint64 x)
{
	x = COUNT(x, 0);
	x = COUNT(x, 1);
	x = COUNT(x, 2);
	x = COUNT(x, 3);
	x = COUNT(x, 4);
	x = COUNT(x, 5);
	return x;
}

uint32 BitUtility::NextNumberOfPow2(uint32 n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;
	return n;
}

uint32 BitUtility::Reverse(uint32 n)
{
	n = ((n & 0xaaaaaaaa) >> 1) | ((n & 0x55555555) << 1);
	n = ((n & 0xcccccccc) >> 2) | ((n & 0x33333333) << 2);
	n = ((n & 0xf0f0f0f0) >> 4) | ((n & 0x0f0f0f0f) << 4);
	n = ((n & 0xff00ff00) >> 8) | ((n & 0x00ff00ff) << 8);
	return (n >> 16) | (n << 16);
}
