#pragma once

#include <cstdio>
#include <intrin.h>
#include <cstdlib>
#include <cstdint>
#include <locale>
#include <cmath>
#include <string>
#include <memory>
#include <algorithm>
#include <chrono>
#include <functional>
#include <vector>
#include <map>

using std::min;
using std::max;
using std::move;
using std::forward;
using std::vector;
using std::map;
using std::make_pair;
using std::string;
using std::wstring;
using std::function;
using std::shared_ptr;
using std::unique_ptr;



#define FREEGLUT_STATIC
#ifndef _DEBUG
#    define NDEBUG
#endif
#include <GL/freeglut.h>//Free GLUT Header