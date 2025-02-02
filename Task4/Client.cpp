#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

using namespace std;


long long mod_exp(long long base, long long exp, long long mod) {
    long long result = 1;
    while (exp > 0) {
        if (exp % 2 == 1) result = (result * base) % mod;
        base = (base * base) % mod;
        exp /= 2;
    }
    return result;
}


long long diffie_hellman(int peer_socket, bool is_initiator) {
    long long p = 23, g = 5;
    srand(time(0));
    long long private_key = rand() % (p - 1) + 1;
    long long public_key = mod_exp(g, private_key, p);
    long long peer_public_key;

    if (is_initiator) {
        send(peer_socket, &public_key, sizeof(public_key), 0);
        recv(peer_socket, &peer_public_key, sizeof(peer_public_key), 0);
    } else {
        recv(peer_socket, &peer_public_key, sizeof(peer_public_key), 0);
        send(peer_socket, &public_key, sizeof(public_key), 0);
    }

    return mod_exp(peer_public_key, private_key, p);
}

void derive_aes_key(long long shared_secret, unsigned char *aes_key) {
    srand(shared_secret);
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
        aes_key[i] = rand() % 256;
    }
}

void aes_encrypt(const unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext, int &ciphertext_len) {
    AES_KEY encryptKey;
    AES_set_encrypt_key(key, 128, &encryptKey);

    int padding_len = AES_BLOCK_SIZE - (plaintext_len % AES_BLOCK_SIZE);
    unsigned char padded_plaintext[plaintext_len + padding_len];
    memcpy(padded_plaintext, plaintext, plaintext_len);
    memset(padded_plaintext + plaintext_len, padding_len, padding_len); // PKCS7 padding

    AES_cbc_encrypt(padded_plaintext, ciphertext, plaintext_len + padding_len, &encryptKey, iv, AES_ENCRYPT);
    ciphertext_len = plaintext_len + padding_len;
}

// AES Decryption (PKCS7 Unpadding)
void aes_decrypt(const unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext, int &plaintext_len) {
    AES_KEY decryptKey;
    AES_set_decrypt_key(key, 128, &decryptKey);

    unsigned char decrypted_data[ciphertext_len];
    AES_cbc_encrypt(ciphertext, decrypted_data, ciphertext_len, &decryptKey, iv, AES_DECRYPT);

   
    int padding_len = decrypted_data[ciphertext_len - 1];
    plaintext_len = ciphertext_len - padding_len;
    memcpy(plaintext, decrypted_data, plaintext_len);
    plaintext[plaintext_len] = '\0';
}

void receive_messages(int peer_socket, unsigned char *aes_key, unsigned char *iv) {
    char buffer[BUFFER_SIZE];
    unsigned char decrypted[BUFFER_SIZE];
    int plaintext_len;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(peer_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            cout << "[Peer Disconnected]\n";
            break;
        }

        aes_decrypt((unsigned char *)buffer, bytes_received, aes_key, iv, decrypted, plaintext_len);
        
        cout << "[Cipher]: ";
        for (int i = 0; i < bytes_received; i++) printf("%02X", (unsigned char)buffer[i]);
        cout << "\n[Plain]: " << decrypted << endl;
    }
    close(peer_socket);
}

// Handle P2P communication
void handle_peer(int peer_socket, bool is_initiator) {
    cout << "Peer connected! Performing Diffie-Hellman Key Exchange...\n";

    long long shared_secret = diffie_hellman(peer_socket, is_initiator);
    unsigned char aes_key[AES_BLOCK_SIZE];
    derive_aes_key(shared_secret, aes_key);

    unsigned char iv[AES_BLOCK_SIZE];
    if (is_initiator) {
        RAND_bytes(iv, AES_BLOCK_SIZE);
        send(peer_socket, iv, AES_BLOCK_SIZE, 0);
    } else {
        recv(peer_socket, iv, AES_BLOCK_SIZE, 0);
    }

    cout << "Secure communication established. You can now chat.\n";
    thread receive_thread(receive_messages, peer_socket, aes_key, iv);

    string message;
    while (true) {
        getline(cin, message);

        unsigned char ciphertext[BUFFER_SIZE];
        int ciphertext_len;
        aes_encrypt((unsigned char *)message.c_str(), message.length(), aes_key, iv, ciphertext, ciphertext_len);

        cout << "[Cipher]: ";
        for (int i = 0; i < ciphertext_len; i++) printf("%02X", ciphertext[i]);
        cout << endl;

        send(peer_socket, ciphertext, ciphertext_len, 0);
        if (message == "EOM") break;
    }

    close(peer_socket);
    receive_thread.join();
}

// Listen for incoming P2P connections
void listen_for_clients(int listen_socket) {
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int peer_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &addr_size);
        thread peer_thread(handle_peer, peer_socket, false);
        peer_thread.detach();
    }
}

// Initiate P2P chat
void start_p2p_chat(int peer_port) {
    int peer_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in peer_addr = {AF_INET, htons(peer_port)};
    inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr);

    if (connect(peer_socket, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) == -1) return;

    cout << "Connected to peer at 127.0.0.1:" << peer_port << "\n";
    handle_peer(peer_socket, true);
}

int main() {
    string username;
    cout << "Enter username: ";
    cin >> username;

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress = {AF_INET, htons(SERVER_PORT)};
    inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) return 1;

    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in listen_addr = {AF_INET, htons(0), INADDR_ANY};

    bind(listen_socket, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
    listen(listen_socket, 5);

    struct sockaddr_in bound_addr;
    socklen_t bound_len = sizeof(bound_addr);
    getsockname(listen_socket, (struct sockaddr*)&bound_addr, &bound_len);
    int client_port = ntohs(bound_addr.sin_port);

    cout << "Listening for P2P connections on port: " << client_port << endl;

    string user_info = username + ":" + to_string(client_port);
    send(clientSocket, user_info.c_str(), user_info.length(), 0);

    thread listener_thread(listen_for_clients, listen_socket);
    listener_thread.detach();

    while (true) {
        cout << "Enter command (connect/list/exit): ";
        string command;
        cin >> command;

        if (command == "exit") break;

        if (command == "connect") {
            int peer_port;
            cout << "Enter Peer Port: ";
            cin >> peer_port;
            start_p2p_chat(peer_port);
            continue;
        }
    }

    close(clientSocket);
    close(listen_socket);
    return 0;
}
