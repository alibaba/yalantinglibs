#include "cuda.h"
namespace coro_io {
  inline const char* to_string(CUresult err) {
    const char* result;
    CUresult strerr = cuGetErrorString((CUresult)err,&result);
    if (strerr!=CUDA_SUCCESS) {
        return "unknown error";
    }
    else {
        return result;
    }
  }
}