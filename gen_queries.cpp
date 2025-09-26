#include <bits/stdc++.h>
#include <fstream>
#include "shares.h"

int val = 100;

int main(int argc, char **argv){
    if(argc < 5){ std::cerr << "USAGE : ./gen_queries m n k q\n"; return 1; }
    int m = std::stoi(argv[1]);
    int n = std::stoi(argv[2]);
    int k = std::stoi(argv[3]);
    int q = std::stoi(argv[4]);

    matrix U(m, vec(k)), V(n, vec(k));
    for(int i = 0; i < m; ++i) for(int j = 0; j < k; ++j) U[i][j] = modnorm((long long)rand_int(-val, val));
    for(int i = 0; i < n; ++i) for(int j = 0; j < k; ++j) V[i][j] = modnorm((long long)rand_int(-val, val));

    std::vector<std::pair<int,int>> queries;
    for(int i = 0; i < q; ++i){
        int a = rand_int(1, m);
        int b = rand_int(1, n);
        queries.emplace_back(a, b);
    }

    party_shares p0, p1;
    p0.U.assign(m, vec(k,0));
    p0.V.assign(n, vec(k,0));
    p1.U.assign(m, vec(k,0));
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
    p0.sv_c.resize(q, vec(k));
    p1.sv_a.resize(q, vec(k));
    p1.sv_b.resize(q);
    p1.sv_c.resize(q, vec(k));

    for(int t = 0; t < q; ++t){
        vec a(k), b(k);
        for(int r = 0; r < k; ++r){ a[r] = modnorm((long long)rand_int(-val, val)); b[r] = modnorm((long long)rand_int(-val, val)); }
        __int128 acc = 0;
        for(int i1 = 0; i1 < k; ++i1) acc += (__int128)a[i1] * (__int128)b[i1];
        int c = modnorm((long long)(acc % (__int128)mod));
        vec a0(k), a1(k), b0(k), b1(k);
        for(int i1 = 0; i1 < k; ++i1){
            int rr = modnorm((long long)rand_int(-val, val));
            a0[i1] = rr;
            a1[i1] = modsub(a[i1], rr);
            rr = modnorm((long long)rand_int(-val, val));
            b0[i1] = rr;
            b1[i1] = modsub(b[i1], rr);
        }
        int c0 = modnorm((long long)rand_int(-val, val));
        int c1 = modsub(c, c0);
        p0.dot_a[t] = a0;
        p1.dot_a[t] = a1;
        p0.dot_b[t] = b0;
        p1.dot_b[t] = b1;
        p0.dot_c[t] = c0;
        p1.dot_c[t] = c1;

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
        p0.sv_a[t] = A0;
        p1.sv_a[t] = A1;
        p0.sv_b[t] = B0;
        p1.sv_b[t] = B1;
        p0.sv_c[t] = C0;
        p1.sv_c[t] = C1;
    }

    p0.write_to_file("party0_shares.txt");
    p1.write_to_file("party1_shares.txt");
    std::ofstream qf("queries.txt");
    for(auto &pr : queries) qf << pr.first << " " << pr.second << "\n";
    std::ofstream originalU("originalU.txt"); originalU << mat_to_string(U);
    std::ofstream originalV("originalV.txt"); originalV << mat_to_string(V);
    std::cerr << "Generated party0_shares.txt and party1_shares.txt and queries.txt\n";
    return 0;
}
