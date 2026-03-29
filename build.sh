#!/usr/bin/env bash
# ============================================================
#  build.sh — ISM HTC-HPC-Security
#  Builds all Docker images and starts the full stack.
#
#  Usage:
#    chmod +x build.sh
#    ./build.sh          # build + start (detached)
#    ./build.sh stop     # stop all containers
#    ./build.sh logs     # tail all logs
#    ./build.sh clean    # stop + remove volumes
# ============================================================

set -euo pipefail

COMPOSE="docker compose"
# Fall back to legacy docker-compose if needed
command -v docker &>/dev/null || { echo "Docker not found"; exit 1; }
$COMPOSE version &>/dev/null || COMPOSE="docker-compose"

case "${1:-up}" in

  up)
    echo "==> Building images..."
    $COMPOSE build --parallel

    echo "==> Starting containers..."
    $COMPOSE up -d

    echo ""
    echo "========================================="
    echo "  Stack is up!"
    echo "  Frontend  →  http://localhost:8080"
    echo "  RabbitMQ  →  http://localhost:15672"
    echo "             (user: admin / pass: admin)"
    echo "  C05 API   →  http://localhost:3000"
    echo "  MySQL     →  localhost:3306"
    echo "  MongoDB   →  localhost:27017"
    echo "========================================="
    ;;

  stop)
    echo "==> Stopping containers..."
    $COMPOSE stop
    ;;

  logs)
    $COMPOSE logs -f
    ;;

  clean)
    echo "==> Stopping and removing all volumes..."
    $COMPOSE down -v --remove-orphans
    ;;

  rebuild)
    # Full teardown + rebuild — useful after source changes
    $COMPOSE down --remove-orphans
    $COMPOSE build --no-cache --parallel
    $COMPOSE up -d
    ;;

  *)
    echo "Unknown command: $1"
    echo "Usage: ./build.sh [up|stop|logs|clean|rebuild]"
    exit 1
    ;;
esac
