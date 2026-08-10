FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    g++ \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt ./
COPY include/ include/
COPY src/ src/
COPY tests/ tests/

RUN mkdir -p build results \
 && cd build \
 && cmake .. -DCMAKE_BUILD_TYPE=Release \
 && cmake --build . -j$(nproc)

FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    python3 \
    bash \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/ModernPDE /app/bin/ModernPDE
COPY --from=builder /app/build/ModernPDE_Phase8 /app/bin/ModernPDE_Phase8
COPY --from=builder /app/build/tests/phase8_test /app/bin/phase8_test

COPY tests/ /app/tests/
COPY scripts/ /app/scripts/

RUN mkdir -p /app/results \
 && chmod +x /app/bin/ModernPDE /app/bin/ModernPDE_Phase8 /app/bin/phase8_test \
 && chmod +x /app/scripts/*.sh 2>/dev/null || true

ENV PATH="/app/bin:${PATH}"

CMD ["ModernPDE_Phase8", "/app/results/phase8_benchmark.txt"]
