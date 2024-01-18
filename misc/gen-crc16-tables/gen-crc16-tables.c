#include <stdio.h>
#include <stdint.h>

uint16_t crc16_table[8][256];

int main(void) {
    unsigned int i;
    unsigned int j;
    uint16_t polynomial = 0x8005;
    uint16_t crc = 0;

    for(i=0;i<256;i++) {
        crc = i << 8;
        for(j=0;j<8;j++) {
            crc = (crc << 1) ^ (crc & (1 << 15) ? polynomial : 0);
        }
        crc16_table[0][i] = crc;
    }

    for(i=0;i<256;i++) {
        for(j=1;j<8;j++) {
            crc16_table[j][i] = crc16_table[0][crc16_table[j-1][i] >> 8] ^ (crc16_table[j-1][i] << 8);
        }
    }

    printf("uint16_t const crc16_table[8][256] = {\n");
    for(j=0;j<8;j++) {
        if(j > 0) printf("\n");
        printf("  {\n    ");
        for(i=0;i<256;i++) {
            if(i > 0) {
                if(i % 8 == 0) {
                    printf("\n    ");
                } else {
                    printf("  ");
                }
            }
            printf("0x%04x,",crc16_table[j][i]);
        }
        printf("\n  },");
    }
    printf("\n};\n");

    return 0;
}
