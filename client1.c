#define _WIN32_WINNT 0x0600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// --- CRC16 ---
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

// --- Internet Checksum ---
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

// --- Parity (Even/Odd) ---
char compute_parity(const uint8_t *data, size_t len, int is_odd) {
    int ones = 0;
    for (size_t i = 0; i < len; i++) {
        for (int k = 0; k < 8; k++) 
            if (data[i] & (1 << k)) ones++;
    }
    int parity_bit = ones % 2;
    
    if (is_odd) {
        // Odd parity: Toplam 1'ler tek sayı olmalı
        return parity_bit ? '0' : '1';
    } else {
        // Even parity: Toplam 1'ler çift sayı olmalı
        return parity_bit ? '1' : '0';
    }
}

// --- 2D Parity ---
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

// --- Hamming (7,4) Code ---
uint8_t hamming_encode_nibble(uint8_t nibble) {
    // Extract data bits (d3 d2 d1 d0)
    int d[4];
    for (int i = 0; i < 4; i++) {
        d[i] = (nibble >> (3 - i)) & 1;
    }
    
    // Calculate parity bits
    int p1 = d[0] ^ d[1] ^ d[3];  // Positions 1,2,4
    int p2 = d[0] ^ d[2] ^ d[3];  // Positions 1,3,4
    int p3 = d[1] ^ d[2] ^ d[3];  // Positions 2,3,4
    
    // Construct 7-bit Hamming code: p1 p2 d0 p3 d1 d2 d3
    uint8_t hamming = (p1 << 6) | (p2 << 5) | (d[0] << 4) |
                      (p3 << 3) | (d[1] << 2) | (d[2] << 1) | d[3];
    
    return hamming;
}

void compute_hamming(const uint8_t *data, size_t len, char *out) {
    char *ptr = out;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        
        // Encode high nibble
        uint8_t high_nibble = (byte >> 4) & 0x0F;
        uint8_t hamming_high = hamming_encode_nibble(high_nibble);
        
        // Encode low nibble
        uint8_t low_nibble = byte & 0x0F;
        uint8_t hamming_low = hamming_encode_nibble(low_nibble);
        
        ptr += sprintf(ptr, "%02X%02X", hamming_high, hamming_low);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) { 
        printf("Kullanim: %s <IP> <Port>\n", argv[0]); 
        return 1; 
    }

    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    char data[1024], method[32], control[4096] = {0};
    
    // Kullanıcıdan veri al
    printf("Gonderilecek metni yazin: ");
    fgets(data, sizeof(data), stdin);
    data[strcspn(data, "\n")] = 0;
    
    // Method seçimi
    printf("Metot secin (PARITY_EVEN, PARITY_ODD, 2DPARITY, CRC16, HAMMING, INETCKSUM): ");
    fgets(method, sizeof(method), stdin);
    method[strcspn(method, "\n")] = 0;

    size_t len = strlen(data);
    
    // Kontrol bilgisi hesapla
    if (strcmp(method, "CRC16") == 0) {
        sprintf(control, "%04X", crc16_ccitt((uint8_t*)data, len));
    }
    else if (strcmp(method, "INETCKSUM") == 0) {
        sprintf(control, "%04X", internet_checksum((uint8_t*)data, len));
    }
    else if (strcmp(method, "PARITY_EVEN") == 0) {
        control[0] = compute_parity((uint8_t*)data, len, 0);
        control[1] = 0;
    }
    else if (strcmp(method, "PARITY_ODD") == 0) {
        control[0] = compute_parity((uint8_t*)data, len, 1);
        control[1] = 0;
    }
    else if (strcmp(method, "2DPARITY") == 0) {
        compute_2d_parity((uint8_t*)data, len, control);
    }
    else if (strcmp(method, "HAMMING") == 0) {
        compute_hamming((uint8_t*)data, len, control);
    }
    else {
        printf("Hata: Geçersiz metot!\n");
        return 1;
    }

    // Socket bağlantısı
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(atoi(argv[2]));
    serv.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) == 0) {
        char packet[8192];
        sprintf(packet, "%s|%s|%s", data, method, control);
        send(sock, packet, strlen(packet), 0);
        printf("[CLIENT1] Paket gonderildi: %s\n", packet);
    } else {
        printf("[CLIENT1] Server'a baglanilamadi!\n");
    }
    
    closesocket(sock); 
    WSACleanup();
    return 0;
}