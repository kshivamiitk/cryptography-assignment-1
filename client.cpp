#include <bits/stdc++.h>
#include "shares.h"

int connect_to_server(const std::string &ip, int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){ perror("socket"); std::exit(1); }
    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    if(inet_pton(AF_INET, ip.c_str(), &serv.sin_addr) <= 0){ perror("inet_pton"); std::exit(1); }
    if(connect(sock, (sockaddr*)&serv, sizeof(serv)) < 0){ perror("connect"); std::exit(1); }
    return sock;
}

int main(int argc, char **argv){
    if(argc < 4){ std::cerr << "Usage: " << argv[0] << " <role 0|1> <server_ip> <server_port>\n"; return 1; }
    int role = std::atoi(argv[1]);
    std::string ip = argv[2];
    int port = std::atoi(argv[3]);
    std::string fname = (role == 0 ? "party0_shares.txt" : "party1_shares.txt");

    party_shares p;
    std::ifstream qf("queries.txt");
    if(!qf){ std::cerr << "cannot open the queries.txt\n"; return 1; }
    std::vector<std::pair<int,int>> queries;
    int qi, qj;
    while(qf >> qi >> qj) queries.emplace_back(qi-1, qj-1);
    qf.close();

    int sock = connect_to_server(ip, port);
    std::cerr << "[client " << role << "] connected to server " << ip << ":" << port << "\n";

    for(size_t t = 0; t < queries.size(); ++t){
        p.read_from_file(fname);
        if(p.U.empty()){ std::cerr << "[client " << role << "] empty U\n"; break; }
        int k = (int)p.U[0].size();
        int i = queries[t].first;
        int j = queries[t].second;
        vec ui = p.U[i];
        vec vj = p.V[j];
        vec a_share = p.dot_a[t];
        vec b_share = p.dot_b[t];
        int c_share = p.dot_c[t];

        vec alpha(k), beta(k);
        for(int r = 0; r < k; ++r){
            alpha[r] = modsub(ui[r], a_share[r]);
            beta[r]  = modsub(vj[r], b_share[r]);
        }

        std::string s = "ALPHA";
        for(auto &x : alpha) s += " " + std::to_string((long long)x);
        std::cerr << "[client " << role << "] sending ALPHA\n";
        send_line(sock, s);

        s = "BETA";
        for(auto &x : beta) s += " " + std::to_string((long long)x);
        std::cerr << "[client " << role << "] sending BETA\n";
        send_line(sock, s);

        std::string opened_alpha_line = recv_line(sock);
        std::string opened_beta_line  = recv_line(sock);
        if(opened_alpha_line.empty() || opened_beta_line.empty()){ std::cerr << "[client " << role << "] server closed or error\n"; break; }
        std::cerr << "[client " << role << "] received " << opened_alpha_line << "\n";
        std::cerr << "[client " << role << "] received " << opened_beta_line << "\n";

        vec opened_alpha_vec = parse_vec_from_line(opened_alpha_line);
        vec opened_beta_vec  = parse_vec_from_line(opened_beta_line);

        std::string rline = recv_line(sock);
        if(rline.empty()){ std::cerr << "[client " << role << "] missing rline\n"; break; }
        std::istringstream issr(rline);
        std::string tag;
        long long rshare_ll = 0;
        issr >> tag >> rshare_ll;
        int rshare = modnorm(rshare_ll);
        std::cerr << "[client " << role << "] received rshare tag=" << tag << " val=" << rshare << "\n";

        __int128 acc = 0;
        acc += (__int128)c_share;
        for(int r = 0; r < k; ++r){
            acc += (__int128)opened_alpha_vec[r] * (__int128)b_share[r];
            acc += (__int128)opened_beta_vec[r]  * (__int128)a_share[r];
        }
        acc += (__int128)rshare;
        int local_dot_share = modnorm((long long)(acc % (__int128)mod));
        int delta_share;
        if(role == 0) delta_share = modsub(1 % mod, local_dot_share);
        else delta_share = modsub(0, local_dot_share);

        vec sv_a_share = p.sv_a[t];
        int sv_b_share = p.sv_b[t];
        vec sv_c_share = p.sv_c[t];

        vec e_vec(k);
        for(int r = 0; r < k; ++r) e_vec[r] = modsub(vj[r], sv_a_share[r]);
        int f_scalar = modsub(delta_share, sv_b_share);

        s = "E_VEC";
        for(auto &x : e_vec) s += " " + std::to_string((long long)x);
        std::cerr << "[client " << role << "] sending E_VEC\n";
        send_line(sock, s);

        s = "F_SCAL " + std::to_string((long long)f_scalar);
        std::cerr << "[client " << role << "] sending F_SCAL\n";
        send_line(sock, s);

        std::string opened_e_line = recv_line(sock);
        std::string opened_f_line = recv_line(sock);
        if(opened_e_line.empty() || opened_f_line.empty()){ std::cerr << "[client " << role << "] server closed or error\n"; break; }
        std::cerr << "[client " << role << "] received " << opened_e_line << "\n";
        std::cerr << "[client " << role << "] received " << opened_f_line << "\n";

        vec opened_e = parse_vec_from_line(opened_e_line);
        std::istringstream issf(opened_f_line);
        std::string tagf;
        long long opened_f_ll = 0;
        issf >> tagf >> opened_f_ll;
        int opened_f = modnorm(opened_f_ll);

        std::string fe_line = recv_line(sock);
        if(fe_line.empty()){ std::cerr << "[client " << role << "] missing fe_line\n"; break; }
        vec fe_share = parse_vec_from_line(fe_line);

        vec product_share(k);
        for(int r = 0; r < k; ++r){
            int part = sv_c_share[r];
            part = modadd(part, modmul(opened_f, sv_a_share[r]));
            part = modadd(part, modmul(opened_e[r], sv_b_share));
            part = modadd(part, fe_share[r]);
            product_share[r] = part;
        }

        for(int r = 0; r < k; ++r) ui[r] = modadd(ui[r], product_share[r]);
        p.U[i] = ui;

        std::string tmpname = fname + ".tmp";
        p.write_to_file(tmpname);
        if(std::rename(tmpname.c_str(), fname.c_str()) != 0) { perror("rename"); }
        std::cerr << "[client " << role << "] finished query " << t << "\n";
    }

    close(sock);
    std::cerr << "[client " << role << "] done\n";
    return 0;
}
