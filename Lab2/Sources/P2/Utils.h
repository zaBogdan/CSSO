#pragma once
#define EXIT_CODE_FAIL 12
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

struct ThreadData 
{
	const char* fileName;

	ThreadData(const char* fileName) : fileName(fileName) {};
};