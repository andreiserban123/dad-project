# ISM HTC-HPC-Security — AES Parallel Picture Encryption

## Architecture

```
C01 (Spring Boot + HTML frontend)
  │  REST POST /api/process  ──►  C02 (RabbitMQ)  ──►  C03 (Spring Boot MDB)
  │                                                          │
  │  WebSocket /topic/done  ◄────────────────────────────  │ Runtime.exec()
  │                                                          ▼
  │                                               C03+C04 (OpenMPI + OpenMP)
  │                                                    aes_enc (ELF64)
  │                                                          │
  │  GET /picture/:id  ◄──────────────────  C05 (Node.js + MySQL BLOB)
  ▼
Browser download
```

## Container Map

| Container      | Port  | Role                                      |
|----------------|-------|-------------------------------------------|
| c01-backend    | 8080  | Spring Boot REST + WebSocket + static HTML|
| c02-rabbitmq   | 15672 | RabbitMQ broker (management UI)           |
| c03-subscriber | 8081  | Spring Boot AMQP consumer + MPI launcher  |
| c04-mpi-worker | —     | OpenMPI worker node (SSH only)            |
| c05-nodejs     | 3000  | Node.js Express + MySQL + MongoDB         |
| mysql-db       | 3306  | Picture BLOB storage                      |
| mongo-db       | 27017 | SNMP metrics (optional)                  |

## Quick Start

```bash
# Linux / macOS
chmod +x build.sh
./build.sh

# Windows
build.bat
```

Then open **http://localhost:8080** in your browser.

## Manual Docker commands (alternative)

```bash
docker compose build --parallel
docker compose up -d
docker compose logs -f c03-subscriber   # watch MPI output
docker compose logs -f c01-backend       # watch notifications
```

## Testing the native MPI binary directly

```bash
# Enter the C03 container
docker exec -it c03-subscriber bash

# Run encryption standalone
mpirun -np 1 /opt/aes_enc encrypt CBC \
  000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f \
  /mpi-work/test.bmp /mpi-work/test_enc.bmp
```

## Key generation helper (OpenSSL)

```bash
# Generate a random 256-bit key as hex (64 chars)
openssl rand -hex 32
```

## Project structure

```
aes-docker/
├── docker-compose.yml
├── build.sh / build.bat
├── shared/
│   └── init.sql               ← MySQL table creation
├── c01-backend/
│   ├── Dockerfile
│   ├── pom.xml
│   └── src/main/java/com/ism/aes/
│       ├── C01Application.java
│       ├── config/
│       │   ├── RabbitConfig.java
│       │   └── WebSocketConfig.java
│       ├── controller/
│       │   └── PictureController.java
│       └── messaging/
│           └── NotificationService.java
├── c01-frontend/
│   └── index.html             ← Upload UI + WebSocket client
├── c02-rabbitmq/              ← No custom Dockerfile (uses official image)
├── c03-subscriber/
│   ├── Dockerfile
│   ├── pom.xml
│   ├── native/
│   │   ├── aes_enc.c          ← OpenMPI + OpenMP + OpenSSL AES
│   │   └── Makefile
│   └── src/main/java/com/ism/subscriber/
│       ├── C03Application.java
│       ├── config/RabbitConfig.java
│       └── messaging/PictureSubscriber.java
├── c04-mpi/
│   ├── Dockerfile
│   └── src/                   ← Same aes_enc.c as C03
└── c05-nodejs/
    ├── Dockerfile
    ├── package.json
    └── src/index.js           ← Express REST + MySQL + MongoDB
```

## ZIP naming rule

```
ISM_HTC-HPC-Security_SURNAME_FirstName1_FirstName2.zip
```
