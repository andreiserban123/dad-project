FROM critoma/amd64_u24_noble_ism_security AS builder

# OpenMPI is installed at /usr/local (compiled from /opt/software/openmpi-5.0.6 source)
# Only native build tools needed
RUN apt-get update && apt-get install -y \
    libssl-dev gcc make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/aes_enc.c .
COPY src/Makefile  .
RUN make

# ── Runtime ──────────────────────────────────────
FROM critoma/amd64_u24_noble_ism_security

# OpenMPI is at /usr/local; only SSL and SSH needed from apt
RUN apt-get update && apt-get install -y \
    libssl3 \
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
