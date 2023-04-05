#ifndef __CLIENT_H_
#define __CLIENT_H_

#include "SocketEx.h"
#include <iostream>
#include <map>
#include <cstdlib>
#include <cassert>
#include <chrono>
#include <thread>

#ifdef DLL_EXPORT
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI __declspec(dllimport)
#endif // DLL_EXPORT

#ifdef __cplusplus
extern "C" {
#endif

DLLAPI extern int main();

#ifdef __cplusplus
}
#endif

#endif // !__CLIENT_H_
