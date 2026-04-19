@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "TARGET=parking_server.exe"
set "CXX=g++"
set "CXXFLAGS=-std=c++17 -Wall -Wextra -O2 -Iinclude"
set "LINKFLAGS=-lws2_32"

if "%~1"=="" goto all
if /I "%~1"=="all" goto all
if /I "%~1"=="run" goto run
if /I "%~1"=="clean" goto clean

echo Unknown target: %~1
exit /b 1

:all
%CXX% %CXXFLAGS% -o %TARGET% main.cpp %LINKFLAGS%
if errorlevel 1 exit /b 1
echo.
echo Build successful ^> %TARGET%
exit /b 0

:run
call "%~f0" all
if errorlevel 1 exit /b 1
%TARGET%
exit /b %errorlevel%

:clean
if exist %TARGET% del /f /q %TARGET%
exit /b 0