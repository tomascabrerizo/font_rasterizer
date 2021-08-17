@echo off

REM gcc main.c -Wall -Wextra -Werror -ggdb -o font -lm
clang main.c -Wall -Wextra -Werror -Wno-deprecated-declarations -g -o font.exe
