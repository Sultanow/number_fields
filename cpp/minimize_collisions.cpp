#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <stdexcept>
#include <unordered_set>
#include <tuple>
#include <algorithm>
#include <future>
#include <mutex>
#include <thread>
#include <chrono>
#include <cmath>

#define ASSERT_MSG(cond, msg) { if (!(cond)) throw std::runtime_error("Assertion (" #cond ") failed at line " + std::to_string(__LINE__) + "! Msg: '" + std::string(msg) + "'."); }
#define ASSERT(cond) ASSERT_MSG(cond, "")

using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;

auto ReadCSV(std::string const & fname) {
    auto Split = [&](std::vector<std::string> & v, std::string_view const & s, std::string_view const & delim){
        v.clear();
        size_t start = 0;
        while (true) {
            size_t pos = s.find(delim, start);
            if (pos == std::string::npos)
                pos = s.size();
            v.emplace_back(s.substr(start, pos - start));
            if (pos >= s.size())
                break;
            start = pos + delim.size();
        }
    };
    
    std::cout << "Reading CSV... " << std::flush;
    
    std::map<std::string, std::vector<std::string>> res;
    std::ifstream csv(fname);
    ASSERT_MSG(csv && csv.is_open(), "Failed to open file '" + fname + "'");
    std::string line;
    std::vector<std::string> elems, col_names;
    size_t iline = 0;
    
    while (true) {
        line.clear();
        std::getline(csv, line);
        line.erase(0, line.find_first_not_of("\t\n\v\f\r ")); // left trim
        line.erase(line.find_last_not_of("\t\n\v\f\r ") + 1); // right trim
        if (!csv && line.empty())
            break;
        if (line.empty())
            continue;
        Split(elems, line, ",");
        if (iline == 0)
            col_names = elems;
        else
            for (size_t j = 0; j < elems.size(); ++j) {
                ASSERT_MSG(j < col_names.size(), "j " + std::to_string(j) + ", line '" + line + "'");
                res[col_names[j]].push_back(elems[j]);
            }
        if ((iline & 0x3FFF) == 0)
            std::cout << (iline / 1000) << "K " << std::flush;
        ++iline;
    }
    
    std::cout << std::endl << std::flush;
    return res;
}

std::string NumToStr(u64 x, std::string const & delim = "_", size_t group = 3) {
    std::string inp = std::to_string(x);
    while (inp.size() % group != 0)
        inp = " " + inp;
    std::string out;
    for (size_t i = 0;; ++i) {
        out = inp.substr(inp.size() - (i + 1) * group, group) + out;
        if ((i + 1) * group >= inp.size())
            break;
        out = delim + out;
    }
    return out;
}

