#include "part.h"
#include "file.h"
#include "fs.h"

#include<iostream>
#include<fstream>
#include<cstdlib>
#include<ctime>
using namespace std;

int main() {
	clock_t start_time = clock();
	Partition* part = new Partition((char*)"p1.ini");
	FS::mount(part);
	char c = 'a';
	for (int i = 0; i < 2000; i++) {
		char name[] = { '/', 's', 'p', 'a', 'm', 'a' + i % 10, 'a' + (i / 10) % 10, 'a' + (i / 100) % 10, 'a' + (i / 1000) % 10, '.', 's', 'p', 'm', 0 };
		File* f = FS::open(name, 'r');
		f->read(1, &c);
		cout << c;
		delete f;
	}
	FS::unmount();
	clock_t end_time = clock();
	cout << end_time - start_time;
}