#include "part.h"
#include "file.h"
#include "fs.h"
#include <iostream>
#include <fstream>

using namespace std;
char* ulazBuffer;
unsigned int ulazSize;

int main(){
	Partition* part = new Partition((char*)"p1.ini");
	FS::mount(part);
	FS::format();

	{//ucitavamo ulazni fajl u bafer, da bi nit 1 i 2 mogle paralelno da citaju
		FILE* f = fopen("ulaz.dat", "rb");
		if (f == 0) {
			cout << "GRESKA: Nije nadjen ulazni fajl 'ulaz.dat' u os domacinu!" << endl;
			system("PAUSE");
			return 0;//exit program
		}
		ulazBuffer = new char[32 * 1024 * 1024];//32MB
		ulazSize = fread(ulazBuffer, 1, 32 * 1024 * 1024, f);
		fclose(f);
	}
	{
		char filepath[] = "/fajl1.dat";
		File* f = FS::open(filepath, 'w');
		f->write(ulazSize, ulazBuffer);
		delete f;
	}
	
	{
		char filepath[] = "/fajl1.dat";
		File* f = FS::open(filepath, 'r');
		char filepath1[] = "/test1.dat";
		File* dst = FS::open(filepath1, 'w');
		char* buff = new char[f->getFileSize() / 2];
		f->read(f->getFileSize() / 2, buff);
		dst->write(f->getFileSize() / 2, buff);
		dst->seek(dst->getFileSize() / 2);
		dst->truncate();
		delete[] buff;
		delete f;
		delete dst;
	}

	{
		char filepath[] = "/fajl1.dat";
		File* f = FS::open(filepath, 'r');
		char filepath1[] = "/test2.dat";
		char* buff = new char[f->getFileSize() / 4];
		File* dst = FS::open(filepath1, 'w');
		f->seek(f->getFileSize() / 4);
		f->read(f->getFileSize() / 4, buff);
		dst->write(f->getFileSize() / 4, buff);
		delete[] buff;
		delete f;
		delete dst;
	}

	{
		char filepath[] = "/fajl1.dat";
		File* f = FS::open(filepath, 'r');
		char filepath1[] = "/test3.dat";
		char* buff = new char[f->getFileSize() / 2];
		f->seek(f->getFileSize() / 2);
		f->read(f->getFileSize() / 2, buff);
		File* dst = FS::open(filepath1, 'w');
		dst->write(f->getFileSize() / 2, buff);
		dst->seek(dst->getFileSize() / 2);
		dst->truncate();
		delete[] buff;
		delete f;
		delete dst;
	}

	{
		char filepath[] = "/fajl1.dat";
		File* f = FS::open(filepath, 'r');
		char filepath1[] = "/test4.dat";
		char* buff = new char[f->getFileSize() / 4];
		File* dst = FS::open(filepath1, 'w');
		f->seek(f->getFileSize() * 3 / 4);
		f->read(f->getFileSize() / 4, buff);
		dst->write(f->getFileSize() / 4, buff);
		delete[] buff; 
		delete f;
		delete dst;
	}

	{
		char filepath1[] = "/test1.dat";
		char filepath2[] = "/test2.dat";
		char filepath3[] = "/test3.dat";
		char filepath4[] = "/test4.dat";

		File* f1 = FS::open(filepath1, 'r');
		File* f2 = FS::open(filepath2, 'r');
		File* f3 = FS::open(filepath3, 'r');
		File* f4 = FS::open(filepath4, 'r');
		
		ofstream fout("izlaz.jpg", ios::out | ios::binary);
		{
			char* buff = new char[f1->getFileSize()];
			f1->read(f1->getFileSize(), buff);
			fout.write(buff, f1->getFileSize());
			delete f1;
			delete[] buff;
		}
		{
			char* buff = new char[f2->getFileSize()];
			f2->read(f2->getFileSize(), buff);
			fout.write(buff, f2->getFileSize());
			delete f2;
			delete[] buff;
		}
		{
			char* buff = new char[f3->getFileSize()];
			f3->read(f3->getFileSize(), buff);
			fout.write(buff, f3->getFileSize());
			delete f3;
			delete[] buff;
		}
		{
			char* buff = new char[f4->getFileSize()];
			f4->read(f4->getFileSize(), buff);
			fout.write(buff, f4->getFileSize());
			delete f4;
			delete[] buff;
		}

		fout.close();
	}



	FS::unmount();
	return 0;
}