#ifndef SHARES_H
#define SHARES_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <random>
#include <cassert>
#include <errno.h>
#include <cstdlib>

using ll = long long;
using vec = std::vector<int>;
using matrix = std::vector<vec>;
using vi = std::vector<int>;

static const int mod = 1000000007;

inline int modnorm(long long x){
    x %= (long long)mod;
    if(x < 0) x += mod;
    return (int)x;
}
inline int modadd(int a, int b){
    int s = a + b;
    if(s >= mod) s -= mod;
    return s;
}
inline int modsub(int a, int b){
    long long s = (long long)a - (long long)b;
    s %= (long long)mod;
    if(s < 0) s += mod;
    return (int)s;
}
inline int modmul(int a, int b){
    __int128 t = (__int128)a * (__int128)b;
    t %= mod;
    if(t < 0) t += mod;
    return (int)t;
}

inline std::mt19937_64 &global_rng(){
    static std::mt19937_64 rng((unsigned)std::random_device{}());
    return rng;
}
inline int rand_int(int low, int high){
    std::uniform_int_distribution<int> dist(low, high);
    return dist(global_rng());
}

std::string vec_to_string(const vec &v){
    std::string ans;
    ans.push_back('[');
    for(size_t i = 0; i < v.size(); ++i){
        if(i) ans += ", ";
        ans += std::to_string((long long)v[i]);
    }
    ans.push_back(']');
    return ans;
}
std::string mat_to_string(const matrix &m){
    std::string s = "{\n";
    for(size_t i = 0; i < m.size(); ++i){
        s += "  " + vec_to_string(m[i]) + "\n";
    }
    s += "}";
    return s;
}

std::string recv_line(int fd){
    std::string line;
    char c;
    while(true){
        ssize_t r = recv(fd, &c, 1, 0);
        if (r == 0) return std::string();           // orderly shutdown
        if (r < 0) {
            if (errno == EINTR) continue;           // retry on interrupt
            return std::string();                   // error
        }
        if(c == '\r') continue;                     // ignore CR (Windows)
        if(c == '\n') break;
        line.push_back(c);
    }
    return line;
}

void send_line(int fd, const std::string &s){
    std::string t = s + "\n";
    ssize_t written = 0;
    const char *buf = t.c_str();
    size_t to_write = t.size();
    while(to_write > 0){
        ssize_t w = send(fd, buf + written, to_write, 0);
        if(w < 0){
            if(errno == EINTR) continue;
            perror("send");
            return;
        }
        written += w;
        to_write -= (size_t)w;
    }
}

vec parse_vec_from_line(const std::string &line){
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    vec v;
    long long x;
    while(iss >> x) v.push_back(modnorm(x));
    return v;
}

int parse_scalar_from_line(const std::string &line){
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    long long x = 0;
    iss >> x;
    return modnorm(x);
}

/*
 Robust party_shares: write_to_file is the same format as before.
 read_from_file is resilient: it scans the file and picks numeric tokens while skipping comments
 (lines that start with '#') and other non-numeric tokens.
*/
struct party_shares{
    matrix U;
    matrix V;
    matrix dot_a;
    matrix dot_b;
    vec dot_c;
    matrix sv_a;
    vec sv_b;
    matrix sv_c;

