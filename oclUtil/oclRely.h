#pragma once

#include <memory>
#include <vector>
#include <string>
#include <Windows.h>


#define USING_INTEL
#define USING_NVIDIA

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL\opencl.h>
#ifdef USING_INTEL
#    ifdef _WIN64
#        pragma comment(lib, "\\Programs\\Intel\\OpenCL SDK\\lib\\x64\\opencl.lib")
#    else
#        pragma comment(lib,"\\Programs\\Intel\\OpenCL SDK\\lib\\x86\\opencl.lib")
#    endif
#elif defined(USING_NVIDIA)
#    ifdef _WIN64
#        pragma comment(lib, "\\Programs\\CUDA\\lib\\x64\\opencl.lib")
#    else
#        pragma comment(lib,"\\Programs\\CUDA\\lib\\Win32\\opencl.lib")
#    endif
#endif
