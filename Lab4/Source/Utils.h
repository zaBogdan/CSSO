#pragma once
#include <string>
#define FAIL_EXIT_CODE 17

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

struct RequestCTX {
	char* data;
	uint16_t size;
	std::string method;
	std::string path;
};

struct Metrics {
	uint16_t get_requests = 0;
	uint16_t post_requests = 0;
	uint16_t total_file_size = 0;
};