#include <iostream>
#include <string>

int pow(int a, int b) {
    /// Calculates a ^ b
    int total {1};
    for (int i = 0; i < b; i++) {
        total *= a;
    }
    return total;
}

std::string bin_to_hex(std::string str) {
    int leading { static_cast<int> (str.length()) % 4 };
    if (leading != 0) {
        str = std::string(4-leading, '0') + str;
    }
    int hex_len { static_cast<int> (str.length()) / 4 };

    std::string hexcode  {"0123456789ABCDEF"};
    std::string hex { "" };
    

    for (int i = 0; i < hex_len; i++) {

        int bit_val {0};

        for (int j = 0; j < 4; j++) {

            if (str[4*i+j] == '1') {
                bit_val += pow(2, (3-j));
            }
        }
        hex += hexcode[bit_val];
    }

    return hex;
}

std::string hex_to_bin(std::string str) {
    
    std::string hexcode {"0123456789ABCDEF"};
    std::string binary {""};
    
    for (int i = 0; i < static_cast<int>(str.length()); i++) {
        std::string bits { "" };
        char c { str[i] };
        int idx { static_cast<int> (hexcode.find(c)) };
        for (int j = 1; j <= 4; j++) {
            int digit { pow(2, 4-j) };
            if (idx / digit == 1) {
                bits += '1';
                idx -= digit;
            } else {
                bits += '0';
            }
        }
        binary += bits;
    }
    return binary;
}

int main() {

    std::cout << "Input binary to convert to hex below: " << '\n';
    std::string binary;
    std::getline(std::cin, binary);

    std::string hex;
    hex = bin_to_hex(binary);
    std::string bin { hex_to_bin(hex) };

    std::cout << "Input binary: " << binary << '\n';
    std::cout << "Output hex: " << hex << '\n';
    std::cout << "Output binary: " << bin << std::endl;
}