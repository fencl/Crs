#pragma once
#ifndef _svtoi_crs_h
#define _svtoi_crs_h
#include <string_view> 
unsigned long long svtoi(std::string_view sv);
double svtod(std::string_view sv);
size_t rot(size_t n, int c);
size_t align_up(size_t value, size_t alignment);
#endif