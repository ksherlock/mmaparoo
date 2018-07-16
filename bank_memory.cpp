#include "bank_memory.h"

constexpr size_t alloc_size = 258 * 0x010000;


bank_memory::bank_memory() {
	std::error_code ec;
	init(ec);
	if (ec) throw std::system_error(ec, "bank_memory");
}

bank_memory::bank_memory(std::error_code &ec) {
	init(ec);
}

#if defined(__APPLE__)

#include <mach/mach.h>
#include <mach/vm_map.h>
#include "kern_error.h"


void bank_memory::init(std::error_code &ec) {
	kern_return_t ok;

	ec.clear();

	vm_address_t address = 0;
	vm_address_t address2 = 0;
	vm_address_t address3 = 0;
	vm_prot_t cur_prot = VM_PROT_NONE;
	vm_prot_t max_prot = VM_PROT_NONE;


	ok = vm_allocate(mach_task_self(), &address, alloc_size,
		VM_FLAGS_ANYWHERE);
	if (ok != KERN_SUCCESS) { goto fail; }

	ok = vm_allocate(mach_task_self(), &address, 256 * 0x010000,
		VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE);

	if (ok != KERN_SUCCESS) { goto fail; }


	address2 = address + 256 * 0x010000;
	ok = vm_remap(mach_task_self(), &address2, 0x010000, 0,
		VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, mach_task_self(), address, 0,
		&cur_prot, &max_prot, VM_INHERIT_NONE);

	if (ok != KERN_SUCCESS) { goto fail; }



	address3 = address + 257 * 0x010000;
	ok = vm_remap(mach_task_self(), &address3, 0x010000, 0,
		VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, mach_task_self(), address, 0,
		&cur_prot, &max_prot, VM_INHERIT_NONE);

	if (ok != KERN_SUCCESS) { goto fail; }


	base = (uint8_t *)address;
	zp = (uint8_t *)address2;
	return;

fail:
	ec = make_kern_error_code(ok);
	if (address) vm_deallocate(mach_task_self(), address, alloc_size);
	return;
}

bank_memory::~bank_memory() {
	if (base)
		vm_deallocate(mach_task_self(), (vm_address_t)base, alloc_size);
}

void bank_memory::reset(void) {

	if (base)
		vm_deallocate(mach_task_self(), (vm_address_t)base, alloc_size);
	base = nullptr;
	zp = nullptr;
}


#elif defined(__WIN32__)

#include <Windows.h>

/* n.b. - gcc libc++ doesn't support win32 errors. */

inline std::error_code make_win32_error_code(void) {
  return std::error_code(GetLastError(), system_category());
}

void bank_memory::init(std::error_code &ec) {

	void *address = nullptr;
	void *address2 = nullptr;
	void *address3 = nullptr;
	void *address4 = nullptr;

	ec.clear();

	handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
		0, 256 * 0x010000, NULL);
	if (handle == INVALID_HANDLE_VALUE) { goto fail; }

	/* find a suitable location */
	address = VirtualAlloc(NULL, alloc_size, MEM_RESERVE, PAGE_READWRITE);
	if (!address) { goto fail; }
	VirtualFree(address, 0, MEM_RELEASE);

	address2 = MapViewOfFileEx(handle, FILE_MAP_READ | FILE_MAP_WRITE,
		0, 0, 256 * 0x010000, address);
	if (!address2) { goto fail; }
	//if (address2 != address) throw 0;


	address = (uint8_t *)address + 256 * 0x010000;
	address3 = MapViewOfFileEx(h, FILE_MAP_READ | FILE_MAP_WRITE,
		0, 0, 1 * 0x010000, address);
	if (!address2) { goto fail; }
	//if (address2 != address) throw 0;
	


	address = (uint8_t *)address + 256 * 0x010000;
	address4 = MapViewOfFileEx(h, FILE_MAP_READ | FILE_MAP_WRITE,
		0, 0, 1 * 0x010000, address);
	if (!address2) { goto fail; }
	//if (address2 != address) throw 0;

	base = address2;
	zp = address3;
	return;

fail:
	ec = make_win32_error_code();
	if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
	if (address2) UnmapViewOfFile(address2);
	if (address3) UnmapViewOfFile(address3);
	if (address4) UnmapViewOfFile(address4);
}

bank_memory::~bank_memory() {
	if (base) {
		UnmapViewOfFile(base);
		UnmapViewOfFile(base + 256 * 0x010000);
		UnmapViewOfFile(base + 257 * 0x010000);
	}
	if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
}

void bank_memory::reset(void) {

	if (base) {
		UnmapViewOfFile(base);
		UnmapViewOfFile(base + 256 * 0x010000);
		UnmapViewOfFile(base + 257 * 0x010000);
	}
	if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);

	base = nullptr;
	zp = nullptr;
	handle = INVALID_HANDLE_VALUE;
}


#else

#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

#include <string>

inline std::error_code make_errno_error_code(void) {
  return std::error_code(errno, generic_category());
}

void bank_memory::init(std::error_code &ec) {

	int fd = -1;
	void *address = nullptr;
	void *address2 = nullptr;
	void *address3 = nullptr;
	void *address4 = nullptr;
	size_t offset;


	ec.clear();

	std::string s("bank_memory-");
	s += std::to_string(getpid());


#ifdef __linux__
	fd = memfd_create(s.c_str(), 0);
#endif
	if (fd < 0) {
		fd = shm_open(s.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
		if (fd < 0) { goto fail; }

		shm_unlink(s.c_str());
	}
	if (ftruncate(fd, 256 * 0x10000) < 0) { goto fail; }

	address = mmap(0, alloc_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE,
		-1, 0);
	if (address == MAP_FAILED) { goto fail; }

	address2 = mmap(address, 256 * 0x010000, PROT_READ | PROT_WRITE,
		MAP_FIXED | MAP_SHARED, fd, 0);
	if (address2 == MAP_FAILED) { goto fail; }

	offset = 256 * 0x010000;
	address3 = mmap(address + offset, 0x10000, PROT_READ | PROT_WRITE,
		MAP_FIXED | MAP_SHARED, fd, 0);
	if (address3 == MAP_FAILED) { goto fail; }


	offset += 0x010000;
	address4 = mmap(address + offset, 0x10000, PROT_READ | PROT_WRITE,
		MAP_FIXED | MAP_SHARED, fd, 0);
	if (address4 == MAP_FAILED) { goto fail; }

	base = address2;
	zp = address3;
	close(fd);
	return;

fail:
	ec = make_errno_error_code();
	if (fd >= 0) close(fd);
	if (address && address != MAP_FAILED) munmap(address, alloc_size);
}

bank_memory::~bank_memory() {
	if (base)
		munmap(base, alloc_size);
}

void bank_memory::reset(void) {

	if (base)
		munmap(base, alloc_size);
	base = nullptr;
	zp = nullptr;
}


#endif


bank_memory::bank_memory(bank_memory &&rhs) {
	std::swap(base, rhs.base);
	std::swap(zp, rhs.zp);
	#ifdef __WIN32__
	std::swap(handle, rhs.handle);
	#endif
}

bank_memory& bank_memory::operator=(bank_memory &&rhs) {
	if (std::addressof(rhs) != this) {
		reset();

		std::swap(base, rhs.base);
		std::swap(zp, rhs.zp);
		#ifdef __WIN32__
		std::swap(handle, rhs.handle);
		#endif
	}
	return *this;
}