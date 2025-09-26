#include <bits/stdc++.h>
#include <netdb.h>
#include "shares.h"


static int connect_to_server(const std::string &host, int port){
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char portstr[16];
    std::snprintf(portstr, sizeof(portstr), "%d", port);
    struct addrinfo *res = nullptr;
    int err = getaddrinfo(host.c_str(), portstr, &hints, &res);
    if(err != 0){
        std::cerr << "getaddrinfo(" << host << ") failed: " << gai_strerror(err) << "\n";
        std::exit(1);
    }
    int sock = -1;
    for(struct addrinfo *rp = res; rp != nullptr; rp = rp->ai_next){
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sock < 0) continue;
        if(connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);
    if(sock < 0){
        std::cerr << "connect failed to " << host << ":" << port << " (" << strerror(errno) << ")\n";
        std::exit(1);
    }
    return sock;
}

static vec compute_vj_from_sel_outputs(int n, int k, const std::vector<vec>& product_shares){
    vec res(k, 0);
    for(int idx = 0; idx < n; ++idx){
        for(int r = 0; r < k; ++r){
            res[r] = modadd(res[r], product_shares[idx][r]);
        }
    }
    return res;
}

int main(int argc, char **argv){
    if(argc < 4){
        std::cerr << "Usage: " << argv[0] << " <role 0|1> <server_host> <server_port>\n";
        return 1;
    }
    int role = std::atoi(argv[1]);
    std::string host = argv[2];
    int port = std::atoi(argv[3]);

    std::string fname = (role == 0 ? "party0_shares.txt" : "party1_shares.txt");

    party_shares p;
    std::ifstream qf("queries_i.txt");
    if(!qf){ std::cerr << "cannot open queries_i.txt\n"; return 1; }
    std::vector<int> queries_i;
    int qi;
    while(qf >> qi) queries_i.push_back(qi - 1);
    qf.close();

    int sock = connect_to_server(host, port);
    std::cerr << "[client " << role << "] connected to " << host << ":" << port << "\n";

    for(size_t t = 0; t < queries_i.size(); ++t){
        p.read_from_file(fname);

        if(p.U.empty()){ std::cerr << "[client " << role << "] empty U\n"; break; }
        int k = (int)p.U[0].size();
        int m = (int)p.U.size();
        int n = (int)p.V.size();
        int i = queries_i[t];
        if(i < 0 || i >= (int)p.U.size()){ std::cerr << "[client " << role << "] i out of range\n"; break; }
        if((int)p.j_shares.size() <= (int)t){ std::cerr << "[client " << role << "] missing j_shares for query " << t << "\n"; break; }

        vec d_share(n);
        std::vector<vec> e_share(n, vec(k));
        for(int idx = 0; idx < n; ++idx){
            d_share[idx] = modsub(p.j_shares[t][idx], p.sel_b[t][idx]);
            for(int r = 0; r < k; ++r){
                e_share[idx][r] = modsub(p.V[idx][r], p.sel_a[t][idx][r]);
            }
        }
   
        std::string s_d = "SEL_D";
        for(int x : d_share) s_d += " " + std::to_string((long long)x);
        send_line(sock, s_d);
       
        std::string s_e = "SEL_E";
        for(int idx = 0; idx < n; ++idx){
            for(int r = 0; r < k; ++r){
                s_e += " " + std::to_string((long long)e_share[idx][r]);
            }
        }
        send_line(sock, s_e);


        std::string opened_d_line = recv_line(sock);
        std::string opened_e_line = recv_line(sock);
        std::string fe_line = recv_line(sock);
        if(opened_d_line.empty() || opened_e_line.empty() || fe_line.empty()){
            std::cerr << "[client " << role << "] server closed during selection\n"; break;
        }
        vec opened_d = parse_vec_from_line(opened_d_line); 
        vec opened_e_flat = parse_vec_from_line(opened_e_line); 
        vec fe_flat = parse_vec_from_line(fe_line); 

        if((int)opened_d.size() != n){ std::cerr << "[client " << role << "] opened_d size mismatch\n"; break; }
        if((int)opened_e_flat.size() != n*k){ std::cerr << "[client " << role << "] opened_e_flat size mismatch\n"; break; }
        if((int)fe_flat.size() != n*k){ std::cerr << "[client " << role << "] fe_flat size mismatch\n"; break; }

        std::vector<vec> opened_e(n, vec(k));
        std::vector<vec> fe_share(n, vec(k));
        for(int idx = 0; idx < n; ++idx){
            for(int r = 0; r < k; ++r){
                int off = idx * k + r;
                opened_e[idx][r] = opened_e_flat[off];
                fe_share[idx][r] = fe_flat[off];
            }
        }


        std::vector<vec> product_shares(n, vec(k));
        for(int idx = 0; idx < n; ++idx){

            vec tmp = p.sel_c[t][idx];

            int od = opened_d[idx];
            for(int r = 0; r < k; ++r){
                int part = modmul(od, p.sel_a[t][idx][r]); 
                tmp[r] = modadd(tmp[r], part);
            }

            int bshare = p.sel_b[t][idx];
            for(int r = 0; r < k; ++r){
                int part = modmul(opened_e[idx][r], bshare);
                tmp[r] = modadd(tmp[r], part);
            }

            for(int r = 0; r < k; ++r){
                tmp[r] = modadd(tmp[r], fe_share[idx][r]);
            }
            product_shares[idx] = tmp;
        }


        vec vj_share(k, 0);
        for(int idx = 0; idx < n; ++idx){
            for(int r = 0; r < k; ++r) vj_share[r] = modadd(vj_share[r], product_shares[idx][r]);
        }

        std::cerr << "[client " << role << "] computed vj_share:";
        for(int x : vj_share) std::cerr << " " << x;
        std::cerr << "\n";


        vec ui = p.U[i];
        vec a_share = p.dot_a[t];
        vec b_share = p.dot_b[t];
        int c_share = p.dot_c[t];

        vec alpha(k), beta(k);
        for(int r = 0; r < k; ++r){
            alpha[r] = modsub(ui[r], a_share[r]);
            beta[r]  = modsub(vj_share[r], b_share[r]);
        }

        std::string s = "ALPHA";
        for(int x : alpha) s += " " + std::to_string((long long)x);
        send_line(sock, s);

        s = "BETA";
        for(int x : beta) s += " " + std::to_string((long long)x);
        send_line(sock, s);

        std::string opened_alpha_line = recv_line(sock);
        std::string opened_beta_line = recv_line(sock);
        if(opened_alpha_line.empty() || opened_beta_line.empty()){ std::cerr << "[client " << role << "] server closed\n"; break; }
        vec opened_alpha = parse_vec_from_line(opened_alpha_line);
        vec opened_beta = parse_vec_from_line(opened_beta_line);

        std::string rline = recv_line(sock);
        if(rline.empty()){ std::cerr << "[client " << role << "] missing rline\n"; break; }
        std::istringstream issr(rline);
        std::string tag; long long rshare_ll = 0; issr >> tag >> rshare_ll;
        int rshare = modnorm(rshare_ll);

        __int128 acc = 0;
        acc += (__int128)c_share;
        for(int r = 0; r < k; ++r){
            acc += (__int128)opened_alpha[r] * (__int128)b_share[r];
            acc += (__int128)opened_beta[r]  * (__int128)a_share[r];
        }
        acc += (__int128)rshare;
        int local_dot_share = modnorm((long long)(acc % (__int128)MOD));
        int delta_share = (role == 0 ? modsub(1 % MOD, local_dot_share) : modsub(0, local_dot_share));

        vec sv_a_share = p.sv_a[t];
        int sv_b_share = p.sv_b[t];
        vec sv_c_share = p.sv_c[t];


        vec e_vec(k);
        for(int r = 0; r < k; ++r) e_vec[r] = modsub(vj_share[r], sv_a_share[r]);
        int f_scalar = modsub(delta_share, sv_b_share);

        s = "E_VEC";
        for(int x : e_vec) s += " " + std::to_string((long long)x);
        send_line(sock, s);

        s = "F_SCAL " + std::to_string((long long)f_scalar);
        send_line(sock, s);

        std::string opened_E_line = recv_line(sock);
        std::string opened_f_line = recv_line(sock);
        if(opened_E_line.empty() || opened_f_line.empty()){ std::cerr << "[client " << role << "] server closed\n"; break; }
        vec opened_E = parse_vec_from_line(opened_E_line);
        std::istringstream issf(opened_f_line);
        std::string tagf; long long opened_f_ll = 0; issf >> tagf >> opened_f_ll;
        int opened_f = modnorm(opened_f_ll);

        std::string fe_line2 = recv_line(sock);
        if(fe_line2.empty()){ std::cerr << "[client " << role << "] missing fe_line\n"; break; }
        vec fe_share2 = parse_vec_from_line(fe_line2);

        vec product_share(k);
        for(int r = 0; r < k; ++r){
            int part = sv_c_share[r];
            part = modadd(part, modmul(opened_f, sv_a_share[r]));
            part = modadd(part, modmul(opened_E[r], sv_b_share));
            part = modadd(part, fe_share2[r]);
            product_share[r] = part;
        }

        for(int r = 0; r < k; ++r) ui[r] = modadd(ui[r], product_share[r]);
        p.U[i] = ui;

        std::string tmp = fname + ".tmp";
        p.write_to_file(tmp);
        if(std::rename(tmp.c_str(), fname.c_str()) != 0) perror("rename");
        std::cerr << "[client " << role << "] finished query " << t << "\n";
    }

    close(sock);
    std::cerr << "[client] done\n";
    return 0;
}
