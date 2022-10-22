#pragma once
#include <string>

#define PRINT(level ,format, ...) \
{ \
	printf("[%s]", level);\
	printf(format, ##__VA_ARGS__);\
	printf("\n");\
}

#define CHECK(c, retVal, format, ...) \
{ \
	if (!(c)) { \
		PRINT("CHECK", format, ##__VA_ARGS__); \
		return retVal; \
	} \
}

#define WARNING(c, format, ...) \
{ \
	if ((c)) { \
		PRINT("WARNING",format, ##__VA_ARGS__); \
	} \
}

struct DirectoryStatsCTX {
	int file_count;
	const char* path;
	const char* used_files[40];
	std::string& buffer;
};