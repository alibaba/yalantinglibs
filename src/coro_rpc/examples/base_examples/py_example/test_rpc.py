import asyncio
import py_coro_rpc

def hello(con, str):
    print("conn id", con.conn_id())
    return str

async def async_hello(con, str):
    return str

def handle_request(con, data: str) -> str:
    print("conn id", con.conn_id())
    return f"Processed: {data}"

def on_done(task):
    print("任务已完成，结果是:", task.result())


# loop = asyncio.get_event_loop()
# loop.run_forever()
# server = py_coro_rpc.coro_rpc_server(2, "0.0.0.0:9004", 10)
# r = server.async_start()
# print("start result", r)
# server.stop()

rest = py_coro_rpc.rest_rpc_server(9004, 2)
rest.register_handler("handle_request", handle_request)
rest.register_handler("hello", hello)
rest.register_handler("async_hello", async_hello)
rest.async_start()

client = py_coro_rpc.rest_rpc_client("127.0.0.1", 9004)
r = client.connect()
print("connected: ", r)
result = client.call("handle_request", "test")
print(result)
result = client.call("hello", "world")
print(result)