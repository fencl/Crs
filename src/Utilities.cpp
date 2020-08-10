#include "Utilities.hpp"
#include <charconv>

size_t rot(size_t n, int c)
{
	const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
	c &= mask;
	return (n >> c) | (n << ((-c) & mask));
}


unsigned long long svtoi(std::string_view sv) {
	unsigned long long r = 0;
	for (size_t i = 0; i < sv.length(); i++) {
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


size_t align_up(size_t value, size_t alignment) {
	return alignment == 0 ? value : ((value % alignment == 0) ? value : value + (alignment - (value % alignment)));
}

uint32_t upper_power_of_two(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}