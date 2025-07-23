import py_coro_rpc

def hello(str):
    return str

def handle_request(data: str) -> str:
    return f"Processed: {data}"

print(py_coro_rpc.hello())
# server = py_coro_rpc.coro_rpc_server(2, "0.0.0.0:9004", 10)
# r = server.async_start()
# print("start result", r)
# server.stop()
rest = py_coro_rpc.rest_rpc_server(9004, 2)
rest.register_handler("handle_request", handle_request)
rest.register_handler("hello", hello)
rest.async_start()

client = py_coro_rpc.rest_rpc_client("127.0.0.1", 9004)
r = client.connect()
print("connected: ", r)
result = client.call("handle_request", "test")
print(result)
result = client.call("hello", "world")
print(result)