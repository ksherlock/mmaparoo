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

class bank_memory {
public:
	bank_memory();
	~bank_memory();

	uint8_t *base = nullptr;
	uint8_t *zp = nullptr;
};

#endif