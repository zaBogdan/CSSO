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

struct Counters
{
	int proccess = 0, module = 0, thread = 0;
	bool dump(char* buff)
	{
		char* data = new char[100];
		sprintf_s(data, 100, "Module: %d\nProcese: %d\nFire: %d", this->module, this->proccess, this->thread);
		memcpy(buff, data, 100);
		return true;
	}
};

struct MMData
{
	char data[100];
};