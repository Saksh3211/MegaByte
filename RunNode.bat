@echo off
REM
cd /d %~dp0

REM 
call .venv\Scripts\activate.bat

REM 
python node\main.py

REM 
pause