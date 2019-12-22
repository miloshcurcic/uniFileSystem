class FCB;

class FileHandle {
private:
	FCB* fcb;
	unsigned int num_readers = 0;
	bool write_access = false;

	friend class KernelFS;
	friend class KernelFile;
};