void Solve(auto const & csv, std::string const & col_prefix) {
    auto const gtb = std::chrono::high_resolution_clock::now();
    
    auto Time = [gtb]{
        return std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::high_resolution_clock::now() - gtb).count();
    };
    
    double const tb = Time();
    
    std::vector<std::string> col_names;
    for (auto const & [k, v]: csv)
        if (k.starts_with(col_prefix))
            col_names.push_back(k);
    std::sort(col_names.begin(), col_names.end(), [&](auto const & a, auto const & b){
        return std::stoll(a.substr(col_prefix.size())) < std::stoll(b.substr(col_prefix.size()));
    });
    std::vector<std::vector<u8>> c0(col_names.size()), c1(col_names.size());
    u8 max_val = 0;
    for (size_t i = 0; i < csv.at(col_names.at(0)).size(); ++i) {
        bool const is_c0 = csv.at("class_number").at(i) == "1";
        for (size_t j = 0; j < col_names.size(); ++j) {
            auto const val = std::stoll(csv.at(col_names[j]).at(i));
            ASSERT(0 <= val && val <= 255);
            if (is_c0)
                c0[j].push_back(u8(val));
            else
                c1[j].push_back(u8(val));
            max_val = std::max(max_val, u8(val));
        }
    }
    u8 max_bits = 0;
    while (max_val >= (1 << max_bits))
        ++max_bits;
    
    std::mutex mux;
    std::tuple<size_t, size_t, size_t> mini_a, mini_b;
    u64 minv_a = 1ULL << 60, minv_b = 1ULL << 60;
    std::vector<std::future<void>> asyncs;
    size_t const cpu_count = std::thread::hardware_concurrency();
    
    for (size_t i = 0; i < col_names.size(); ++i) {
        asyncs.emplace_back(std::async(std::launch::async, [&, i]{
            std::vector<u32> c0_present;
            std::vector<u8> c0_same;
            for (size_t j = i + 1; j < col_names.size(); ++j)
                for (size_t k = j + 1; k < col_names.size(); ++k) {
                    c0_present.clear();
                    c0_present.resize(1 << (max_bits * 3));
                    c0_same.clear();
                    c0_same.resize(1 << (max_bits * 3));
                    {
                        auto const & [cA, cB, cC] = std::tie(c0[i], c0[j], c0[k]);
                        for (size_t l = 0; l < c0[0].size(); ++l) {
                            auto const pos = (u32(cA[l]) << (max_bits * 2)) | (u32(cB[l]) << max_bits) | u32(cC[l]);
                            //ASSERT(c0_present[pos] < u32(-1));
                            ++c0_present[pos];
                        }
                    }
                    u64 cnt_a = 0, cnt_b = 0;
                    {
                        auto const & [cA, cB, cC] = std::tie(c1[i], c1[j], c1[k]);
                        for (size_t l = 0; l < c1[0].size(); ++l) {
                            auto const pos = (u32(cA[l]) << (max_bits * 2)) | (u32(cB[l]) << max_bits) | u32(cC[l]);
                            cnt_a += c0_present[pos];
                            if (c0_present[pos] > 0) {
                                if (!c0_same[pos]) {
                                    ++cnt_b;
                                    c0_same[pos] = 1;
                                }
                            }
                        }
                    }
                    if (cnt_a < minv_a) {
                        std::unique_lock<std::mutex> lock(mux);
                        if (cnt_a < minv_a) {
                            minv_a = cnt_a;
                            mini_a = std::tie(i, j, k);
                            std::cout
                                << "A, Time " << std::setw(5) << std::llround(Time()) << " sec, ("
                                << std::setw(3) << i << " '" << std::setw(col_prefix.size() + 3) << col_names[i] << "', "
                                << std::setw(3) << j << " '" << std::setw(col_prefix.size() + 3) << col_names[j] << "', " << std::setw(3) << k
                                << " '" << std::setw(col_prefix.size() + 3) << col_names[k] << "'), collisions_a "
                                << std::setw(4 * 4) << NumToStr(minv_a) << std::endl << std::flush;
                        }
                    }
                    if (cnt_b < minv_b) {
                        std::unique_lock<std::mutex> lock(mux);
                        if (cnt_b < minv_b) {
                            minv_b = cnt_b;
                            mini_b = std::tie(i, j, k);
                            std::cout
                                << "B, Time " << std::setw(5) << std::llround(Time()) << " sec, ("
                                << std::setw(3) << i << " '" << std::setw(col_prefix.size() + 3) << col_names[i] << "', "
                                << std::setw(3) << j << " '" << std::setw(col_prefix.size() + 3) << col_names[j] << "', " << std::setw(3) << k
                                << " '" << std::setw(col_prefix.size() + 3) << col_names[k] << "'), collisions_b "
                                << std::setw(4 * 4) << minv_b << std::endl << std::flush;
                        }
                    }
                }
        }));
        while (asyncs.size() >= cpu_count * 3 / 2 || i + 1 >= col_names.size() && !asyncs.empty()) {
            for (size_t j = 0; j < asyncs.size(); ++j) {
                auto & async = asyncs[j];
                if (async.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                    continue;
                async.get();
                asyncs.erase(asyncs.begin() + j);
                break;
            }
            std::this_thread::yield();
        }
    }
}

int main(int argc, char ** argv) {
    try {
        ASSERT(argc >= 2);
        Solve(ReadCSV(argv[1]), argc >= 3 ? argv[2] : "a_");
        return 0;
    } catch (std::exception const & ex) {
        std::cout << "Exception: " << ex.what() << std::endl;
        return -1;
    }
}