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
#include <filesystem>
#include <sstream>

#define ASSERT_MSG(cond, msg) { if (!(cond)) throw std::runtime_error("Assertion (" #cond ") failed at line " + std::to_string(__LINE__) + "! Msg: '" + std::string(msg) + "'."); }
#define ASSERT(cond) ASSERT_MSG(cond, "")

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i64 = int64_t;

double Time() {
    static auto const gtb = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::high_resolution_clock::now() - gtb).count();
}

class Timing {
public:
    Timing(std::string const & name)
        : name_(name), tb_(Time()) {
    }
    ~Timing() {
        double const tp = Time() - tb_;
        std::cout << "'" << name_ << "' time " << std::fixed
                  << std::setprecision(2) << (tp >= 60 ? (tp / 60.0) : tp)
                  << (tp >= 60 ? " mins" : " sec") << std::endl;
    }
private:
    std::string name_;
    double const tb_ = 0;
};

auto ReadCSV(std::string const & fname) {
    Timing tim("CSV Read");

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

struct ParsedData {
    std::vector<std::string> col_names;
    std::vector<std::vector<u8>> c0, c1;
    std::vector<u8> val2idx;
    size_t vals_cnt = 0, max_bits = 0;
};

auto Parse(auto const & csv, std::string const & col_prefix) {
    Timing tim("CSV Parse");
    ParsedData parsed_data;
    auto & col_names = parsed_data.col_names;
    for (auto const & [k, v]: csv)
        if (k.starts_with(col_prefix))
            col_names.push_back(k);
    std::sort(col_names.begin(), col_names.end(), [&](auto const & a, auto const & b){
        return std::stoll(a.substr(col_prefix.size())) < std::stoll(b.substr(col_prefix.size()));
    });
    {
        std::ofstream f("green_red_columns_names.txt");
        ASSERT(f && f.is_open());
        for (size_t i = 0; i < col_names.size(); ++i)
            f << i << "," << col_names[i] << std::endl;
    }
    auto & c0 = parsed_data.c0, & c1 = parsed_data.c1;
    c0.resize(col_names.size());
    c1.resize(col_names.size());
    std::vector<bool> classes;
    size_t const num_rows = csv.at(col_names.at(0)).size();
    for (size_t i = 0; i < num_rows; ++i)
        classes.push_back(csv.at("class_number").at(i) == "1");
    auto & val2idx = parsed_data.val2idx;
    u8 max_val = 0;
    for (size_t j = 0; j < col_names.size(); ++j) {
        auto & col = csv.at(col_names[j]);
        for (size_t i = 0; i < num_rows; ++i) {
            auto const val = std::stoll(col.at(i));
            ASSERT(0 <= val && val <= 255);
            max_val = std::max(max_val, u8(val));
            if (max_val >= val2idx.size())
                val2idx.resize(max_val + 1);
            val2idx[val] = 1;
        }
    }
    auto & vals_cnt = parsed_data.vals_cnt;
    {
        size_t idx = 0;
        for (auto & e: val2idx) {
            if (e == 0)
                continue;
            ASSERT(idx < 255);
            e = idx;
            ++idx;
        }
        vals_cnt = idx;
    }
    for (size_t j = 0; j < col_names.size(); ++j) {
        auto & col = csv.at(col_names[j]);
        for (size_t i = 0; i < num_rows; ++i) {
            bool const is_c0 = classes[i];
            auto const val = std::stoll(col[i]);
            if (is_c0)
                c0[j].push_back(u8(val2idx[val]));
            else
                c1[j].push_back(u8(val2idx[val]));
        }
    }
    {
        auto & max_bits = parsed_data.max_bits;
        while (vals_cnt > (1 << max_bits))
            ++max_bits;
    }
    return parsed_data;
}

void SolveVar0(auto const & csv, std::string const & col_prefix) {
    Timing tim("SolveVar0");

    double const tb = Time();
    
    auto const parsed_data = Parse(csv, col_prefix);
    
    auto & c0 = parsed_data.c0, & c1 = parsed_data.c1;
    size_t const max_bits = parsed_data.max_bits;
    auto const & col_names = parsed_data.col_names;
    
    std::mutex mux;
    std::tuple<size_t, size_t, size_t> mini_a, mini_b;
    u64 minv_a = 1ULL << 60, minv_b = 1ULL << 60, minv_ba = 1ULL << 60;
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
                    if (cnt_b <= minv_b) {
                        std::unique_lock<std::mutex> lock(mux);
                        if (cnt_b <= minv_b) {
                            bool updated = false;
                            if (cnt_b < minv_b || cnt_b == minv_b && cnt_a < minv_ba) {
                                minv_b = cnt_b;
                                minv_ba = cnt_a;
                                mini_b = std::tie(i, j, k);
                                updated = true;
                            }
                            if (updated)
                                std::cout
                                    << "B, Time " << std::setw(5) << std::llround(Time()) << " sec, ("
                                    << std::setw(3) << i << " '" << std::setw(col_prefix.size() + 3) << col_names[i] << "', "
                                    << std::setw(3) << j << " '" << std::setw(col_prefix.size() + 3) << col_names[j] << "', " << std::setw(3) << k
                                    << " '" << std::setw(col_prefix.size() + 3) << col_names[k] << "'), collisions_b "
                                    << std::setw(4 * 4) << minv_b << ", " << std::setw(4 * 4) << NumToStr(minv_ba) << std::endl << std::flush;
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

struct __attribute__((packed)) PrecEntry {
    u16 i = 0, j = 0, k = 0, c0_cnt = 0, cmn_cnt = 0, c1_cnt = 0;
};

std::string const prec_fname = "green_red_precomputed.dat";

void PreCompute(auto const & csv, std::string const & col_prefix) {
    Timing tim("PreCompute");
    
    auto const parsed_data = Parse(csv, col_prefix);

    double const tb = Time();
    
    auto & c0 = parsed_data.c0, & c1 = parsed_data.c1;
    size_t const max_bits = parsed_data.max_bits;
    auto const & col_names = parsed_data.col_names;
    
    std::vector<std::future<std::vector<PrecEntry>>> asyncs;
    size_t const cpu_count = std::thread::hardware_concurrency();

    {
        std::ofstream f(prec_fname, std::ios::binary);
        ASSERT(f && f.is_open());
    }

    size_t num_done = 0;
    double report_time = -1000;

    for (size_t i = 0; i < col_names.size(); ++i) {
        asyncs.emplace_back(std::async(std::launch::async, [&, i]{
            std::vector<u8> c0_present, c1_present;
            std::vector<PrecEntry> entries;
            
            for (size_t j = i + 1; j < col_names.size(); ++j)
                for (size_t k = j + 1; k < col_names.size(); ++k) {
                    c0_present.clear();
                    c0_present.resize(1 << (max_bits * 3));
                    u64 c0_cnt = 0, c1_cnt = 0, cmn_cnt = 0;
                    {
                        auto const & [cA, cB, cC] = std::tie(c0[i], c0[j], c0[k]);
                        for (size_t l = 0; l < c0[0].size(); ++l) {
                            auto const pos = (u32(cA[l]) << (max_bits * 2)) | (u32(cB[l]) << max_bits) | u32(cC[l]);
                            if (!c0_present[pos]) {
                                ++c0_cnt;
                                c0_present[pos] = 1;
                            }
                        }
                    }
                    c1_present.clear();
                    c1_present.resize(1 << (max_bits * 3));
                    {
                        auto const & [cA, cB, cC] = std::tie(c1[i], c1[j], c1[k]);
                        for (size_t l = 0; l < c1[0].size(); ++l) {
                            auto const pos = (u32(cA[l]) << (max_bits * 2)) | (u32(cB[l]) << max_bits) | u32(cC[l]);
                            if (!c1_present[pos]) {
                                ++c1_cnt;
                                c1_present[pos] = 1;
                                if (c0_present[pos])
                                    ++cmn_cnt;
                            }
                        }
                    }
                    ASSERT(c0_cnt < (1 << 16));
                    ASSERT(cmn_cnt < (1 << 16));
                    ASSERT(c1_cnt < (1 << 16));
                    entries.push_back({.i = u16(i), .j = u16(j), .k = u16(k), .c0_cnt = u16(c0_cnt), .cmn_cnt = u16(cmn_cnt), .c1_cnt = u16(c1_cnt)});
                }
            
            return std::move(entries);
        }));
        while (asyncs.size() >= cpu_count * 3 / 2 || i + 1 >= col_names.size() && !asyncs.empty()) {
            for (size_t j = 0; j < asyncs.size(); ++j) {
                auto & async = asyncs[j];
                if (async.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                    continue;
                auto const entries = async.get();
                asyncs.erase(asyncs.begin() + j);
                {
                    std::ofstream f(prec_fname, std::ios::binary | std::ios::app);
                    ASSERT(f && f.is_open());
                    f.write((char*)entries.data(), entries.size() * sizeof(entries[0]));
                }
                ++num_done;
                if (Time() - report_time >= 30 || num_done >= col_names.size()) {
                    std::cout << std::fixed << std::setprecision(1) << double(num_done) * 100.0 / col_names.size()
                        << "% (" << std::llround(Time() - tb) << " sec), " << std::flush;
                    report_time = Time();
                }
                break;
            }
            std::this_thread::yield();
        }
    }
    std::cout << std::endl;
}

void SolveVar1(size_t cnt, auto const & cost) {
    Timing tim("SolveVar1");
    
    u64 const fsize = std::filesystem::file_size(prec_fname);
    
    std::ifstream f(prec_fname, std::ios::binary);
    ASSERT_MSG(f && f.is_open(), "Failed to open file '" + prec_fname + "'.");
    std::vector<PrecEntry> entries, gheap;
    size_t const read_block = 1 << 25;
    //std::function<bool(PrecEntry const &, PrecEntry const &)>
    auto const Cmp =
        [&](auto const & a, auto const & b){
            return cost(a.c0_cnt, a.cmn_cnt, a.c1_cnt) < cost(b.c0_cnt, b.cmn_cnt, b.c1_cnt);
        };
    
    auto const tb = Time();
    double report_time = -1000;
    
    {
        Timing tim("Main Stage");
        
        std::vector<std::future<std::tuple<size_t, std::vector<PrecEntry>>>> asyncs;
        size_t const cpu_count = std::thread::hardware_concurrency();
        size_t total_processed = 0;
        
        while (true) {
            entries.clear();
            entries.resize(read_block);
            f.read((char*)entries.data(), entries.size() * sizeof(entries[0]));
            i64 const readed = f.gcount();
            ASSERT(readed % sizeof(entries[0]) == 0);
            entries.resize(readed / sizeof(entries[0]));
            
            asyncs.push_back(std::async(std::launch::async, [&, entries]{
                std::vector<PrecEntry> heap;
                for (auto const & entry: entries) {
                    heap.push_back(entry);
                    std::push_heap(heap.data(), heap.data() + heap.size(), Cmp);
                    if (heap.size() > cnt) {
                        std::pop_heap(heap.data(), heap.data() + heap.size(), Cmp);
                        heap.pop_back();
                    }
                }
                std::sort_heap(heap.data(), heap.data() + heap.size(), Cmp);
                return std::make_tuple(entries.size(), std::move(heap));
            }));
            
            while (asyncs.size() >= cpu_count * 3 / 2 || !asyncs.empty() && entries.size() < read_block) {
                for (size_t j = 0; j < asyncs.size(); ++j) {
                    auto & async = asyncs[j];
                    if (async.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                        continue;
                    auto const [entries_processed, heap_async] = async.get();
                    asyncs.erase(asyncs.begin() + j);
                    
                    total_processed += entries_processed;
                    
                    for (auto const & entry: heap_async) {
                        gheap.push_back(entry);
                        std::push_heap(gheap.data(), gheap.data() + gheap.size(), Cmp);
                        if (gheap.size() > cnt) {
                            std::pop_heap(gheap.data(), gheap.data() + gheap.size(), Cmp);
                            gheap.pop_back();
                        }
                    }
                    
                    break;
                }
                std::this_thread::yield();
            }
            
            if (Time() - report_time >= 30 || entries.size() < read_block) {
                std::cout << std::fixed << std::setprecision(1) << double(total_processed) * 100.0 / (fsize / sizeof(entries[0]))
                    << "% (" << std::llround(Time() - tb) << " sec), " << std::flush;
                report_time = Time();
            }
            
            if (entries.size() < read_block)
                break;
        }
        
        std::cout << std::endl;
        
        std::sort_heap(gheap.data(), gheap.data() + gheap.size(), Cmp);
    }
    
    {
        Timing tim("Save Results");

        auto const tb = Time();
        
        std::stringstream ss;
        std::ofstream fout("green_red_answer.txt");
        ASSERT(fout && fout.is_open());
        for (size_t i = 0; i < gheap.size(); ++i) {
            auto const & entry = gheap[i];
            ss << entry.i << "," << entry.j << "," << entry.k << "," << entry.c0_cnt << "," << entry.cmn_cnt << ","
                << entry.c1_cnt << "," << std::fixed << cost(entry.c0_cnt, entry.cmn_cnt, entry.c1_cnt) << std::endl;
            if (ss.view().size() >= (1 << 24) || i == 0 || i + 1 >= gheap.size()) {
                fout << ss.str();
                ss.str("");
                std::cout << std::fixed << std::setprecision(1) << double(i + 1) * 100.0 / gheap.size()
                    << "% (" << std::llround(Time() - tb) << " sec), " << std::flush;
            }
        }
        
        std::cout << std::endl;
    }
}

int main(int argc, char ** argv) {
    try {
        ASSERT(argc >= 2);
        //SolveVar0(ReadCSV(argv[1]), argc >= 3 ? argv[2] : "a_");
        PreCompute(ReadCSV(argv[1]), argc >= 3 ? argv[2] : "a_");
        SolveVar1(10'000, [](double c0_cnt, double cmn_cnt, double c1_cnt){
            return cmn_cnt / (c0_cnt + c1_cnt - 2 * cmn_cnt);
        });
        return 0;
    } catch (std::exception const & ex) {
        std::cout << "Exception: " << ex.what() << std::endl;
        return -1;
    }
}