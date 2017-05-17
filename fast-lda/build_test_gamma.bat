@echo off
icl /O3 /Wall /Z7 /std=c99 /DWIN32 /D_CRT_SECURE_NO_WARNINGS /c /Fotest-gamma.o test-gamma.c
icl /O3 /Wall /Z7 /std=c99 /DWIN32 /D_CRT_SECURE_NO_WARNINGS /c /Foutils.o utils.c
icl /O3 /Wall /Z7 /std=c99 /DWIN32 /D_CRT_SECURE_NO_WARNINGS /c /Fordtsc-helper.o rdtsc-helper.c

icl /O3 /Wall /Z7 /std=c99 /DWIN32 /D_CRT_SECURE_NO_WARNINGS test-gamma.o utils.o rdtsc-helper.o /Fe:test-gamma.exe