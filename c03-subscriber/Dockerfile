FROM critoma/amd64_u24_noble_ism_security AS native-builder

# OpenMPI is installed at /usr/local (compiled from /opt/software/openmpi-5.0.6 source)
# Only native build tools needed
RUN apt-get update && apt-get install -y \
    libssl-dev \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /native
COPY native/aes_enc.c .
COPY native/Makefile   .
RUN make

# ── Java builder stage ──────────────────────────
FROM critoma/amd64_u24_noble_ism_security AS java-builder

ENV JAVA_HOME=/opt/software/java/jdks/jdk-21.0.2
ENV PATH=$JAVA_HOME/bin:$PATH

# Maven is not pre-installed; JDK comes from /opt/software
RUN apt-get update && apt-get install -y maven && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY pom.xml .
COPY src ./src
RUN mvn clean package -DskipTests -q

# ── Runtime stage ───────────────────────────────
FROM critoma/amd64_u24_noble_ism_security

ENV JAVA_HOME=/opt/software/java/jdks/jdk-21.0.2
ENV PATH=$JAVA_HOME/bin:$PATH

# Java comes from /opt/software; OpenMPI is at /usr/local; only SSL and SSH needed from apt
RUN apt-get update && apt-get install -y \
    libssl3 \
    openssh-client \
    openssh-server \
    && rm -rf /var/lib/apt/lists/*

# SSH needed for MPI multi-node communication
RUN mkdir /var/run/sshd && \
    echo 'root:mpipass' | chpasswd && \
    sed -i 's/#PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed -i 's/UsePAM yes/UsePAM no/' /etc/ssh/sshd_config

# MPI SSH key pair (passwordless auth between nodes)
COPY mpi_rsa     /root/.ssh/id_rsa
COPY mpi_rsa.pub /root/.ssh/id_rsa.pub
RUN mkdir -p /root/.ssh && \
    sed -i 's/\r$//' /root/.ssh/id_rsa /root/.ssh/id_rsa.pub && \
    cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys && \
    chmod 600 /root/.ssh/id_rsa /root/.ssh/authorized_keys && \
    chmod 700 /root/.ssh && \
    printf 'Host *\n  StrictHostKeyChecking no\n  UserKnownHostsFile /dev/null\n' > /root/.ssh/config

WORKDIR /app

# Copy native MPI binary
COPY --from=native-builder /native/aes_enc /opt/aes_enc
RUN chmod +x /opt/aes_enc

# Copy Spring Boot JAR
COPY --from=java-builder /build/target/*.jar app.jar

# MPI work directory (shared volume)
RUN mkdir -p /mpi-work

EXPOSE 8081 22

# Start SSH daemon + Spring Boot
CMD service ssh start && java -jar /app/app.jar
