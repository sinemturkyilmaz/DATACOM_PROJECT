#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")
#define PORT 5000
#define BUF_SZ 8192

// --- Client 1 ile AYNI fonksiyonlar ---

uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i] << 8);
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    return crc & 0xFFFF;
}

uint16_t internet_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i + 1 < len; i += 2) {
        sum += (data[i] << 8) | data[i + 1];
        if (sum > 0xFFFF) sum = (sum & 0xFFFF) + (sum >> 16);
    }
    if (len % 2) {
        sum += (data[len - 1] << 8);
        if (sum > 0xFFFF) sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

char compute_parity(const uint8_t *data, size_t len, int is_odd) {
    int ones = 0;
    for (size_t i = 0; i < len; i++) {
        for (int k = 0; k < 8; k++) 
            if (data[i] & (1 << k)) ones++;
    }
    int parity_bit = ones % 2;
    
    if (is_odd) {
        return parity_bit ? '0' : '1';
    } else {
        return parity_bit ? '1' : '0';
    }
}

void compute_2d_parity(const uint8_t *data, size_t len, char *out) {
    char row_p[1024] = {0};
    uint8_t col_p = 0;
    
    for (size_t i = 0; i < len; i++) {
        int ones = 0;
        for (int k = 0; k < 8; k++) 
            if (data[i] & (1 << k)) ones++;
        row_p[i] = (ones % 2 == 0) ? '0' : '1';
    }
    row_p[len] = '\0';
    
    for (int bit = 0; bit < 8; bit++) {
        int ones = 0;
        for (size_t i = 0; i < len; i++) 
            if (data[i] & (1 << bit)) ones++;
        if (ones % 2 != 0) col_p |= (1 << bit);
    }
    sprintf(out, "%s|%02X", row_p, col_p);
}

uint8_t hamming_encode_nibble(uint8_t nibble) {
    int d[4];
    for (int i = 0; i < 4; i++) {
        d[i] = (nibble >> (3 - i)) & 1;
    }
    
    int p1 = d[0] ^ d[1] ^ d[3];
    int p2 = d[0] ^ d[2] ^ d[3];
    int p3 = d[1] ^ d[2] ^ d[3];
    
    uint8_t hamming = (p1 << 6) | (p2 << 5) | (d[0] << 4) |
                      (p3 << 3) | (d[1] << 2) | (d[2] << 1) | d[3];
    
    return hamming;
}

void compute_hamming(const uint8_t *data, size_t len, char *out) {
    char *ptr = out;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        
        uint8_t high_nibble = (byte >> 4) & 0x0F;
        uint8_t hamming_high = hamming_encode_nibble(high_nibble);
        
        uint8_t low_nibble = byte & 0x0F;
        uint8_t hamming_low = hamming_encode_nibble(low_nibble);
        
        ptr += sprintf(ptr, "%02X%02X", hamming_high, hamming_low);
    }
    *ptr = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Kullanim: %s <IP> <Port>\n", argv[0]);
        return 1;
    }

    WSADATA wsa; 
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(atoi(argv[2]));
    serv.sin_addr.s_addr = inet_addr(argv[1]);

    printf("[CLIENT2] Server'a baglaniyor...\n");
    
    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) != 0) {
        printf("[CLIENT2] Server'a baglanilamadi!\n");
        return 1;
    }

    printf("[CLIENT2] Server'a baglandi. Veri bekleniyor...\n");

    char buf[BUF_SZ];
    int r = recv(sock, buf, BUF_SZ - 1, 0);
    
    if (r > 0) {
        buf[r] = 0;
        
        char data[BUF_SZ], method[128], incoming[4096];
        char computed[4096] = {0};
        
        // Paketi parse et
        char *p1 = strchr(buf, '|');
        char *p2 = strchr(p1 + 1, '|');
        
        if (!p1 || !p2) {
            printf("[CLIENT2] Hata: Gecersiz paket format!\n");
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        
        strncpy(data, buf, p1 - buf);
        data[p1 - buf] = 0;
        
        strncpy(method, p1 + 1, p2 - (p1 + 1));
        method[p2 - (p1 + 1)] = 0;
        
        strcpy(incoming, p2 + 1);
        
        size_t dlen = strlen(data);
        
        // Kontrol bilgisi hesapla
        if (strcmp(method, "CRC16") == 0) {
            sprintf(computed, "%04X", crc16_ccitt((uint8_t*)data, dlen));
        }
        else if (strcmp(method, "INETCKSUM") == 0) {
            sprintf(computed, "%04X", internet_checksum((uint8_t*)data, dlen));
        }
        else if (strcmp(method, "PARITY_EVEN") == 0) {
            computed[0] = compute_parity((uint8_t*)data, dlen, 0);
            computed[1] = 0;
        }
        else if (strcmp(method, "PARITY_ODD") == 0) {
            computed[0] = compute_parity((uint8_t*)data, dlen, 1);
            computed[1] = 0;
        }
        else if (strcmp(method, "2DPARITY") == 0) {
            compute_2d_parity((uint8_t*)data, dlen, computed);
        }
        else if (strcmp(method, "HAMMING") == 0) {
            compute_hamming((uint8_t*)data, dlen, computed);
        }
        else {
            printf("[CLIENT2] Hata: Bilinmeyen metot: %s\n", method);
        }

        // İSTENEN ÇIKTI FORMATINDA GÖSTER
        printf("\n");
        printf("    Received Data : %s\n", data);
        printf("    Method : %s\n", method);
        printf("    Sent Check Bits : %s\n", incoming);
        printf("    Computed Check Bits : %s\n", computed);
        printf("    Status : %s\n", 
               (strcmp(incoming, computed) == 0) ? "DATA CORRECT" : "DATA CORRUPTED");
        printf("\n");
    } else {
        printf("[CLIENT2] Sunucudan veri alinamadi!\n");
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}