#pragma once

#include <string>
#include <vector>

#include <cstdint>

/*
 * Binary executable for Commodore 64.
 * The first two bytes indicate the store address.
 * The KERNAL will load the prg at that address.
 */
class PRG final {
public:
	std::string path;
	std::vector<uint8_t> data;

	static constexpr unsigned min_prg_size = 2;
	// max size according to https://www.c64-wiki.com/wiki/Commodore_1541:
	static constexpr unsigned max_prg_size = 2 + 202 * 256; // +2 for start address

	PRG() : path(""), data() {}

	bool is_valid() const noexcept { return data.size() >= min_prg_size && data.size() <= max_prg_size; }

	void load(const std::string&);
	void store(std::vector<uint8_t>&);

	uint16_t load_address() const { return data.at(0) | (data.at(1) << 8); }
};
