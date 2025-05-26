#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int ec;
  char *err_msg;
  uint64_t len;
} rpc_result;

typedef struct {
  int connect_timeout_sec;
  int req_timeout_sec;
  char *local_ip;
  bool enable_ib;
} client_config;

// ip:port
extern void load_service(void *ctx, uint64_t req_id);
extern void *response_msg(void *ctx, char *msg, uint64_t size);
extern void response_error(void *ctx, uint16_t err_code, const char *err_msg);
extern rpc_result wait_response_finish(void *p);
extern void *start_rpc_server(char *addr, int parallel, bool enable_ib);
extern void stop_rpc_server(void *server);

extern void *create_client_pool(char *addr, client_config conf);
extern void free_client_pool(void *pool);
extern rpc_result load(void *pool, uint64_t req_id, char *dest,
                       uint64_t dest_len);
/*
enum log_level {
  NONE = 0,
  TRACE,
  DEBUG,
  INFO,
  WARN,
  WARNING = WARN,
  ERROR,
  CRITICAL,
  FATAL = CRITICAL,
};
*/
extern void init_rpc_log(char *log_filename, int log_level,
                         uint64_t max_file_size, uint64_t max_file_num,
                         bool async);
extern void flush_rpc_log();

extern char *get_first_local_ip();

#ifdef __cplusplus
}
#endif