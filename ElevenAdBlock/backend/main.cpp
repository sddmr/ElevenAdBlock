#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <cstring>
#include "dns.h"

std::unordered_set<std::string> blocklist;
const char* UPSTREAM_DNS_SERVER = "8.8.8.8";
const int DNS_PORT = 53;

void loadBlocklist(const std::string& filename = "hosts.txt") {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "HATA: " << filename << " dosyasi acilamadi." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t first_space = line.find_first_of(" \t");
        if (first_space != std::string::npos) {
            size_t second_space = line.find_first_not_of(" \t", first_space);
            if (second_space != std::string::npos) {
                std::string domain = line.substr(second_space);
                if (!domain.empty() && domain.back() == '\r') {
                    domain.pop_back();
                }
                if (!domain.empty()) {
                    blocklist.insert(domain);
                }
            }
        }
    }
    file.close();
    std::cout << "[BILGI] " << blocklist.size() << " adet alan adi engelleme listesine eklendi." << std::endl;
}

std::string ReadName(unsigned char* reader, unsigned char* buffer, int* count) {
    std::string name;
    unsigned int p = 0, jumped = 0, offset;
    *count = 1;

    while (*reader != 0) {
        if (*reader >= 192) {
            offset = (*reader) * 256 + *(reader + 1) - 49152;
            reader = buffer + offset - 1;
            jumped = 1;
        } else {
            name += (char)*reader;
        }

        reader = reader + 1;

        if (jumped == 0) {
            *count = *count + 1;
        }
    }

    if (jumped == 1) {
        *count = *count + 1;
    }

    int len = name.length();
    for (int i = 0; i < len; i++) {
        p = name[i];
        for (int j = 0; j < (int)p; j++) {
            name[i] = name[i + 1];
            i++;
        }
        name[i] = '.';
    }
    name.pop_back();
    return name;
}


int main() {

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup hatasi." << std::endl;
        return 1;
    }
#endif

    int s;
    struct sockaddr_in server_addr, cli_addr;
    unsigned char buf[65536];
    socklen_t addr_len = sizeof(cli_addr);
    int port = DNS_PORT;

#ifdef _WIN32
    s = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        std::cerr << "Soket olusturma hatasi: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
#else
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        std::cerr << "Soket olusturma hatasi." << std::endl;
        return 1;
    }
#endif

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Port " << port << " baglanma hatasi. (Yonetici olarak calistirdiniz mi?)" << std::endl;
#ifdef _WIN32
        closesocket(s);
        WSACleanup();
#else
        close(s);
#endif
        return 1;
    }

    std::cout << "[BILGI] DNS Sunucusu " << port << " portunda basladi..." << std::endl;

    loadBlocklist("backend/hosts.txt");
    struct DNS_HEADER* dns = NULL;
    int reader_stop_pos;

    while (1) {
        int n = recvfrom(s, (char*)buf, 65536, 0, (struct sockaddr*)&cli_addr, &addr_len);
        if (n < 0) {
            std::cerr << "recvfrom hatasi." << std::endl;
            continue;
        }

        dns = (struct DNS_HEADER*)buf;
        unsigned char* qname_ptr = (unsigned char*)&buf[sizeof(struct DNS_HEADER)];

        std::string domain_name = ReadName(qname_ptr, buf, &reader_stop_pos);

        if (blocklist.count(domain_name))
        {
            std::cout << "[ENGELENDI] " << domain_name << std::endl;

            dns->qr = 1;
            dns->ra = 1;
            dns->ancount = htons(1);
            dns->nscount = 0;
            dns->arcount = 0;

            int answer_pos = reader_stop_pos + sizeof(QUESTION);

            unsigned short name_pointer = htons(0xc00c);
            memcpy(&buf[answer_pos], &name_pointer, 2);
            answer_pos += 2;

            struct R_DATA r_data;
            r_data.type = htons(1);
            r_data._class = htons(1);
            r_data.ttl = htonl(60);
            r_data.data_len = htons(4);
            memcpy(&buf[answer_pos], &r_data, sizeof(struct R_DATA));
            answer_pos += sizeof(struct R_DATA);

            unsigned long blocked_ip = 0;
            memcpy(&buf[answer_pos], &blocked_ip, 4);
            answer_pos += 4;

            sendto(s, (char*)buf, answer_pos, 0, (struct sockaddr*)&cli_addr, addr_len);
        }
        else
        {
            std::cout << "[IZIN VERILDI] " << domain_name << std::endl;

            int forward_sock;
            struct sockaddr_in upstream_addr;

            upstream_addr.sin_family = AF_INET;
            upstream_addr.sin_port = htons(DNS_PORT);
            upstream_addr.sin_addr.s_addr = inet_addr(UPSTREAM_DNS_SERVER);

#ifdef _WIN32
            forward_sock = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (forward_sock == INVALID_SOCKET) {
                std::cerr << "Iletme soketi olusturulamadi." << std::endl;
                continue;
            }
#else
            forward_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (forward_sock < 0) {
                std::cerr << "Iletme soketi olusturulamadi." << std::endl;
                continue;
            }
#endif

            sendto(forward_sock, (char*)buf, n, 0, (struct sockaddr*)&upstream_addr, sizeof(upstream_addr));

            unsigned char forward_buf[65536];
            socklen_t upstream_addr_len = sizeof(upstream_addr);
            int forward_n = recvfrom(forward_sock, (char*)forward_buf, 65536, 0, (struct sockaddr*)&upstream_addr, &upstream_addr_len);

            if (forward_n > 0) {
                sendto(s, (char*)forward_buf, forward_n, 0, (struct sockaddr*)&cli_addr, addr_len);
            }

#ifdef _WIN32
            closesocket(forward_sock);
#else
            close(forward_sock);
#endif
        }
    }

#ifdef _WIN32
    closesocket(s);
    WSACleanup();
#else
    close(s);
#endif

    return 0;
}