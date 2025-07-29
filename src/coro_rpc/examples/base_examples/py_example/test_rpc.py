import asyncio
import py_coro_rpc

def handle_msg(con, msg):
    # begin to handle msg from client, do business

    # after finish business, response result
    con.response_msg(msg, lambda r: print("done", msg))

async def async_request(pool):
    loop = asyncio.get_event_loop()

    for i in range(3):
        print("Python is doing other work while C++ runs...")
        # await the result from C++
        result = await pool.async_send_msg(loop, b"hello world")
        print(f"Python received result from C++: {result}")
    

if __name__ == "__main__":
    # init server and client pool
    server = py_coro_rpc.coro_rpc_server(2, "0.0.0.0:9004", handle_msg, 10)
    r = server.async_start()
    print("server start", r)

    pool = py_coro_rpc.py_coro_rpc_client_pool("0.0.0.0:9004")

    asyncio.run(async_request(pool))
