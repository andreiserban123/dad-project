FROM critoma/amd64_u24_noble_ism_security AS native-builder

# Install build tools for the C/MPI native layer
RUN apt-get update && apt-get install -y \
    openmpi-bin \
    libopenmpi-dev \
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

RUN apt-get update && apt-get install -y openjdk-21-jdk maven && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY pom.xml .
COPY src ./src
RUN mvn clean package -DskipTests -q

# ── Runtime stage ───────────────────────────────
FROM critoma/amd64_u24_noble_ism_security

RUN apt-get update && apt-get install -y \
    openjdk-21-jre \
    openmpi-bin \
    libopenmpi3 \
    libssl3 \
    openssh-client \
    openssh-server \
    && rm -rf /var/lib/apt/lists/*

# SSH needed for MPI multi-node communication
RUN mkdir /var/run/sshd && \
    echo 'root:mpipass' | chpasswd && \
    sed -i 's/#PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed -i 's/UsePAM yes/UsePAM no/' /etc/ssh/sshd_config

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
