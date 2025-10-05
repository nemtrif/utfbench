#include <string_view>
#include <vector>
#include <bit>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <functional>
#include <string>
#include <cstdint>

using namespace std;

constexpr string_view ascii_text = "The quick brown fox jumps over the lazy dog";

void generate_test_data(vector<unsigned char>& data) {
    constexpr int data_multiplier = 50'000'000;
    data.resize(ascii_text.size() * data_multiplier);
    for (int i = 0; i < data_multiplier; ++i) {
        copy(ascii_text.begin(), ascii_text.end(), data.begin() + i * ascii_text.size());
    }
}


int bitmask_utf8_sequence_length(unsigned char lead_byte) {
    if ((lead_byte & 0x80) == 0) {
        return 1;
    } else if ((lead_byte & 0xE0) == 0xC0) {
        return 2;
    } else if ((lead_byte & 0xF0) == 0xE0) {
        return 3;
    } else if ((lead_byte & 0xF8) == 0xF0) {
        return 4;
    } else {
        return 0;
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

int lookuputf8_sequence_length(unsigned char lead_byte) {
    // Hard-coded lookup table for UTF-8 lead byte lengths
static const unsigned char lookup[256] = {
        // 0x00–0x7F: 1
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        // 0x80–0xBF: 0
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        // 0xC0–0xC1: 0
        0,0,
        // 0xC2–0xDF: 2
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        // 0xE0–0xEF: 3
        3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
        // 0xF0–0xF4: 4
        4,4,4,4,4,
        // 0xF5–0xFF: 0
        0,0,0,0,0,0,0,0,0,0,0
    };

    // Access the hard-coded table to determine the sequence length
    return lookup[lead_byte];
}



int main(int argc, char* argv[]) {
    // registry of available functions
    const vector<pair<string, function<int(unsigned char)>>> registry{
        {"bitmask", bitmask_utf8_sequence_length},
        {"countlz", countlz_utf8_sequence_length},
        {"lookup", lookuputf8_sequence_length}
    };

    const vector<string> args(argv + 1, argv + argc);
    vector<string> selected_names;

    for (const auto &a : args) {
        if (a == "--all") {
            for (auto &p : registry) selected_names.push_back(p.first);
        } else if (a == "--bitmask" || a == "bitmask") {
            selected_names.push_back("bitmask");
        } else if (a == "--countlz" || a == "countlz") {
            selected_names.push_back("countlz");
        } else if (a == "--lookup" || a == "lookup") {
            selected_names.push_back("lookup");
        } else if (a == "-h" || a == "--help") {
            cout << "Usage: " << argv[0] << " [--all|--bitmask|--countlz|--lookup]" << '\n';
            cout << "Prints CSV to stdout: function,time_ms,items,checksum" << '\n';
            return 0;
        } else {
            cerr << "Unknown option: " << a << "\n";
            return 2;
        }
    }

    // if nothing selected, show help
    if (selected_names.empty()) {
        cerr << "No functions selected. Use --all or one of --bitmask --countlz --lookup.\n";
        return 2;
    }

    // remove duplicates while preserving order
    vector<string> unique_selected;
    for (auto &n : selected_names) {
        if (find(unique_selected.begin(), unique_selected.end(), n) == unique_selected.end())
            unique_selected.push_back(n);
    }
    selected_names.swap(unique_selected);

    // generate data
    vector<unsigned char> data;
    generate_test_data(data);

    // print CSV header to stdout (no checksum column)
        // print CSV header to stdout (add MB/s column)
        cout << "function,time_ms,items,MB_per_s\n";

    // run each selected function and print results
    for (auto &name : selected_names) {
        auto it = find_if(registry.begin(), registry.end(), [&](auto &p){ return p.first == name; });
        if (it == registry.end()) {
            cerr << "Unknown function: " << name << "\n";
            continue;
        }
        auto fn = it->second;

        auto start = chrono::steady_clock::now();
        for (size_t i = 0; i < data.size(); ++i) {
            (void)fn(data[i]);
        }
        auto end = chrono::steady_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        chrono::duration<double> dur = end - start;
        double secs = dur.count();
        double mb = static_cast<double>(data.size()) / 1e6; // megabytes (10^6 bytes)
        double mbps = (secs > 0.0) ? (mb / secs) : 0.0;

        // CSV to stdout; print throughput with 2 decimal places
        cout << name << ',' << ms << ',' << data.size() << ',' << fixed << setprecision(2) << mbps << '\n' << defaultfloat;
    }

    return 0;
}

