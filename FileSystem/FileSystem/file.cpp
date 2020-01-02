#include "file.h"
#include "kernel_file.h"

File::~File()
{
	if (myImpl != nullptr) {
		myImpl->close();
	}
}

char File::write(BytesCnt count, char* buffer)
{
	return myImpl->write(count, buffer);
}

BytesCnt File::read(BytesCnt count, char* buffer)
{
	return myImpl->read(count, buffer);
}

char File::seek(BytesCnt pos)
{
	return myImpl->seek(pos);
}

BytesCnt File::filePos()
{
	return myImpl->filePos();
}

char File::eof()
{
	return myImpl->eof();
}

BytesCnt File::getFileSize()
{
	return myImpl->getFileSize();
}

char File::truncate()
{
	return myImpl->truncate();
}

File::File()
{
	myImpl = nullptr;
}
