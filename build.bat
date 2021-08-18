@echo off

REM gcc main.c -Wall -Wextra -Werror -ggdb -o font -lm
clang main.c -Wall -Wextra -Werror -Wno-deprecated-declarations -Wno-deprecated -g -o font.exe -lgdi32 -luser32 -lopengl32
