#pragma once
#include <chrono>

class Timer {
	typedef std::chrono::time_point<std::chrono::high_resolution_clock> Clock;
	long long count;
	bool running;
	Clock prev_start_;
	Clock Now() {
		return std::chrono::high_resolution_clock::now();
	}
public:
	void Start() {
		running = true;
		prev_start_ = Now();
	}
	void Pause() {
		if (running) {
			running = false;
			auto diff = Now() - prev_start_;
			count += std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
		}
	}
	void Reset() {
		running = false;
		count = 0;
	}
	long long get_count() {
		return count;
	}
	Timer() {Reset();}
};

#define printf_timer(timer) printf("Timer: %lldms\n", timer.get_count());
// #define printf_timer(str, timer) printf("%s -- Timer: %lldms\n", str, timer.get_count());
