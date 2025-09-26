#include <bits/stdc++.h>
#include "shares.h"

int main(int argc, char **argv){
    if(argc < 5){
        std::cerr << "Usage: " << argv[0] << " m n k q\n";
        return 1;
    }
    int m = std::stoi(argv[1]);
    int n = std::stoi(argv[2]);
    int k = std::stoi(argv[3]);
    int q = std::stoi(argv[4]);

    int val = 100;

    matrix U(m, vec(k));
    matrix V(n, vec(k));
    for(int i = 0; i < m; ++i) for(int j = 0; j < k; ++j) U[i][j] = modnorm((long long)rand_int(-val, val));
    for(int i = 0; i < n; ++i) for(int j = 0; j < k; ++j) V[i][j] = modnorm((long long)rand_int(-val, val));
    
    std::vector<int> queries_i(q), queries_j(q);
    for(int t = 0; t < q; ++t){
        queries_i[t] = rand_int(1, m); 
        queries_j[t] = rand_int(1, n);
    }

    party_shares p0, p1;
    p0.U.assign(m, vec(k,0));
    p1.U.assign(m, vec(k,0));
    p0.V.assign(n, vec(k,0));
    p1.V.assign(n, vec(k,0));

    for(int i = 0; i < m; ++i){
        for(int j = 0; j < k; ++j){
            int r = modnorm((long long)rand_int(-val, val));
            p0.U[i][j] = r;
            p1.U[i][j] = modsub(U[i][j], r);
        }
    }
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < k; ++j){
            int r = modnorm((long long)rand_int(-val, val));
            p0.V[i][j] = r;
            p1.V[i][j] = modsub(V[i][j], r);
        }
    }


    p0.dot_a.resize(q, vec(k));
    p0.dot_b.resize(q, vec(k));
    p0.dot_c.resize(q);
    p1.dot_a.resize(q, vec(k));
    p1.dot_b.resize(q, vec(k));
    p1.dot_c.resize(q);

    p0.sv_a.resize(q, vec(k));
    p0.sv_b.resize(q);
    p0.sv_c.resize(q);
    p1.sv_a.resize(q, vec(k));
    p1.sv_b.resize(q);
    p1.sv_c.resize(q);

    p0.sel_a.assign(q, matrix(n, vec(k)));
    p1.sel_a.assign(q, matrix(n, vec(k)));
    p0.sel_b.assign(q, vec(n, 0));
    p1.sel_b.assign(q, vec(n, 0));
    p0.sel_c.assign(q, matrix(n, vec(k)));
    p1.sel_c.assign(q, matrix(n, vec(k)));

    p0.j_shares.assign(q, vec(n,0));
    p1.j_shares.assign(q, vec(n,0));

    for(int t = 0; t < q; ++t){
        vec a(k), b(k);
        for(int r = 0; r < k; ++r){ a[r] = modnorm((long long)rand_int(-val, val)); b[r] = modnorm((long long)rand_int(-val, val)); }
        __int128 acc = 0;
        for(int r = 0; r < k; ++r) acc += (__int128)a[r] * (__int128)b[r];
        int c = modnorm((long long)(acc % (__int128)MOD));
        vec a0(k), a1(k), b0(k), b1(k);
        for(int r = 0; r < k; ++r){
            int rr = modnorm((long long)rand_int(-val, val));
            a0[r] = rr; a1[r] = modsub(a[r], rr);
            rr = modnorm((long long)rand_int(-val, val));
            b0[r] = rr; b1[r] = modsub(b[r], rr);
        }
        int c0 = modnorm((long long)rand_int(-val, val));
        int c1 = modsub(c, c0);
        p0.dot_a[t] = a0; p1.dot_a[t] = a1;
        p0.dot_b[t] = b0; p1.dot_b[t] = b1;
        p0.dot_c[t] = c0; p1.dot_c[t] = c1;


        vec A(k);
        for(int r = 0; r < k; ++r) A[r] = modnorm((long long)rand_int(-val, val));
        int B = modnorm((long long)rand_int(-val, val));
        vec C(k);
        for(int r = 0; r < k; ++r) C[r] = modmul(B, A[r]);
        vec A0(k), A1(k), C0(k), C1(k);
        for(int r = 0; r < k; ++r){
            int rr = modnorm((long long)rand_int(-val, val));
            A0[r] = rr; A1[r] = modsub(A[r], rr);
            rr = modnorm((long long)rand_int(-val, val));
            C0[r] = rr; C1[r] = modsub(C[r], rr);
        }
        int B0 = modnorm((long long)rand_int(-val, val));
        int B1 = modsub(B, B0);
        p0.sv_a[t] = A0; p1.sv_a[t] = A1;
        p0.sv_b[t] = B0; p1.sv_b[t] = B1;
        p0.sv_c[t] = C0; p1.sv_c[t] = C1;


        for(int idx = 0; idx < n; ++idx){

            vec A_sel(k);
            for(int r = 0; r < k; ++r) A_sel[r] = modnorm((long long)rand_int(-val, val));
            int B_sel = modnorm((long long)rand_int(-val, val));
            vec C_sel(k);
            for(int r = 0; r < k; ++r) C_sel[r] = modmul(B_sel, A_sel[r]);

            vec A0sel(k), A1sel(k), C0sel(k), C1sel(k);
            for(int r = 0; r < k; ++r){
                int rr = modnorm((long long)rand_int(-val, val));
                A0sel[r] = rr; A1sel[r] = modsub(A_sel[r], rr);
                rr = modnorm((long long)rand_int(-val, val));
                C0sel[r] = rr; C1sel[r] = modsub(C_sel[r], rr);
            }
            int B0sel = modnorm((long long)rand_int(-val, val));
            int B1sel = modsub(B_sel, B0sel);
            p0.sel_a[t][idx] = A0sel; p1.sel_a[t][idx] = A1sel;
            p0.sel_b[t][idx] = B0sel; p1.sel_b[t][idx] = B1sel;
            p0.sel_c[t][idx] = C0sel; p1.sel_c[t][idx] = C1sel;
        }


        int jpos1 = queries_j[t];
        int jpos = jpos1 - 1;
        for(int idx = 0; idx < n; ++idx){
            int bit = (idx == jpos) ? 1 : 0;
            int r = modnorm((long long)rand_int(-val, val));
            p0.j_shares[t][idx] = r;
            p1.j_shares[t][idx] = modsub(bit, r);
        }
    }

    p0.write_to_file("party0_shares.txt");
    p1.write_to_file("party1_shares.txt");
    std::ofstream qf("queries_i.txt");
    for(int t = 0; t < q; ++t) qf << queries_i[t] << "\n";
    qf.close();

    std::cerr << "Generated party0_shares.txt and party1_shares.txt and queries_i.txt\n";
    return 0;
}
