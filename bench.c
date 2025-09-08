#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define ITER 100000000  // 100 ล้าน

uint8_t lookup_table[256];

void init_lookup() {
    for (int i = 0; i < 256; i++) {
        uint8_t x = i;
        int cnt = 0;
        while (x) { cnt ^= (x & 1); x >>= 1; }
        lookup_table[i] = cnt;
    }
}

uint64_t test_lookup(uint8_t *data, size_t len) {
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += lookup_table[data[i]];
    }
    return sum;
}

uint8_t parity_bitwise(uint8_t x) {
    int cnt = 0;
    while (x) { cnt ^= (x & 1); x >>= 1; }
    return cnt;
}

uint64_t test_bitwise(uint8_t *data, size_t len) {
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += parity_bitwise(data[i]);
    }
    return sum;
}

int main() {
    uint8_t *data = malloc(ITER);
    if (!data) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    for (int i = 0; i < ITER; i++) data[i] = (uint8_t)(i & 0xFF);

    init_lookup();

    clock_t t1 = clock();
    uint64_t s1 = test_lookup(data, ITER);
    clock_t t2 = clock();
    uint64_t s2 = test_bitwise(data, ITER);
    clock_t t3 = clock();

    double time_lookup = (double)(t2 - t1) / CLOCKS_PER_SEC;
    double time_bitwise = (double)(t3 - t2) / CLOCKS_PER_SEC;

    printf("Lookup sum=%lu time=%.6f sec\n", s1, time_lookup);
    printf("Bitwise sum=%lu time=%.6f sec\n", s2, time_bitwise);

    free(data);
    return 0;
}
