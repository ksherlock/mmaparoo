/*
 * allocate 16MB of ram (256 banks of 64k)
 * banks 257 and 258 are mapped to bank 0.
 *
 * Thus, wrapping around bank 256 -> bank 0
 * and wrapping within bank 257/0.
 */

#ifndef bank_memory_h
#define bank_memory_h

#include <cstdint>
#include <system_error>

class bank_memory {
public:
	bank_memory();
	bank_memory(std::error_code &ec);
	bank_memory(bank_memory &&);
	bank_memory(const bank_memory &) = delete;
	//bank_memory();
	~bank_memory();
	void reset();

bank_memory& operator=(bank_memory &&);
bank_memory& operator=(const bank_memory &) = delete;

	uint8_t *base = nullptr;
	uint8_t *zp = nullptr;

private:
	void init(std::error_code &ec);

#ifdef __WIN32__
	void *handle = (void *)-1;
#endif
};

#endif