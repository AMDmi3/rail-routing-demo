#include <sys/time.h>
#include <stdio.h>

#define _PROFILE_START(ID) __profile_time_ ## ID ## _start
#define _PROFILE_STOP(ID) __profile_time_ ## ID ## _stop
#define _PROFILE_TMP(ID) __profile_time_ ## ID ## _tmp

#define PROFILE_START(ID) \
	struct timeval _PROFILE_START(ID); \
	struct timeval _PROFILE_STOP(ID); \
	gettimeofday(&_PROFILE_START(ID), 0);

#define PROFILE_STOP(ID) \
	gettimeofday(&_PROFILE_STOP(ID), 0);

#define PROFILE_REPEAT(ID, COUNT, FUNCTION) \
	PROFILE_START(ID); \
	int _PROFILE_TMP(ID); \
	for (_PROFILE_TMP(ID) = 0; _PROFILE_TMP(ID) < COUNT; _PROFILE_TMP(ID)++) \
		FUNCTION; \
	PROFILE_STOP(ID);

#define PROFILE_TEST(NAME, COUNT, FUNCTION) \
	{ \
		printf("%s: ", NAME); \
		PROFILE_REPEAT(_THISTEST, COUNT, FUNCTION); \
		printf("%.5f seconds (%.2f iterations/sec)\n", PROFILE_GET(_THISTEST), float(COUNT)/PROFILE_GET(_THISTEST)); \
	}

#define PROFILE_GET(ID) \
	((float)(_PROFILE_STOP(ID).tv_sec - _PROFILE_START(ID).tv_sec) + (float)(_PROFILE_STOP(ID).tv_usec - _PROFILE_START(ID).tv_usec)/1000000.0)
