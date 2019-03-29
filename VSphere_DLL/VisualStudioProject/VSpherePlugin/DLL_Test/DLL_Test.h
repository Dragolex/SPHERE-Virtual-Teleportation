#pragma once

#include "stdafx.h"

typedef void(__stdcall *func)();
typedef bool(__stdcall *func_bool)();
typedef int(__stdcall *func_int)();
typedef void(__stdcall *func_arg_int_int)(int, int);
typedef void(__stdcall *func_arg_bool_int)(bool, int);
typedef int(__stdcall *func_int_arg_9int)(int, int, int, int, int, int, int, int, int);
typedef void(__stdcall *func_arg_int_str_int)(int, const char*, int);
typedef void(__stdcall *func_arg_intptrptr_intptr)(int**, int*);

int main();

string computeRootPath();
