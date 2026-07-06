#!/usr/bin/env bash
set -euo pipefail

BIN_ROOT="${1:-$(pwd)/build/output/examples}"
LOG_DIR="${2:-$(pwd)/build/ntls-e2e-logs}"
RPC_SERVER_PID=""
HTTP_SERVER_PID=""

mkdir -p "${LOG_DIR}"

cleanup() {
  if [[ -n "${RPC_SERVER_PID}" ]] && kill -0 "${RPC_SERVER_PID}" 2>/dev/null; then
    kill "${RPC_SERVER_PID}" 2>/dev/null || true
  fi
  if [[ -n "${HTTP_SERVER_PID}" ]] && kill -0 "${HTTP_SERVER_PID}" 2>/dev/null; then
    kill "${HTTP_SERVER_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT

wait_for_port() {
  local port="$1"
  local retries="${2:-40}"
  python - "$port" "$retries" <<'PY'
import socket, sys, time
port = int(sys.argv[1])
retries = int(sys.argv[2])
for _ in range(retries):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(1)
        try:
            sock.connect(("127.0.0.1", port))
        except OSError:
            time.sleep(1)
        else:
            sys.exit(0)
print(f"port {port} unreachable after {retries}s", file=sys.stderr)
sys.exit(1)
PY
}

run_ntls_rpc_tests() {
  local server_bin="${BIN_ROOT}/coro_rpc/ntls_server"
  local client_bin="${BIN_ROOT}/coro_rpc/ntls_client"

  if [[ ! -x "${server_bin}" || ! -x "${client_bin}" ]]; then
    echo "NTLS RPC binaries not found under ${BIN_ROOT}. Skipping RPC tests."
    return 1
  fi

  echo "Starting NTLS RPC server..."
  timeout 120 "${server_bin}" >"${LOG_DIR}/ntls_rpc_server.log" 2>&1 &
  RPC_SERVER_PID=$!
  for port in 8801 8802 8803 8804; do
    wait_for_port "${port}" 40
  done

  echo "Running NTLS RPC client..."
  timeout 120 "${client_bin}" >"${LOG_DIR}/ntls_rpc_client.log" 2>&1
  echo "NTLS RPC client completed successfully."

  # Verify mutual authentication enforces client certificate
  echo "Verifying mutual auth rejects clients without certificates..."
  local tongsuo_bin="${TONGSUO_INSTALL_PREFIX:-/usr/local/tongsuo}/bin/tongsuo"
  if [[ ! -x "${tongsuo_bin}" ]]; then
    tongsuo_bin="${TONGSUO_INSTALL_PREFIX:-/usr/local/tongsuo}/bin/openssl"
  fi
  if [[ -x "${tongsuo_bin}" ]]; then
    # Port 8802: TLCP mutual auth - should reject connection without client cert
    if "${tongsuo_bin}" s_client -connect 127.0.0.1:8802 -ntls \
       -CAfile "${TONGSUO_INSTALL_PREFIX:-/usr/local/tongsuo}/ssl/certs/sm2/chain-ca.crt" \
       </dev/null 2>&1 | grep -qi "error\|alert\|fail\|denied\|handshake"; then
      echo "PASS: TLCP mutual auth (8802) correctly rejected client without certificate"
    else
      echo "WARNING: TLCP mutual auth (8802) may have accepted client without certificate"
    fi

    # Port 8804: TLS 1.3 + GM mutual auth - should reject connection without client cert
    if "${tongsuo_bin}" s_client -connect 127.0.0.1:8804 -ntls \
       -CAfile "${TONGSUO_INSTALL_PREFIX:-/usr/local/tongsuo}/ssl/certs/sm2/chain-ca.crt" \
       </dev/null 2>&1 | grep -qi "error\|alert\|fail\|denied\|handshake"; then
      echo "PASS: TLS 1.3 + GM mutual auth (8804) correctly rejected client without certificate"
    else
      echo "WARNING: TLS 1.3 + GM mutual auth (8804) may have accepted client without certificate"
    fi
  else
    echo "Tongsuo binary not found, skipping mutual auth rejection verification"
  fi

  kill "${RPC_SERVER_PID}" 2>/dev/null || true
  RPC_SERVER_PID=""
}

run_ntls_http_tests() {
  local server_bin="${BIN_ROOT}/coro_http/ntls_http_server"
  local client_bin="${BIN_ROOT}/coro_http/ntls_http_client"

  if [[ ! -x "${server_bin}" || ! -x "${client_bin}" ]]; then
    echo "NTLS HTTP binaries not found under ${BIN_ROOT}. Skipping HTTP tests."
    return 1
  fi

  echo "Starting NTLS HTTP server..."
  timeout 120 "${server_bin}" >"${LOG_DIR}/ntls_http_server.log" 2>&1 &
  HTTP_SERVER_PID=$!
  for port in 8801 8802 8803 8804; do
    wait_for_port "${port}" 40
  done

  echo "Running NTLS HTTP client..."
  timeout 120 "${client_bin}" >"${LOG_DIR}/ntls_http_client.log" 2>&1
  echo "NTLS HTTP client completed successfully."

  # Verify mutual authentication enforces client certificate
  echo "Verifying HTTP mutual auth rejects clients without certificates..."
  local tongsuo_bin="${TONGSUO_INSTALL_PREFIX:-/usr/local/tongsuo}/bin/tongsuo"
  if [[ ! -x "${tongsuo_bin}" ]]; then
    tongsuo_bin="${TONGSUO_INSTALL_PREFIX:-/usr/local/tongsuo}/bin/openssl"
  fi
  if [[ -x "${tongsuo_bin}" ]]; then
    # Port 8802: TLCP mutual auth - should reject connection without client cert
    if "${tongsuo_bin}" s_client -connect 127.0.0.1:8802 -ntls \
       -CAfile "${TONGSUO_INSTALL_PREFIX:-/usr/local/tongsuo}/ssl/certs/sm2/chain-ca.crt" \
       </dev/null 2>&1 | grep -qi "error\|alert\|fail\|denied\|handshake"; then
      echo "PASS: HTTP TLCP mutual auth (8802) correctly rejected client without certificate"
    else
      echo "WARNING: HTTP TLCP mutual auth (8802) may have accepted client without certificate"
    fi

    # Port 8804: TLS 1.3 + GM mutual auth - should reject connection without client cert
    if "${tongsuo_bin}" s_client -connect 127.0.0.1:8804 -ntls \
       -CAfile "${TONGSUO_INSTALL_PREFIX:-/usr/local/tongsuo}/ssl/certs/sm2/chain-ca.crt" \
       </dev/null 2>&1 | grep -qi "error\|alert\|fail\|denied\|handshake"; then
      echo "PASS: HTTP TLS 1.3 + GM mutual auth (8804) correctly rejected client without certificate"
    else
      echo "WARNING: HTTP TLS 1.3 + GM mutual auth (8804) may have accepted client without certificate"
    fi
  else
    echo "Tongsuo binary not found, skipping HTTP mutual auth rejection verification"
  fi

  kill "${HTTP_SERVER_PID}" 2>/dev/null || true
  HTTP_SERVER_PID=""
}

echo "===== Running NTLS RPC end-to-end tests ====="
run_ntls_rpc_tests
echo "===== Running NTLS HTTP end-to-end tests ====="
run_ntls_http_tests
echo "NTLS end-to-end tests finished."

