#include <string_view>
#include <vector>
#include <bit>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <tuple>
#include <string>

using namespace std;

constexpr string_view ascii_text = "The quick brown fox jumps over the lazy dog";

void generate_test_data(vector<unsigned char>& data) {
    constexpr int data_multiplier = 1'000'000;
    data.resize(ascii_text.size() * data_multiplier);
    for (int i = 0; i < data_multiplier; ++i) {
        data.insert(data.end(), ascii_text.begin(), ascii_text.end());
    }
}


int countlz_utf8_sequence_length(unsigned char lead_byte) {
    switch (std::countl_one(lead_byte)) {
        case 0: return 1;
        case 2: return 2;
        case 3: return 3;
        case 4: return 4;
        default: return 0; // invalid lead
    }
}


int main() {
    // generate data
    vector<unsigned char> data;
    generate_test_data(data);

    // CSV header
    cout << "time_ms,items,MB_per_s\n";

    long long ms = 0;
    size_t count = 0;
    auto start = chrono::steady_clock::now();
    for (size_t i = 0; i < data.size(); ++i) {
        i += countlz_utf8_sequence_length(data[i]);
        ++count;
    }
    auto end = chrono::steady_clock::now();
    ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    chrono::duration<double> dur = chrono::milliseconds(ms);
    double secs = dur.count();
    double mb = static_cast<double>(data.size()) / 1e6;
    double mbps = (secs > 0.0) ? (mb / secs) : 0.0;

    cout << ms << ',' << count << ',' << fixed << setprecision(2) << mbps << '\n' << defaultfloat;

    return 0;
}

