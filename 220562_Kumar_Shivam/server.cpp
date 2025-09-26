#include <bits/stdc++.h>
#include "shares.h"

int create_server_socket(int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){ perror("socket"); std::exit(1); }
    int opt = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){ perror("setsockopt"); std::exit(1); }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0){ perror("bind"); std::exit(1); }
    if(listen(sock, 5) < 0){ perror("listen"); std::exit(1); }
    std::cerr << "[server] listening on port " << port << "\n";
    return sock;
}

int main(int argc, char **argv){
    int port = 9000;

    if(argc >= 2) port = std::atoi(argv[1]);
    int lsock = create_server_socket(port);

    sockaddr_in cli{};
    socklen_t len = sizeof(cli);

    int c1 = accept(lsock, (sockaddr*)&cli, &len);
    if(c1 < 0){ perror("accept"); std::exit(1); }

    std::cerr << "[server] accepted first client\n";
    len = sizeof(cli);

    int c2 = accept(lsock, (sockaddr*)&cli, &len);
    if(c2 < 0){ perror("accept"); std::exit(1); }
    std::cerr << "[server] accepted second client\n";

    while(true){

        std::string sd1 = recv_line(c1);
        std::string sd2 = recv_line(c2);

        if(sd1.empty() || sd2.empty()) break;

        std::cerr << "[server] recv SEL_D from client1: " << sd1 << "\n";
        std::cerr << "[server] recv SEL_D from client2: " << sd2 << "\n";

        vec d1 = parse_vec_from_line(sd1);
        vec d2 = parse_vec_from_line(sd2);

        std::string se1 = recv_line(c1);
        std::string se2 = recv_line(c2);

        if(se1.empty() || se2.empty()) break;

        std::cerr << "[server] recv SEL_E from client1 (len=" << se1.size() << ")\n";
        std::cerr << "[server] recv SEL_E from client2 (len=" << se2.size() << ")\n";

        vec e1 = parse_vec_from_line(se1);
        vec e2 = parse_vec_from_line(se2);

        int n = (int)d1.size(); 
        if((int)d2.size() != n){ std::cerr << "[server] SEL_D size mismatch\n";
         break; }
        if(n == 0){ std::cerr << "[server] n==0\n"; 
        break; }

        if((int)e1.size() != (int)e2.size()){ std::cerr << "[server] SEL_E size mismatch\n"; 
        break; }

        int flat_len = (int)e1.size();
        if(flat_len % n != 0){ std::cerr << "[server] SEL_E flat length not divisible by n\n"; 
        break; }

        int k = flat_len / n;


        vec opened_d(n);
        std::vector<vec> opened_e(n, vec(k));

        for(int idx = 0; idx < n; ++idx){
            opened_d[idx] = modadd(d1[idx], d2[idx]);
            for(int j = 0; j < k; ++j){
                int off = idx * k + j;
                opened_e[idx][j] = modadd(e1[off], e2[off]);
            }
        }

        std::vector<vec> fe0(n, vec(k)), fe1(n, vec(k));
        for(int idx = 0; idx < n; ++idx){
            int alpha = opened_d[idx];
            for(int j = 0; j < k; ++j){
                int prod = modmul(alpha, opened_e[idx][j]);
                int r = modnorm((long long)rand_int(-1000000, 1000000));
                fe0[idx][j] = r;
                fe1[idx][j] = modsub(prod, r);
            }
        }

        std::string s_open_d = "OPENED_SEL_D";
        for(int x : opened_d) s_open_d += " " + std::to_string((long long)x);
        send_line(c1, s_open_d); send_line(c2, s_open_d);
        std::cerr << "[server] sent OPENED_SEL_D\n";

        std::string s_open_e = "OPENED_SEL_E";
        for(int idx = 0; idx < n; ++idx){
            for(int j = 0; j < k; ++j) s_open_e += " " + std::to_string((long long)opened_e[idx][j]);
        }

        send_line(c1, s_open_e); send_line(c2, s_open_e);
        std::cerr << "[server] sent OPENED_SEL_E\n";

        std::string s_fe0 = "SEL_FE_VEC";
        std::string s_fe1 = "SEL_FE_VEC";

        for(int idx = 0; idx < n; ++idx){
            for(int j = 0; j < k; ++j){
                s_fe0 += " " + std::to_string((long long)fe0[idx][j]);
                s_fe1 += " " + std::to_string((long long)fe1[idx][j]);
            }
        }
        send_line(c1, s_fe0); send_line(c2, s_fe1);
        std::cerr << "[server] sent SEL_FE_VEC shares\n";

        std::string a1 = recv_line(c1);
        std::string a2 = recv_line(c2);

        if(a1.empty() || a2.empty()) break;

        std::cerr << "[server] recv A1: " << a1 << "\n";
        std::cerr << "[server] recv A2: " << a2 << "\n";

        vec alpha1 = parse_vec_from_line(a1);
        vec alpha2 = parse_vec_from_line(a2);

        std::string b1 = recv_line(c1);
        std::string b2 = recv_line(c2);

        if(b1.empty() || b2.empty()) break;
        std::cerr << "[server] recv B1: " << b1 << "\n";
        std::cerr << "[server] recv B2: " << b2 << "\n";

        vec beta1 = parse_vec_from_line(b1);
        vec beta2 = parse_vec_from_line(b2);

        if(alpha1.size() != alpha2.size() || alpha1.size() != beta1.size() || beta1.size() != beta2.size()){
            std::cerr << "[server] size mismatch\n";
            break;
        }
        int k2 = (int)alpha1.size();
        vec alpha(k2), beta(k2);

        for(int t = 0; t < k2; ++t){
            alpha[t] = modadd(alpha1[t], alpha2[t]);
            beta[t] = modadd(beta1[t], beta2[t]);
        }

        __int128 acc = 0;
        for(int t = 0; t < k2; ++t) acc += (__int128)alpha[t] * (__int128)beta[t];
        int alpha_beta = modnorm((long long)(acc % (__int128)MOD));
        int r0 = modnorm((long long)rand_int(-1000000, 1000000));
        int r1 = modsub(alpha_beta, r0);

        std::string salpha = "OPENED_ALPHA";
        for(int x : alpha) salpha += " " + std::to_string((long long)x);
        send_line(c1, salpha); send_line(c2, salpha);
        std::cerr << "[server] sent OPENED_ALPHA\n";

        std::string sbeta = "OPENED_BETA";
        for(int x : beta) sbeta += " " + std::to_string((long long)x);
        send_line(c1, sbeta); send_line(c2, sbeta);
        std::cerr << "[server] sent OPENED_BETA\n";

        std::string sr1 = "ALPHA_BETA_R " + std::to_string((long long)r0);
        std::string sr2 = "ALPHA_BETA_R " + std::to_string((long long)r1);
        send_line(c1, sr1); send_line(c2, sr2);
        std::cerr << "[server] sent ALPHA_BETA_R shares\n";

        std::string e1_line = recv_line(c1);
        std::string e2_line = recv_line(c2);
        if(e1_line.empty() || e2_line.empty()) break;
        std::cerr << "[server] recv E1: " << e1_line << "\n";
        std::cerr << "[server] recv E2: " << e2_line << "\n";
        vec e1v = parse_vec_from_line(e1_line);
        vec e2v = parse_vec_from_line(e2_line);

        std::string f1_line = recv_line(c1);
        std::string f2_line = recv_line(c2);
        if(f1_line.empty() || f2_line.empty()) break;
        std::cerr << "[server] recv F1: " << f1_line << "\n";
        std::cerr << "[server] recv F2: " << f2_line << "\n";
        int f1 = parse_scalar_from_line(f1_line);
        int f2 = parse_scalar_from_line(f2_line);

        if(e1v.size() != e2v.size()){ std::cerr << "[server] e size mismatch\n"; 
        break; }
        vec e(e1v.size());
        for(size_t i = 0; i < e.size(); ++i) e[i] = modadd(e1v[i], e2v[i]);
        int f = modadd(f1, f2);

        vec fe(e.size());
        for(size_t t = 0; t < e.size(); ++t) fe[t] = modmul(f, e[t]);

        vec fe0vec(fe.size()), fe1vec(fe.size());
        for(size_t t = 0; t < fe.size(); ++t){
            int tmp = modnorm((long long)rand_int(-1000000, 1000000));
            fe0vec[t] = tmp;
            fe1vec[t] = modsub(fe[t], tmp);
        }

        std::string sopenE = "OPENED_E";
        for(int x : e) sopenE += " " + std::to_string((long long)x);
        send_line(c1, sopenE); send_line(c2, sopenE);
        std::cerr << "[server] sent OPENED_E\n";

        std::string sopenF = "OPENED_F " + std::to_string((long long)f);
        send_line(c1, sopenF); send_line(c2, sopenF);
        std::cerr << "[server] sent OPENED_F\n";

        std::string o1 = "FE_VEC";
        for(int x : fe0vec) o1 += " " + std::to_string((long long)x);
        std::string o2 = "FE_VEC";
        for(int x : fe1vec) o2 += " " + std::to_string((long long)x);
        send_line(c1, o1); send_line(c2, o2);
        std::cerr << "[server] sent FE_VEC shares\n";
    }

    close(c1); close(c2); close(lsock);
    std::cerr << "[server] exit\n";
    return 0;
}
