
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive


RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    g++ \
    ca-certificates \
    net-tools \
    iproute2 \
    locales \
    wget \
    curl \
    git \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app


COPY . /app


RUN chmod +x ./run.sh || true

RUN set -eux; \
    g++ -std=c++17 -O2 gen_queries.cpp -o gen_queries; \
    g++ -std=c++17 -O2 server.cpp -o server; \
    g++ -std=c++17 -O2 client.cpp -o client; \
    g++ -std=c++17 -O2 check_results.cpp -o check_results; \
    true

EXPOSE 9000


ENTRYPOINT ["./run.sh"]

CMD ["3","3","3","1"]
