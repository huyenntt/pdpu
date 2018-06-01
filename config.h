// Automatically generated using env2h.py, do not edit!
#ifndef _CONFIG__
#define _CONFIG__

#define CONFIG_PREFIX "~/x/local"
#define CONFIG_VERSION "v0.5.2"
#define CONFIG_BUILD_DATE "Wed, 02 May 2018 14:54:51 +0200"
#define CONFIG_MAX_DEFECT_REPETITION 5
#define CONFIG_MAX_EVENTS_PER_PROCCESS 20000
#define CONFIG_CFLAGS "-Wall -std=c11 -g"
#define CONFIG_GUEST_DEFAULT_MEMORY_SIZE 134217728
#define CONFIG_BUILD_DIRTY 1
#define CONFIG_MAX_VERB_LEVEL 2
#define CONFIG_COMPILE "clang++-6.0 -Wall -std=c++11 -g -I ./src  -I ./lib/steroids/include/  -c -o config.h config.mk -fopenmp"
#define CONFIG_GUEST_TRACE_BUFFER_SIZE 1048576
#define CONFIG_GUEST_DEFAULT_THREAD_STACK_SIZE 1048576
#define CONFIG_DEBUG 1
#define CONFIG_BUILD_COMMIT "37f86be"
#define CONFIG_LINK "clang++-6.0  -L ./lib/steroids/src/  -o config.h config.mk ./lib/steroids/src/libsteroids.a -Wl,-Bstatic -lsteroids -Wl,-Bdynamic  -lz -lpthread -lffi -lncurses -ldl -lm -fopenmp"
#define CONFIG_LLVM_VER 6.0
#define CONFIG_MAX_PROCESSES 32
#define CONFIG_SKIP_STEP 4

#endif
