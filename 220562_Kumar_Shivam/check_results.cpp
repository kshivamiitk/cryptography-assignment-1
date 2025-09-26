#include <bits/stdc++.h>
#include "shares.h"

int main(){
    const std::string p0_orig = "party0_shares.orig";
    const std::string p1_orig = "party1_shares.orig";
    const std::string p0_final = "party0_shares.txt";
    const std::string p1_final = "party1_shares.txt";
    const std::string queries_file = "queries_i.txt";
    const std::string out_file = "final_reconstructed_UV.txt";

    party_shares p0o, p1o;
    p0o.read_from_file(p0_orig);
    p1o.read_from_file(p1_orig);

    int m = (int)p0o.U.size();
    int n = (int)p0o.V.size();
    int k = (m>0 ? (int)p0o.U[0].size() : 0);
    int q = (int)p0o.dot_c.size();


    matrix U_plain(m, vec(k));
    matrix V_plain(n, vec(k));
    for(int i=0;i<m;++i) for(int r=0;r<k;++r) U_plain[i][r] = modadd(p0o.U[i][r], p1o.U[i][r]);
    for(int i=0;i<n;++i) for(int r=0;r<k;++r) V_plain[i][r] = modadd(p0o.V[i][r], p1o.V[i][r]);

 
    std::vector<int> j_indices(q, -1);
    for(int t=0;t<q;++t){
        int found=-1, cnt=0;
        for(int j=0;j<n;++j){
            int v = modadd(p0o.j_shares[t][j], p1o.j_shares[t][j]);
            if(v == 1){ found=j; ++cnt; }
            else if(v != 0){
                std::cerr << "[checker] warning: j_shares summed to non-binary value at t=" << t << " j=" << j << " val=" << v << "\n";
            }
        }
        std::cerr << "[checker] query " << t << " reconstructed ones=" << cnt << " idx=" << found << "\n";
        j_indices[t]=found;
    }


    std::ifstream qf(queries_file);
    if(!qf){ std::cerr << "cannot open " << queries_file << "\n"; return 2; }
    std::vector<int> queries_i;
    int tmpi;
    while(qf >> tmpi) queries_i.push_back(tmpi-1);
    qf.close();

    size_t nq = std::min((size_t)q, queries_i.size());


    matrix U_expected = U_plain;
    for(size_t t=0; t<nq; ++t){
        int ii = queries_i[t];
        int jj = j_indices[t];
        if(ii<0||ii>=m||jj<0||jj>=n){ std::cerr << "[checker] skipping bad query t=" << t << "\n"; continue; }
        long long dot = 0;
        for(int r=0;r<k;++r){ dot += (long long)U_expected[ii][r] * (long long)V_plain[jj][r]; dot %= MOD; }
        long long delta = (1 - dot) % MOD; if(delta < 0) delta += MOD;
        for(int r=0;r<k;++r){
            long long add = ((long long)V_plain[jj][r] * delta) % MOD;
            U_expected[ii][r] = (int)modnorm((long long)U_expected[ii][r] + add);
        }
    }


    party_shares p0f, p1f;
    p0f.read_from_file(p0_final);
    p1f.read_from_file(p1_final);
    matrix U_recon(m, vec(k));
    for(int i=0;i<m;++i) for(int r=0;r<k;++r) U_recon[i][r] = modadd(p0f.U[i][r], p1f.U[i][r]);


    std::ofstream ofs(out_file);
    if(!ofs){ std::cerr << "cannot open " << out_file << " for write\n"; }
    ofs << "U_final\n";
    for(int i=0;i<m;++i){
        for(int r=0;r<k;++r) ofs << U_recon[i][r] << (r+1<k ? " " : "");
        ofs << "\n";
    }
    ofs << "V_final\n";
    for(int i=0;i<n;++i){
        for(int r=0;r<k;++r) ofs << V_plain[i][r] << (r+1<k ? " " : "");
        ofs << "\n";
    }
    ofs.close();


    bool ok = true;
    for(int i=0;i<m;++i){
        for(int r=0;r<k;++r){
            if(U_expected[i][r] != U_recon[i][r]){
                std::cerr << "Mismatch U[" << i << "][" << r << "]: expected=" << U_expected[i][r] << " got=" << U_recon[i][r] << "\n";
                ok = false;
            }
        }
    }

    if(ok){
        std::cout << "CHECKER: PASS — final_reconstructed_UV.txt written.\n";
        return 0;
    } else {
        std::cerr << "CHECKER: FAIL — final_reconstructed_UV.txt written.\n";
        return 3;
    }
}
