#include "Utilities.h"
#include <charconv>

size_t rot(size_t n, int c)
{
	const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
	c &= mask;
	return (n >> c) | (n << ((-c) & mask));
}


unsigned long long svtoi(std::string_view sv) {
	unsigned long long r = 0;
	for (int i = 0; i < sv.length(); i++) {
		r *= 10;
		r += (unsigned char)(sv[i] - '0');
	}
	return r;
}

double svtod(std::string_view sv)
{
	double dbl;
	auto result = std::from_chars(sv.data(), sv.data() + sv.size(), dbl);
	return dbl;
}