    void write_to_file(const std::string &filename) const {
        std::ofstream ofs(filename);
        if(!ofs) { std::cerr << "cannot open file " << filename << std::endl; std::exit(1); }
        int m = (int)U.size();
        int n = (int)V.size();
        if(m == 0 || n == 0) { std::cerr << "Empty U or V" << std::endl; std::exit(1); }
        if(U[0].size() != V[0].size()){ std::cerr << "ERROR : U[0] size is not equal to V[0] size\n"; std::exit(1); }
        int k = (int)U[0].size();
        int q = (int)dot_c.size();
        ofs << "M " << m << " N " << n << " K " << k << " Q " << q << "\n";
        ofs << "# U\n";
        for(int i = 0; i < m; ++i){
            for(int j = 0; j < k; ++j) ofs << modnorm(U[i][j]) << (j + 1 < k ? " " : "");
            ofs << "\n";
        }
        ofs << "# V\n";
        for(int i = 0; i < n; ++i){
            for(int j = 0; j < k; ++j) ofs << modnorm(V[i][j]) << (j + 1 < k ? " " : "");
            ofs << "\n";
        }
        ofs << "# DOT_TRIPLES\n";
        for(int i = 0; i < q; ++i){
            for(int j = 0; j < k; ++j) ofs << modnorm(dot_a[i][j]) << (j + 1 < k ? " " : "");
            ofs << "\n";
            for(int j = 0; j < k; ++j) ofs << modnorm(dot_b[i][j]) << (j + 1 < k ? " " : "");
            ofs << "\n";
            ofs << modnorm(dot_c[i]) << "\n";
        }
        ofs << "# Scalar_Vector_TRIPLES\n";
        for(int i = 0; i < q; ++i){
            for(int j = 0; j < k; ++j) ofs << modnorm(sv_a[i][j]) << (j + 1 < k ? " " : "");
            ofs << "\n";
            ofs << modnorm(sv_b[i]) << "\n";
            for(int j = 0; j < k; ++j) ofs << modnorm(sv_c[i][j]) << (j + 1 < k ? " " : "");
            ofs << "\n";
        }
        ofs.close();
    }

private:
    static long long read_next_number(std::ifstream &ifs){
        std::string tok;
        while(ifs >> tok){
            if(tok.size() > 0 && tok[0] == '#'){
                // skip rest of the line
                std::string rest;
                std::getline(ifs, rest);
                continue;
            }
            // try parse as integer using strtoll
            char *endptr = nullptr;
            errno = 0;
            long long val = std::strtoll(tok.c_str(), &endptr, 10);
            if(endptr != tok.c_str()){
                return val;
            }
            // not a number, continue
        }
        std::cerr << "Unexpected end of file while parsing numbers\n";
        std::exit(1);
    }

public:
    void read_from_file(const std::string &filename){
        std::ifstream ifs(filename);
        if(!ifs){ std::cerr << "cannot open file : " << filename << std::endl; std::exit(1); }

        // read four numeric values in order: m n k q (file has M m N n K k Q q but we'll just grab numbers)
        long long m_ll = read_next_number(ifs);
        long long n_ll = read_next_number(ifs);
        long long k_ll = read_next_number(ifs);
        long long q_ll = read_next_number(ifs);
        int m = (int)m_ll;
        int n = (int)n_ll;
        int k = (int)k_ll;
        int q = (int)q_ll;

        U.assign(m, vec(k,0));
        V.assign(n, vec(k,0));
        for(int i = 0; i < m; ++i){
            for(int j = 0; j < k; ++j){
                long long tmp = read_next_number(ifs);
                U[i][j] = modnorm(tmp);
            }
        }
        for(int i = 0; i < n; ++i){
            for(int j = 0; j < k; ++j){
                long long tmp = read_next_number(ifs);
                V[i][j] = modnorm(tmp);
            }
        }

        dot_a.assign(q, vec(k));
        dot_b.assign(q, vec(k));
        dot_c.assign(q, 0);
        for(int t = 0; t < q; ++t){
            for(int j = 0; j < k; ++j){
                long long tmp = read_next_number(ifs);
                dot_a[t][j] = modnorm(tmp);
            }
            for(int j = 0; j < k; ++j){
                long long tmp = read_next_number(ifs);
                dot_b[t][j] = modnorm(tmp);
            }
            long long tmpc = read_next_number(ifs);
            dot_c[t] = modnorm(tmpc);
        }

        sv_a.assign(q, vec(k));
        sv_b.assign(q, 0);
        sv_c.assign(q, vec(k));
        for(int t = 0; t < q; ++t){
            for(int j = 0; j < k; ++j){
                long long tmp = read_next_number(ifs);
                sv_a[t][j] = modnorm(tmp);
            }
            long long tmpb = read_next_number(ifs);
            sv_b[t] = modnorm(tmpb);
            for(int j = 0; j < k; ++j){
                long long tmp2 = read_next_number(ifs);
                sv_c[t][j] = modnorm(tmp2);
            }
        }
        ifs.close();
    }
};

#endif // SHARES_H
