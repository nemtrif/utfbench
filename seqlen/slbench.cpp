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
    return lookup[lead_byte];
}

// Enum identifying each sequence-length implementation
enum class SeqlenMethod { Bitmask = 0, Countlz = 1, Lookup = 2 };

// Each run_* function performs the timed inner loop for its method and
// returns tuple<ms, items_processed>.
static tuple<long long, size_t> run_bitmask(const vector<unsigned char>& data) {
    size_t count = 0;
    auto start = chrono::steady_clock::now();
    for (size_t i = 0; i < data.size(); ++i) {
        i += bitmask_utf8_sequence_length(data[i]);
        ++count;
    }
    auto end = chrono::steady_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    return make_tuple(ms, count);
}

static tuple<long long, size_t> run_countlz(const vector<unsigned char>& data) {
    size_t count = 0;
    auto start = chrono::steady_clock::now();
    for (size_t i = 0; i < data.size(); ++i) {
        i += countlz_utf8_sequence_length(data[i]);
        ++count;
    }
    auto end = chrono::steady_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    return make_tuple(ms, count);
}

static tuple<long long, size_t> run_lookup(const vector<unsigned char>& data) {
    size_t count = 0;
    auto start = chrono::steady_clock::now();
    for (size_t i = 0; i < data.size(); ++i) {
        i += lookuputf8_sequence_length(data[i]);
        ++count;
    }
    auto end = chrono::steady_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    return make_tuple(ms, count);
}


int main(int argc, char* argv[]) {
    // registry name -> enum id
    const vector<pair<string, SeqlenMethod>> registry = {
        {"bitmask", SeqlenMethod::Bitmask},
        {"countlz", SeqlenMethod::Countlz},
        {"lookup", SeqlenMethod::Lookup}
    };

    const vector<string> args(argv + 1, argv + argc);
    vector<SeqlenMethod> selected;

    for (const auto &a : args) {
        if (a == "--all") {
            for (auto &p : registry) selected.push_back(p.second);
        } else if (a == "--bitmask" || a == "bitmask") {
            selected.push_back(SeqlenMethod::Bitmask);
        } else if (a == "--countlz" || a == "countlz") {
            selected.push_back(SeqlenMethod::Countlz);
        } else if (a == "--lookup" || a == "lookup") {
            selected.push_back(SeqlenMethod::Lookup);
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
    if (selected.empty()) {
        cerr << "No functions selected. Use --all or one of --bitmask --countlz --lookup.\n";
        return 2;
    }

    // generate data
    vector<unsigned char> data;
    generate_test_data(data);

    // CSV header
    cout << "function,time_ms,items,MB_per_s\n";

    // run each selected function and print results
    for (auto &method : selected) {
        long long ms = 0;
        size_t count = 0;
        switch (method) {
            case SeqlenMethod::Bitmask:
                std::tie(ms, count) = run_bitmask(data);
            break;
            case SeqlenMethod::Countlz:
                std::tie(ms, count) = run_countlz(data);
            break;
            case SeqlenMethod::Lookup:
                std::tie(ms, count) = run_lookup(data);
            break;
        }
        chrono::duration<double> dur = chrono::milliseconds(ms);
        double secs = dur.count();
        double mb = static_cast<double>(data.size()) / 1e6;
        double mbps = (secs > 0.0) ? (mb / secs) : 0.0;

        // get name for printing
        string name;
        for (auto &p : registry) if (p.second == method) name = p.first;
        cout << name << ',' << ms << ',' << count << ',' << fixed << setprecision(2) << mbps << '\n' << defaultfloat;
    }

    return 0;
}

