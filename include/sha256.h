#ifndef SHA256_H
#define SHA256_H

#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iomanip>

// INDUSTRY STANDARD SHA-256 IMPLEMENTATION
// This class provides cryptographic hashing capabilities to the server.
// It is used to generate tamper-proof session IDs and audit logs.

class SHA256 {
private:
    typedef unsigned char uint8;
    typedef unsigned int uint32;
    typedef unsigned long long uint64;

    const static uint32 k[64];
    static const uint32 initial_h[8];

    // Rotates bits to the right (Circular Shift)
    template <typename T>
    static T rightRotate(T x, uint32 n) {
        return (x >> n) | (x << (32 - n));
    }

public:
    // Main hashing function: Converts string input -> 64-char Hex Hash
    static std::string hash(const std::string& input) {
        uint32 h[8];
        memcpy(h, initial_h, 8 * sizeof(uint32));

        std::vector<uint8> msg(input.begin(), input.end());

        // 1. Padding Logic (Standard SHA-256 Block Formatting)
        uint64 originalBitLen = (uint64)msg.size() * 8;
        msg.push_back(0x80);
        while ((msg.size() + 8) % 64 != 0) {
            msg.push_back(0x00);
        }

        // 2. Append Length
        for (int i = 7; i >= 0; --i) {
            // Use static_cast for C++ compliance instead of C-style cast
            msg.push_back(static_cast<uint8>(originalBitLen >> (i * 8)));
        }

        // 3. Process 512-bit chunks
        for (size_t i = 0; i < msg.size(); i += 64) {
            uint32 w[64];
            for (int t = 0; t < 16; ++t) {
                w[t] = (msg[i + t * 4] << 24) | (msg[i + t * 4 + 1] << 16) |
                       (msg[i + t * 4 + 2] << 8) | (msg[i + t * 4 + 3]);
            }
            for (int t = 16; t < 64; ++t) {
                uint32 s0 = rightRotate(w[t - 15], 7) ^ rightRotate(w[t - 15], 18) ^ (w[t - 15] >> 3);
                uint32 s1 = rightRotate(w[t - 2], 17) ^ rightRotate(w[t - 2], 19) ^ (w[t - 2] >> 10);
                w[t] = w[t - 16] + s0 + w[t - 7] + s1;
            }

            uint32 a = h[0], b = h[1], c = h[2], d = h[3];
            uint32 e = h[4], f = h[5], g = h[6], h_val = h[7];

            for (int t = 0; t < 64; ++t) {
                uint32 S1 = rightRotate(e, 6) ^ rightRotate(e, 11) ^ rightRotate(e, 25);
                uint32 ch = (e & f) ^ ((~e) & g);
                uint32 temp1 = h_val + S1 + ch + k[t] + w[t];
                uint32 S0 = rightRotate(a, 2) ^ rightRotate(a, 13) ^ rightRotate(a, 22);
                uint32 maj = (a & b) ^ (a & c) ^ (b & c);
                uint32 temp2 = S0 + maj;

                h_val = g; g = f; f = e; e = d + temp1;
                d = c; c = b; b = a; a = temp1 + temp2;
            }

            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
            h[4] += e; h[5] += f; h[6] += g; h[7] += h_val;
        }

        std::stringstream ss;
        // Use Range-based for loop (Modern C++ Standard)
        for (const auto& val : h) {
            ss << std::hex << std::setw(8) << std::setfill('0') << val;
        }
        return ss.str();
    }
};

// SHA-256 Constants (Required for the algorithm to work)
const SHA256::uint32 SHA256::k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

const SHA256::uint32 SHA256::initial_h[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

#endif