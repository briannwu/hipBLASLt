#pragma once
#include <iostream>

//#define DEBUGMODE

#ifdef DEBUGMODE
    class MemTracker {
    public:
        static int count;
        MemTracker(const char* file, int line): name(file+std::to_string(line)) {
            classes[name] = classes[name] + 1;
        }
        virtual ~MemTracker() { classes[name] -= 1; }

        static std::unordered_map<std::string, int> classes;
        static void Dump() {
            for (auto& pair : classes) if (pair.second) 
                std::cout << pair.first << " " << pair.second << std::endl; 
        }
        const std::string name;
    };
#else
    class MemTracker {
    public:
        static int count;
        MemTracker(const char* file, int line) { count += 1; }
        virtual ~MemTracker() { count -= 1; }

        static std::unordered_map<std::string, int> classes;
        static void Dump() { if (count) std::cout << "Warning - Leftover allocs: " << count << std::endl; }
    };
#endif

#define set_tracked() private: MemTracker tracker = {__FILE__, __LINE__};

#define QWARNING(exp, msg, todo) if (!(exp)) { std::cout << "Warning: " << __FILE__ << ":" << __LINE__ << " " << msg << std::endl; todo; };
#define QASSERT(exp, msg) if (!(exp)) { std::cout << "Error: " << __FILE__ << ":" << __LINE__ << " " << msg << std::endl; __builtin_trap(); };

#define ENABLE_FPSLIMITER() true
#define FPS_LIMITER_TIMEOUT() 10 // 10ms = max 100 Hz refresh rate
