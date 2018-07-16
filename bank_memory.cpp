#include "bank_memory.h"

#if defined(__APPLE__)

#include <mach/mach.h>
#include <mach/vm_map.h>

constexpr size_t alloc_size = 258 * 0x010000;

bank_memory::bank_memory() {
	kern_return_t ok;

	vm_address_t address = 0;
	vm_address_t address2 = 0;
	vm_address_t address3 = 0;
	vm_prot_t cur_prot = VM_PROT_NONE;
	vm_prot_t max_prot = VM_PROT_NONE;


	ok = vm_allocate(mach_task_self(), &address, alloc_size, VM_FLAGS_ANYWHERE);
	if (ok != KERN_SUCCESS) throw ok;
	base = (uint8_t *)address;

	ok = vm_allocate(mach_task_self(), &address, 256 * 0x010000, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE);
	if (ok != KERN_SUCCESS) throw ok;

	address2 = address + 256 * 0x010000;
	ok = vm_remap(mach_task_self(), &address2, 0x010000, 0, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE,
	mach_task_self(), address, 0, &cur_prot, &max_prot, VM_INHERIT_NONE);
	if (ok != KERN_SUCCESS) throw ok;


	address3 = address + 257 * 0x010000;
	ok = vm_remap(mach_task_self(), &address3, 0x010000, 0, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE,
	mach_task_self(), address, 0, &cur_prot, &max_prot, VM_INHERIT_NONE);
	if (ok != KERN_SUCCESS) throw ok;

	zp = (uint8_t *)address2;
}

bank_memory::~bank_memory() {
	if (base)
		vm_deallocate(mach_task_self(), (vm_address_t)base, alloc_size);
}
#elif defined(__WIN32__)

#include <Windows.h>

bank_memory::bank_memory() {

	void *address;
	void *address2;
	void *address3;
	void *address4;

	handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256 * 0x010000, NULL);
	if (handle == INVALID_HANDLE_VALUE) throw GetLastError();

	/* find a suitable location */
	address = VirtualAlloc(NULL, alloc_size, MEM_RESERVE, PAGE_READWRITE);
	if (!address) throw GetLastError();
	VirtualFree(address, 0, MEM_RELEASE);

	address2 = MapViewOfFileEx(handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 256 * 0x010000, address);
	if (address2 == NULL) throw GetLastError();
	if (address2 != address) throw 0;

	base = address2;

	address = (uint8_t *)address + 256 * 0x010000;
	address3 = MapViewOfFileEx(h, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1 * 0x010000, address);
	if (address2 == NULL) throw GetLastError();
	if (address2 != address) throw 0;
	
	zp = address3;


	address = (uint8_t *)address + 256 * 0x010000;
	address4 = MapViewOfFileEx(h, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1 * 0x010000, address);
	if (address2 == NULL) throw GetLastError();
	if (address2 != address) throw 0;
}

bank_memory::~bank_memory() {
	if (base) {
		UnmapViewOfFile(base);
		UnmapViewOfFile(base + 256 * 0x010000);
		UnmapViewOfFile(base + 257 * 0x010000);
	}
	if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
}


#else

#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

#include <string>

bank_memory::bank_memory() {

	int fd = -1;
	void *address = nullptr;
	void *address2 = nullptr;
	void *address3 = nullptr;
	size_t offset;

	std::string s("bank_memory-");
	s += std::to_string(getpid());


#ifdef __linux__
	fd = memfd_create(s.c_str(), 0);
#endif
	if (fd < 0) {
		fd = shm_open(s.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
		if (fd < 0) throw errno;

		shm_unlink(s.c_str());
	}
	if (ftruncate(fd, 256 * 0x10000) < 0) {
		close(fd);
		throw errno;
	}
	address = mmap(0, alloc_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (address == MAP_FAILED) { close(fd); throw errno; }

	base = mmap(address, 256 * 0x010000, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
	if (base == MAP_FAILED) {
		close(fd);
		throw errno;
	}
	offset = 256 * 0x010000;
	address2 = mmap(address + offset, 0x10000, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
	if (address2 == MAP_FAILED) { close(fd); throw errno; }


	offset += 0x010000;
	address3 = mmap(address + offset, 0x10000, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
	if (address3 == MAP_FAILED) { close(fd); throw errno; }

	zp = address2;
	close(fd);
}

bank_memory::~bank_memory() {
	if (base && base != MAP_FAILED)
		munmap(base, alloc_size);
}


#endif


