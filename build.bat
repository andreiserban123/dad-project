@echo off
REM ============================================================
REM  build.bat — ISM HTC-HPC-Security (Windows)
REM  Usage: build.bat [up|stop|logs|clean|rebuild]
REM ============================================================

SET COMPOSE=docker compose

IF "%1"==""       GOTO up
IF "%1"=="up"     GOTO up
IF "%1"=="stop"   GOTO stop
IF "%1"=="logs"   GOTO logs
IF "%1"=="clean"  GOTO clean
IF "%1"=="rebuild" GOTO rebuild
GOTO usage

:up
echo =^> Building images...
%COMPOSE% build --parallel
echo =^> Starting containers...
%COMPOSE% up -d
echo.
echo =========================================
echo   Stack is up!
echo   Frontend  -^>  http://localhost:8080
echo   RabbitMQ  -^>  http://localhost:15672
echo              (admin / admin)
echo   C05 API   -^>  http://localhost:3000
echo =========================================
GOTO end

:stop
%COMPOSE% stop
GOTO end

:logs
%COMPOSE% logs -f
GOTO end

:clean
%COMPOSE% down -v --remove-orphans
GOTO end

:rebuild
%COMPOSE% down --remove-orphans
%COMPOSE% build --no-cache --parallel
%COMPOSE% up -d
GOTO end

:usage
echo Usage: build.bat [up^|stop^|logs^|clean^|rebuild]

:end
