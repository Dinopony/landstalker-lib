#pragma once

#include <vector>
#include <cstdint>

std::vector<uint8_t> decode_lz77(const uint8_t*& it);
