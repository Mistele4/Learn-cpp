#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

std::vector<std::string> split(const std::string& text, char delimiter) {

    std::vector<std::string> tokens {};
    std::string cur_token {};

    for (char c : text) {
        if (c == delimiter) {
            tokens.push_back(cur_token);
            cur_token.clear();
        } else {
            cur_token += c;
        }
    }
    tokens.push_back(cur_token);
    return tokens;
}

// Trims all leading and trailing whitespace, not just " " and "\t"
std::string trim(const std::string& text) {

    std::string trimmed {};
    std::string unsure {};

    for (char c : text) {
        if (trimmed.empty() && std::isspace(static_cast<unsigned char>(c))) {
            continue;
        } else if (!unsure.empty() && !std::isspace(static_cast<unsigned char>(c))) {
            trimmed += unsure;
            trimmed += c;
            unsure.clear();
        } else if (!std::isspace(static_cast<unsigned char>(c))) {
            trimmed += c;
        } else {
            unsure += c;
        }
    }
    return trimmed;
}

bool parse_int(const std::string& token, int& out, std::string& error) {

    const bool has_sign { token[0] == '+' || token[0] == '-' };
    const std::string unsigned_token { has_sign ? token.substr(1) : token};

    if (unsigned_token.empty()) {
        error = "Unsigned token had no value";
        return false;
    }
    for (char c : unsigned_token) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            error = "Unsigned token had no numerical digit";
            return false;
        }
    }
    try {
        out = std::stoi(token);
    } catch (const std::out_of_range&) {
        error = "Value outside of int range";
        return false;
    }
    return true;
}

long long sum(const std::vector<int>& values) {
    long long aggreg { 0 };
    for (int v : values) {
        aggreg += v;
    }
    return aggreg;
}
//must not receive empty vector vals
double mean(const std::vector<int>& values) {
    const long long temp_sum { sum(values) };
    return static_cast<double>(temp_sum) / static_cast<double>(values.size());
}
//must not receive empty vector vals
int min_value(const std::vector<int>& values) {
    int min_v {values[0]};
    
    for (int v : values) {
        if (v < min_v) {
            min_v = v;
        }
    }
    return min_v;
}
//must not receive empty vector vals
int max_value(const std::vector<int>& values) {
    int max_v {values[0]};
    
    for (int v : values) {
        if (v > max_v) {
            max_v = v;
        }
    }

    return max_v;
}

bool parse_prompt_aid(const std::vector<std::string>& tokens, std::vector<int>& values) {
    values.clear(); // clears any unspecified partial results
    for (const std::string& token : tokens) {
        const std::string trimmed_token { trim(token) };
        int parsed_token {};
        std::string error {};
        const bool parse_success { parse_int(trimmed_token, parsed_token, error) };
        if (!parse_success) {
            std::cout << "Retry, parsing failed on token: '" << token << "'" << '\n';
            std::cout << "Encountered: " << error << '\n';
            return false;
        }
        values.push_back(parsed_token);
    }
    return true;
}


int main() {
    
    while (true){
        // Read the input
        std::string inp {};
        std::cout << "Input values or 'quit' to exit: " << '\n';
        if (!std::getline(std::cin, inp)) {
            break;
        }
        // Check for user exit condition
        const std::string triminp { trim(inp) };
        if (triminp == "quit") {
            break;
        } else if (triminp.empty()) {
            std::cout << "No inputs were given, please retry" << '\n';
            continue;
        }
        // Creates token vectors (string and int) from input line
        std::vector<std::string> tokens { split(triminp, ',') };
        std::vector<int> values {};
        // Iterate through each token to trim and parse
        const bool parsed { parse_prompt_aid(tokens, values) };
        if (!parsed) {
            continue;
        }

        std::cout << "Count: " << values.size() << '\n';
        std::cout << "Sum: " << sum(values) << '\n';
        std::cout << "Mean: " << mean(values) << '\n';
        std::cout << "Min: " << min_value(values) << '\n';
        std::cout << "Max: " << max_value(values) << '\n';
    }
    return 0;
}