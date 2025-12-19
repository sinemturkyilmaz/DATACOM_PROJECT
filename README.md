# DATACOM_PROJECT

Bu proje, veri iletişiminde kullanılan hata tespit yöntemlerini (Parity, 2D Parity, CRC, Hamming Code, Internet Checksum) pratik olarak gösteren bir socket programlama uygulamasıdır.

Proje üç ana bileşenden oluşur:

Client 1 - Veri Gönderici

Server - Ara Düğüm + Veri Bozucu

Client 2 - Alıcı + Hata Kontrolcü

DERLEME İÇİN !!
gcc -o server.exe server.c -lws2_32
gcc -o client1.exe client1.c -lws2_32
gcc -o client2.exe client2.c -lws2_32

server.exe
client1.exe 127.0.0.1 5000 sonrasında metin girip hata kontrol aracını seçin
client2.exe 127.0.0.1 5000 

Client 1 - Desteklenen Hata Tespit Metodları
Parity Bit (Even/Odd Parity)

2D Parity (Matrix Parity)

CRC-16 (Cyclic Redundancy Check)

Hamming Code (7,4 encoding)

Internet Checksum (IP Checksum)

Server - Hata Enjeksiyon Yöntemleri
Bit Flip (Tek bit değiştirme)

Character Substitution (Karakter değiştirme)

Character Deletion (Karakter silme)

Character Insertion (Karakter ekleme)

Character Swapping (Karakter yer değiştirme)

Multiple Bit Flips (Çoklu bit değiştirme)

Burst Error (Patlama hatası - 3-8 karakter bozma)

Client 2 - Hata Kontrolü
Gelen veriyi aynı algoritma ile doğrular

Orijinal ve hesaplanan kontrol bitlerini karşılaştırır

"DATA CORRECT" veya "DATA CORRUPTED" durumunu gösterir
