#pragma once

#ifdef RGF_LIBRARY
#define RGF_API __declspec(dllexport)
#else
#define RGF_API
#endif