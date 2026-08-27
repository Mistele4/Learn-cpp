#include <iostream>
#include <string>
#include <cctype>

constexpr int num_letters { 26 };

std::string encrypt(const std::string& text, int shift) {

    std::string encrypted;
    shift = shift % num_letters;
    if (shift < 0) {
        shift = num_letters + shift;
    } 

    for (char c: text) {

        if (std::isalpha(static_cast<unsigned char>(c))) {

            const char base { (std::isupper(static_cast<unsigned char>(c))) ? 'A' : 'a' };
            encrypted += static_cast<char>(base + ((c-base + shift) % num_letters));
        } else {
            encrypted += c;
        }
    }
    return encrypted;
}

std::string decrypt(const std::string& text, int shift) {

    return encrypt(text, -(shift % num_letters));

}

int main() {

    std::cout << "Input text to cypher: " << '\n';
    std::string text;
    std::getline(std::cin, text);

    std::cout << "Input shift size: " << '\n';
    std::string shift_str;
    std::getline(std::cin, shift_str);
    int shift { std::stoi(shift_str) };

    std::string encrypted { encrypt(text, shift) };
    std::string decrypted { decrypt(encrypted, shift) };

    std::cout << "Input text:\n" << text << '\n';
    std::cout << "Encrypted:\n" << encrypted << '\n';
    std::cout << "Decrypted:\n" << decrypted << std::endl;

    return 0;
}