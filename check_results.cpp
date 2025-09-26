
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <cstdlib>

static const long long MOD = 1000000007LL;
using ll = long long;
using ivec = std::vector<int>;
using matrix = std::vector<ivec>;

static inline long long modnorm_ll(long long x){
    x %= MOD;
    if(x < 0) x += MOD;
    return x;
}
static inline int modnorm(int x){
    long long t = x;
    return (int)modnorm_ll(t);
}
static inline int modmul_ll(long long a, long long b){
    __int128 t = (__int128)a * (__int128)b;
    t %= MOD;
    if(t < 0) t += MOD;
    return (int)t;
}

static std::vector<long long> read_tokens(const std::string &filename){
    std::ifstream ifs(filename);
    if(!ifs){
        std::cerr << "Cannot open file: " << filename << "\n";
        std::exit(1);
    }
    std::vector<long long> tokens;
    std::string tok;
    while(ifs >> tok){
        if(tok.size() > 0 && tok[0] == '#'){
            std::string rest;
            std::getline(ifs, rest);
            continue;
        }
        char *endptr = nullptr;
        errno = 0;
        long long val = std::strtoll(tok.c_str(), &endptr, 10);
        if(endptr != tok.c_str()){
            tokens.push_back(val);
        } else {

        }
    }
    return tokens;
}

static void parse_shares_file(const std::string &fname, int &m, int &n, int &k, int &q, matrix &U, matrix &V){
    auto tokens = read_tokens(fname);
    if(tokens.size() < 4){
        std::cerr << "Not enough tokens in " << fname << "\n";
        std::exit(1);
    }

    m = (int)tokens[0];
    n = (int)tokens[1];
    k = (int)tokens[2];
    q = (int)tokens[3];

    std::size_t idx = 4;
    U.assign(m, ivec(k, 0));
    for(int i = 0; i < m; ++i){
        for(int j = 0; j < k; ++j){
            if(idx >= tokens.size()){ std::cerr << "Unexpected EOF while reading U from " << fname << "\n"; std::exit(1); }
            U[i][j] = (int)modnorm_ll(tokens[idx++]);
        }
    }
    V.assign(n, ivec(k, 0));
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < k; ++j){
            if(idx >= tokens.size()){ std::cerr << "Unexpected EOF while reading V from " << fname << "\n"; std::exit(1); }
            V[i][j] = (int)modnorm_ll(tokens[idx++]);
        }
    }
}

int main(){

    const std::string P0_ORIG = "party0_shares.orig";
    const std::string P1_ORIG = "party1_shares.orig";
    const std::string P0_FINAL = "party0_shares.txt";
    const std::string P1_FINAL = "party1_shares.txt";
    const std::string QUERIES = "queries.txt";

    int m0,n0,k0,q0;
    matrix U0_orig, V0_orig;
    parse_shares_file(P0_ORIG, m0, n0, k0, q0, U0_orig, V0_orig);

    int m1,n1,k1,q1;
    matrix U1_orig, V1_orig;
    parse_shares_file(P1_ORIG, m1, n1, k1, q1, U1_orig, V1_orig);

    if(!(m0==m1 && n0==n1 && k0==k1 && q0==q1)){
        std::cerr << "Mismatch in dimensions between party0 and party1 original files\n";
        return 2;
    }
    int m = m0, n = n0, k = k0, q = q0;

    // reconstruct original plaintext U and V by adding orig shares
    matrix U_plain(m, ivec(k,0));
    matrix V_plain(n, ivec(k,0));
    for(int i = 0; i < m; ++i){
        for(int j = 0; j < k; ++j){
            U_plain[i][j] = (int)modnorm_ll((long long)U0_orig[i][j] + (long long)U1_orig[i][j]);
        }
    }
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < k; ++j){
            V_plain[i][j] = (int)modnorm_ll((long long)V0_orig[i][j] + (long long)V1_orig[i][j]);
        }
    }

    // read queries
    std::ifstream qf(QUERIES);
    if(!qf){
        std::cerr << "Cannot open queries file: " << QUERIES << "\n";
        return 3;
    }
    std::vector<std::pair<int,int>> queries;
    int a,b;
    while(qf >> a >> b){
        queries.emplace_back(a-1, b-1);
    }
    qf.close();

    if((int)queries.size() != q){
        std::cerr << "Warning: Q in shares header = " << q << " but queries.txt has " << queries.size() << " entries\n";
    }
    for(std::size_t t = 0; t < queries.size(); ++t){
        int ii = queries[t].first;
        int jj = queries[t].second;
        if(ii < 0 || ii >= m || jj < 0 || jj >= n){
            std::cerr << "Query index out of bound at t=" << t << " pair=(" << ii << "," << jj << ")\n";
            return 4;
        }
        long long dot = 0;
        for(int r = 0; r < k; ++r){
            dot += (long long)U_plain[ii][r] * (long long)V_plain[jj][r];
            dot %= MOD;
        }
        long long delta = (1 - dot) % MOD;
        if(delta < 0) delta += MOD;
        for(int r = 0; r < k; ++r){
            long long add = ((long long)V_plain[jj][r] * delta) % MOD;
            U_plain[ii][r] = (int)modnorm_ll((long long)U_plain[ii][r] + add);
        }
    }

    int mf,nf,kf,qf_h;
    matrix U0_final, V0_final;
    parse_shares_file(P0_FINAL, mf, nf, kf, qf_h, U0_final, V0_final);
    int mf2,nf2,kf2,qf_h2;
    matrix U1_final, V1_final;
    parse_shares_file(P1_FINAL, mf2, nf2, kf2, qf_h2, U1_final, V1_final);

    if(!(mf==mf2 && nf==nf2 && kf==kf2)){
        std::cerr << "Dimension mismatch in final share files\n";
        return 5;
    }

    matrix U_recon(mf, ivec(kf,0));
    for(int i = 0; i < mf; ++i){
        for(int r = 0; r < kf; ++r){
            U_recon[i][r] = (int)modnorm_ll((long long)U0_final[i][r] + (long long)U1_final[i][r]);
        }
    }


    bool ok = true;
    int max_show = 10;
    int shown = 0;
    for(int i = 0; i < m && ok; ++i){
        for(int r = 0; r < k; ++r){
            int expected = U_plain[i][r];
            int got = U_recon[i][r];
            if(expected != got){
                if(shown < max_show){
                    std::cerr << "Mismatch at U[" << i << "][" << r << "]: expected=" << expected << " got=" << got << "\n";
                    ++shown;
                }
                ok = false;
            }
        }
    }

    if(ok){
        std::cout << "CHECKER: PASS — final reconstructed U matches expected plaintext result.\n";
        return 0;
    } else {
        std::cerr << "CHECKER: FAIL — mismatches detected. Shown up to " << max_show << " entries above.\n";
        return 6;
    }
}
