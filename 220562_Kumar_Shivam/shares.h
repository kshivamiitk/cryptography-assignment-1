#ifndef SHARES_H
#define SHARES_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <random>
#include <cerrno>
#include <cstring>
#include <cstdlib>

using ll = long long;
using vec = std::vector<int>;
using matrix = std::vector<vec>;

static const int MOD = 1000000007;

inline int modnorm(long long x){
    x %= (long long)MOD;
    if(x < 0) x += MOD;
    return (int)x;
}
inline int modadd(int a, int b){
    int s = a + b;
    if(s >= MOD) s -= MOD;
    return s;
}
inline int modsub(int a, int b){
    long long s = (long long)a - (long long)b;
    s %= (long long)MOD;
    if(s < 0) s += MOD;
    return (int)s;
}
inline int modmul(int a, int b){
    __int128 t = (__int128)a * (__int128)b;
    t %= MOD;
    if(t < 0) t += MOD;
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


static std::string recv_line(int fd){
    std::string line;
    char c;
    while(true){
        ssize_t r = recv(fd, &c, 1, 0);
        if(r == 0) return std::string(); 
        if(r < 0){
            if(errno == EINTR) continue;
            return std::string();
        }
        if(c == '\r') continue;
        if(c == '\n') break;
        line.push_back(c);
    }
    return line;
}

static void send_line(int fd, const std::string &s){
    std::string t = s + "\n";
    const char *buf = t.c_str();
    size_t remaining = t.size();
    ssize_t off = 0;
    while(remaining > 0){
        ssize_t w = send(fd, buf + off, remaining, 0);
        if(w < 0){
            if(errno == EINTR) continue;
            perror("send");
            return;
        }
        off += w;
        remaining -= (size_t)w;
    }
}


static vec parse_vec_from_line(const std::string &line){
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    vec v;
    long long x;
    while(iss >> x) v.push_back(modnorm(x));
    return v;
}

static int parse_scalar_from_line(const std::string &line){
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    long long x = 0;
    iss >> x;
    return modnorm(x);
}


struct party_shares {
    matrix U; // m x k
    matrix V; // n x k
    matrix dot_a; // q x k
    matrix dot_b; // q x k
    vec dot_c;    // q
    matrix sv_a;  // q x k
    vec sv_b;     // q
    matrix sv_c;  // q x k
    std::vector<matrix> sel_a; 
    std::vector<vec>    sel_b; 
    std::vector<matrix> sel_c; 

    matrix j_shares; 

    void write_to_file(const std::string &fname) const {
        std::ofstream ofs(fname);
        if(!ofs){ std::cerr << "cannot open " << fname << " for writing\n"; std::exit(1); }
        int m = (int)U.size();
        int n = (int)V.size();
        int k = (m>0 ? (int)U[0].size() : (n>0 ? (int)V[0].size() : 0));
        int q = (int)dot_c.size();
        ofs << m << " " << n << " " << k << " " << q << "\n";
        ofs << "# U\n";
        for(int i = 0; i < m; ++i){
            for(int j = 0; j < k; ++j) ofs << modnorm(U[i][j]) << (j+1<k ? " " : "");
            ofs << "\n";
        }
        ofs << "# V\n";
        for(int i = 0; i < n; ++i){
            for(int j = 0; j < k; ++j) ofs << modnorm(V[i][j]) << (j+1<k ? " " : "");
            ofs << "\n";
        }
        ofs << "# DOT_TRIPLES\n";
        for(int t = 0; t < q; ++t){
            for(int j = 0; j < k; ++j) ofs << modnorm(dot_a[t][j]) << (j+1<k ? " " : "");
            ofs << "\n";
            for(int j = 0; j < k; ++j) ofs << modnorm(dot_b[t][j]) << (j+1<k ? " " : "");
            ofs << "\n";
            ofs << modnorm(dot_c[t]) << "\n";
        }
        ofs << "# SV_TRIPLES\n";
        for(int t = 0; t < q; ++t){
            for(int j = 0; j < k; ++j) ofs << modnorm(sv_a[t][j]) << (j+1<k ? " " : "");
            ofs << "\n";
            ofs << modnorm(sv_b[t]) << "\n";
            for(int j = 0; j < k; ++j) ofs << modnorm(sv_c[t][j]) << (j+1<k ? " " : "");
            ofs << "\n";
        }
        ofs << "# SEL_TRIPLES\n";
        for(int t = 0; t < q; ++t){
            int n_local = (int)sel_b[t].size();
            for(int idx = 0; idx < n_local; ++idx){
                for(int j = 0; j < k; ++j) ofs << modnorm(sel_a[t][idx][j]) << (j+1<k ? " " : "");
                ofs << "\n";
                ofs << modnorm(sel_b[t][idx]) << "\n";
                for(int j = 0; j < k; ++j) ofs << modnorm(sel_c[t][idx][j]) << (j+1<k ? " " : "");
                ofs << "\n";
            }
        }
        ofs << "# J_SHARES\n";
        for(size_t t = 0; t < j_shares.size(); ++t){
            for(size_t j = 0; j < j_shares[t].size(); ++j){
                ofs << modnorm(j_shares[t][j]) << (j+1<j_shares[t].size() ? " " : "");
            }
            ofs << "\n";
        }
        ofs.close();
    }

    
    static long long read_next_number(std::ifstream &ifs){
        std::string tok;
        while(ifs >> tok){
            if(tok.size() > 0 && tok[0] == '#'){
                std::string rest;
                std::getline(ifs, rest);
                continue;
            }

            char *endptr = nullptr;
            errno = 0;
            long long v = std::strtoll(tok.c_str(), &endptr, 10);
            if(endptr != tok.c_str()) return v;
        }
        std::cerr << "Unexpected EOF while reading shares file\n";
        std::exit(1);
    }

    void read_from_file(const std::string &fname){
        std::ifstream ifs(fname);
        if(!ifs){ std::cerr << "cannot open " << fname << " for reading\n"; std::exit(1); }
        long long m_ll = read_next_number(ifs);
        long long n_ll = read_next_number(ifs);
        long long k_ll = read_next_number(ifs);
        long long q_ll = read_next_number(ifs);
        int m = (int)m_ll, n = (int)n_ll, k = (int)k_ll, q = (int)q_ll;
        U.assign(m, vec(k,0));
        V.assign(n, vec(k,0));
        for(int i = 0; i < m; ++i) for(int j = 0; j < k; ++j) U[i][j] = modnorm(read_next_number(ifs));
        for(int i = 0; i < n; ++i) for(int j = 0; j < k; ++j) V[i][j] = modnorm(read_next_number(ifs));
        dot_a.assign(q, vec(k));
        dot_b.assign(q, vec(k));
        dot_c.assign(q, 0);
        for(int t = 0; t < q; ++t){
            for(int j = 0; j < k; ++j) dot_a[t][j] = modnorm(read_next_number(ifs));
            for(int j = 0; j < k; ++j) dot_b[t][j] = modnorm(read_next_number(ifs));
            dot_c[t] = modnorm(read_next_number(ifs));
        }
        sv_a.assign(q, vec(k));
        sv_b.assign(q, 0);
        sv_c.assign(q, vec(k));
        for(int t = 0; t < q; ++t){
            for(int j = 0; j < k; ++j) sv_a[t][j] = modnorm(read_next_number(ifs));
            sv_b[t] = modnorm(read_next_number(ifs));
            for(int j = 0; j < k; ++j) sv_c[t][j] = modnorm(read_next_number(ifs));
        }
        sel_a.assign(q, matrix(n, vec(k)));
        sel_b.assign(q, vec(n, 0));
        sel_c.assign(q, matrix(n, vec(k)));
        for(int t = 0; t < q; ++t){
            for(int idx = 0; idx < n; ++idx){
                for(int j = 0; j < k; ++j) sel_a[t][idx][j] = modnorm(read_next_number(ifs));
                sel_b[t][idx] = modnorm(read_next_number(ifs));
                for(int j = 0; j < k; ++j) sel_c[t][idx][j] = modnorm(read_next_number(ifs));
            }
        }
        j_shares.assign(q, vec(n, 0));
        for(int t = 0; t < q; ++t){
            for(int j = 0; j < n; ++j) j_shares[t][j] = modnorm(read_next_number(ifs));
        }
        ifs.close();
    }
};

#endif 
