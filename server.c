#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 5000
#define BACKLOG 5
#define BUF_SZ 8192

// Rastgele sayı üretici
int rnd(int a, int b) { 
    return a + rand() % (b - a + 1); 
}

// --- Hata Enjeksiyon Yöntemleri ---
void bit_flip(char *data) {
    size_t n = strlen(data);
    if (n == 0) return;
    size_t idx = rnd(0, n - 1);
    unsigned char c = data[idx];
    int bit = rnd(0, 7);
    c ^= (1 << bit);
    data[idx] = c;
}

void char_sub(char *data) {
    size_t n = strlen(data);
    if (n == 0) return;
    size_t idx = rnd(0, n - 1);
    data[idx] = (char)rnd(33, 126);
}

void char_delete(char *data) {
    size_t n = strlen(data);
    if (n <= 1) return;
    size_t idx = rnd(0, n - 1);
    memmove(data + idx, data + idx + 1, n - idx);
}

void char_insert(char *data) {
    size_t n = strlen(data);
    if (n + 1 >= BUF_SZ) return;
    size_t idx = rnd(0, n);
    memmove(data + idx + 1, data + idx, n - idx + 1);
    data[idx] = (char)rnd(33, 126);
}

void char_swap(char *data) {
    size_t n = strlen(data);
    if (n < 2) return;
    size_t idx = rnd(0, n - 2);
    char t = data[idx];
    data[idx] = data[idx + 1];
    data[idx + 1] = t;
}

void multi_bit_flip(char *data) {
    int flips = rnd(2, 5);
    for (int i = 0; i < flips; i++) {
        bit_flip(data);
    }
}

void burst_error(char *data) {
    size_t n = strlen(data);
    if (n == 0) return;
    size_t start = rnd(0, n - 1);
    int len = rnd(3, 8);
    for (int i = 0; i < len && (start + i) < n; i++) {
        data[start + i] = (char)rnd(33, 126);
    }
}

void corrupt_data(char *data) {
    int method = rnd(1, 7);
    switch (method) {
        case 1: bit_flip(data); printf("[SERVER] Hata: bit_flip\n"); break;
        case 2: char_sub(data); printf("[SERVER] Hata: char_sub\n"); break;
        case 3: char_delete(data); printf("[SERVER] Hata: char_delete\n"); break;
        case 4: char_insert(data); printf("[SERVER] Hata: char_insert\n"); break;
        case 5: char_swap(data); printf("[SERVER] Hata: char_swap\n"); break;
        case 6: multi_bit_flip(data); printf("[SERVER] Hata: multi_bit_flip\n"); break;
        case 7: burst_error(data); printf("[SERVER] Hata: burst_error\n"); break;
    }
}

int main() {
    srand(time(NULL));
    WSADATA wsa; 
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listenfd, BACKLOG);

    printf("[SERVER] 127.0.0.1:%d portunda dinleniyor...\n", PORT);

    // Önce Sender'ı (Client 1) kabul et
    printf("[SERVER] Sender bekleniyor...\n");
    SOCKET s1 = accept(listenfd, NULL, NULL);
    printf("[SERVER] Sender baglandi.\n");

    // Sonra Receiver'ı (Client 2) kabul et
    printf("[SERVER] Receiver bekleniyor...\n");
    SOCKET s2 = accept(listenfd, NULL, NULL);
    printf("[SERVER] Receiver baglandi.\n");

    char buf[BUF_SZ];
    int r = recv(s1, buf, BUF_SZ - 1, 0);
    
    if (r > 0) {
        buf[r] = '\0';
        printf("[SERVER] Paket alindi: %s\n", buf);

        // Paketi parse et
        char data[BUF_SZ], method[128], control[1024];
        char *p1 = strchr(buf, '|');
        char *p2 = strchr(p1 + 1, '|');

        if (p1 && p2) {
            strncpy(data, buf, p1 - buf);
            data[p1 - buf] = 0;
            
            strncpy(method, p1 + 1, p2 - (p1 + 1));
            method[p2 - (p1 + 1)] = 0;
            
            strcpy(control, p2 + 1);

            printf("[SERVER] Orijinal veri: %s\n", data);
            
            // Veriyi boz (hata enjekte et)
            corrupt_data(data);
            
            printf("[SERVER] Bozulmus veri: %s\n", data);

            // Paketi tekrar birleştir ve gönder
            char out[BUF_SZ];
            sprintf(out, "%s|%s|%s", data, method, control);
            send(s2, out, strlen(out), 0);
            printf("[SERVER] Bozuk paket receiver'a gonderildi.\n");
        }
    }

    closesocket(s1); 
    closesocket(s2); 
    closesocket(listenfd);
    WSACleanup();
    
    printf("[SERVER] Kapatiliyor...\n");
    return 0;
}