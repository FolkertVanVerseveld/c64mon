#include "prg.hpp"

#include <cstddef>
#include <cstdio>

#include <fstream>
#include <stdexcept>

void PRG::load(const std::string &path) {
	//printf("prg path: %s\n", path.c_str());

	try {
		std::ifstream in(path, std::ios::binary);
		in.exceptions(std::ifstream::failbit);

		in.seekg(0, std::ios_base::end);
		size_t end = in.tellg();
		in.seekg(0);
		size_t begin = in.tellg();

		size_t size = end - begin;

		//printf("prg size: %zu\n", size);

		this->path = path;
		this->data.resize(size);
		in.read((char*)this->data.data(), size);
	} catch (const std::runtime_error &e) {
		fprintf(stderr, "%s: %s\n", __func__, e.what());
	}
}

void PRG::store(std::vector<uint8_t> &out) {
	if (out.size() >= PRG::max_prg_size)
		throw std::runtime_error("prg too big");

	unsigned size = data.size();

	out.emplace_back(size & 0xff);
	out.emplace_back(size >> 8);

	out.insert(out.end(), data.begin(), data.end());
}
