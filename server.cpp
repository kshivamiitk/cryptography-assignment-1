#include <bits/stdc++.h>
#include "shares.h"

int create_server_socket(int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){ perror("socket"); std::exit(1); }
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { perror("setsockopt"); std::exit(1); }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); std::exit(1); }
    if(listen(sock, 5) < 0){ perror("listen"); std::exit(1); }
    std::cerr << "[server] listening on port " << port << "\n";
    return sock;
}

int main(int argc, char **argv){
    int port = 9000;
    if(argc >= 2) port = std::atoi(argv[1]);
    int lsock = create_server_socket(port);
    sockaddr_in cli{};
    socklen_t len;
    len = sizeof(cli);
    int c1 = accept(lsock, (sockaddr*)&cli, &len);
    if(c1 < 0){ perror("accept c1"); std::exit(1); }
    std::cerr << "[server] accepted first client\n";
    len = sizeof(cli);
    int c2 = accept(lsock, (sockaddr*)&cli, &len);
    if(c2 < 0){ perror("accept c2"); std::exit(1); }
    std::cerr << "[server] accepted second client\n";

    while(true){
        std::string a1 = recv_line(c1);
        std::string a2 = recv_line(c2);
        if(a1.empty() || a2.empty()) break;
        std::cerr << "[server] received from c1: " << a1 << "\n";
        std::cerr << "[server] received from c2: " << a2 << "\n";
        vec alpha1 = parse_vec_from_line(a1);
        vec alpha2 = parse_vec_from_line(a2);

        std::string b1 = recv_line(c1);
        std::string b2 = recv_line(c2);
        if(b1.empty() || b2.empty()) break;
        std::cerr << "[server] received from c1: " << b1 << "\n";
        std::cerr << "[server] received from c2: " << b2 << "\n";
        vec beta1 = parse_vec_from_line(b1);
        vec beta2 = parse_vec_from_line(b2);

        if(alpha1.size() != alpha2.size()){
            std::cerr << "[server] alpha sizes mismatch\n";
            break;
        }
        int k = (int)alpha1.size();
        vec alpha(k), beta(k);
        for(int t = 0; t < k; ++t){
            alpha[t] = modadd(alpha1[t], alpha2[t]);
            beta[t]  = modadd(beta1[t], beta2[t]);
        }

        __int128 acc = 0;
        for(int t = 0; t < k; ++t) acc += (__int128)alpha[t] * (__int128)beta[t];
        int alpha_beta = modnorm((long long)(acc % (__int128)mod));
        int r0 = modnorm((long long)rand_int(-1000000,1000000));
        int r1 = modsub(alpha_beta, r0);

        std::string salpha = "OPENED_ALPHA";
        for(auto &x : alpha) salpha += " " + std::to_string((long long)x);
        send_line(c1, salpha); send_line(c2, salpha);
        std::cerr << "[server] sent OPENED_ALPHA\n";

        std::string sbeta = "OPENED_BETA";
        for(auto &x : beta) sbeta += " " + std::to_string((long long)x);
        send_line(c1, sbeta); send_line(c2, sbeta);
        std::cerr << "[server] sent OPENED_BETA\n";

        std::string sab1 = "ALPHA_BETA_R " + std::to_string((long long)r0);
        std::string sab2 = "ALPHA_BETA_R " + std::to_string((long long)r1);
        send_line(c1, sab1); send_line(c2, sab2);
        std::cerr << "[server] sent ALPHA_BETA_R shares\n";

        std::string e1_line = recv_line(c1);
        std::string e2_line = recv_line(c2);
        if(e1_line.empty() || e2_line.empty()) break;
        std::cerr << "[server] received E from c1: " << e1_line << "\n";
        std::cerr << "[server] received E from c2: " << e2_line << "\n";
        vec e1 = parse_vec_from_line(e1_line);
        vec e2 = parse_vec_from_line(e2_line);

        std::string f1_line = recv_line(c1);
        std::string f2_line = recv_line(c2);
        if(f1_line.empty() || f2_line.empty()) break;
        std::cerr << "[server] received F from c1: " << f1_line << "\n";
        std::cerr << "[server] received F from c2: " << f2_line << "\n";
        int f1 = parse_scalar_from_line(f1_line);
        int f2 = parse_scalar_from_line(f2_line);

        if(e1.size() != e2.size()){
            std::cerr << "[server] e sizes mismatch\n";
            break;
        }
        vec e(e1.size());
        for(size_t i = 0; i < e.size(); ++i) e[i] = modadd(e1[i], e2[i]);
        int f = modadd(f1, f2);

        vec fe(e.size());
        for(size_t t = 0; t < e.size(); ++t) fe[t] = modmul(f, e[t]);

        vec fe0(fe.size()), fe1(fe.size());
        for(size_t t = 0; t < fe.size(); ++t){
            int tmp = modnorm((long long)rand_int(-1000000,1000000));
            fe0[t] = tmp;
            fe1[t] = modsub(fe[t], tmp);
        }

        std::string se = "OPENED_E";
        for(auto &x : e) se += " " + std::to_string((long long)x);
        send_line(c1, se); send_line(c2, se);
        std::cerr << "[server] sent OPENED_E\n";

        std::string sf = "OPENED_F " + std::to_string((long long)f);
        send_line(c1, sf); send_line(c2, sf);
        std::cerr << "[server] sent OPENED_F\n";

        std::string o1 = "FE_VEC";
        for(auto &x : fe0) o1 += " " + std::to_string((long long)x);
        std::string o2 = "FE_VEC";
        for(auto &x : fe1) o2 += " " + std::to_string((long long)x);
        send_line(c1, o1); send_line(c2, o2);
        std::cerr << "[server] sent FE_VEC shares\n";
    }

    close(c1); close(c2); close(lsock);
    std::cerr << "[server] exiting\n";
    return 0;
}
