#include <stdint.h>
#include <stdio.h>
#include <string.h>

void xtea_encrypt(uint32_t v[2], const uint32_t key[4], uint32_t num_rounds) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
    for (uint32_t i = 0; i < num_rounds; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
    }
    v[0] = v0; v[1] = v1;
}

void xtea_decrypt(uint32_t v[2], const uint32_t key[4], uint32_t num_rounds) {
    uint32_t v0 = v[0], v1 = v[1], delta = 0x9E3779B9;
    uint32_t sum = delta * num_rounds;
    for (uint32_t i = 0; i < num_rounds; i++) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }
    v[0] = v0; v[1] = v1;
}

int main() {
    uint32_t key[4] = {0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210};
    uint32_t v[2] = {0, 0};
    uint32_t num_rounds = 32;  // Standard number of rounds for XTEA
    
    // Message to encrypt (8 bytes / 64 bits)
    char message[] = "HelloWld";
    
    // Copy message to v array
    memcpy(v, message, 8);
    
    printf("Original message: %.8s\n", (char*)v);
    
    // Encrypt
    xtea_encrypt(v, key, num_rounds);
    printf("Encrypted: 0x%08X 0x%08X\n", v[0], v[1]);
    
    // Decrypt
    xtea_decrypt(v, key, num_rounds);
    printf("Decrypted message: %.8s\n", (char*)v);
    
    return 0;
}

