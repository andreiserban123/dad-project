FROM critoma/amd64_u24_noble_ism_security AS builder

RUN apt-get update && apt-get install -y \
    openmpi-bin libopenmpi-dev libssl-dev gcc make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/aes_enc.c .
COPY src/Makefile  .
RUN make

# ── Runtime ──────────────────────────────────────
FROM critoma/amd64_u24_noble_ism_security

RUN apt-get update && apt-get install -y \
    openmpi-bin libopenmpi3 libssl3 \
    openssh-server \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir /var/run/sshd && \
    echo 'root:mpipass' | chpasswd && \
    sed -i 's/#PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed -i 's/UsePAM yes/UsePAM no/' /etc/ssh/sshd_config

COPY --from=builder /build/aes_enc /opt/aes_enc
RUN chmod +x /opt/aes_enc

RUN mkdir -p /mpi-work

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
