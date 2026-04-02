FROM critoma/amd64_u24_noble_ism_security AS builder

ENV JAVA_HOME=/opt/software/java/jdks/jdk-21.0.2
ENV PATH=$JAVA_HOME/bin:$PATH

# Maven is not pre-installed; JDK comes from /opt/software
RUN apt-get update && apt-get install -y maven && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY pom.xml .
COPY src ./src

# Build fat JAR
RUN mvn clean package -DskipTests -q

# ── Runtime stage ──────────────────────────────
FROM critoma/amd64_u24_noble_ism_security

ENV JAVA_HOME=/opt/software/java/jdks/jdk-21.0.2
ENV PATH=$JAVA_HOME/bin:$PATH

WORKDIR /app
COPY --from=builder /build/target/*.jar app.jar

EXPOSE 8080
ENTRYPOINT ["java", "-jar", "app.jar"]