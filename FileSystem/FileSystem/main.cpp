#include "part.h"
#include "cluster_cache.h"
#include <iostream>
#include <regex>

void write5chars(char* arr, unsigned int start_pos) {
	for (int i = 0; i < 5; i++) {
		std::cout << arr[start_pos + i];
	}
}

int main() {	
}