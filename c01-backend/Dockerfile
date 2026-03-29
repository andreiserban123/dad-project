FROM critoma/amd64_u24_noble_ism_security AS builder

# Install Java 21 and Maven
RUN apt-get update && apt-get install -y \
    openjdk-21-jdk \
    maven \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY pom.xml .
COPY src ./src

# Build fat JAR
RUN mvn clean package -DskipTests -q

# ── Runtime stage ──────────────────────────────
FROM critoma/amd64_u24_noble_ism_security

RUN apt-get update && apt-get install -y openjdk-21-jre && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/target/*.jar app.jar

EXPOSE 8080
ENTRYPOINT ["java", "-jar", "app.jar"]
