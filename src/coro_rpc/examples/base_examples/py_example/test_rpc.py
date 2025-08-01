import asyncio
import py_coro_rpc

def handle_msg(con, msg):
    # begin to handle msg from client, do business

    # after finish business, response result
    con.response_msg(msg, lambda r: print("done", len(msg)))

async def async_request(pool):
    loop = asyncio.get_event_loop()
    for i in range(3):
        print("Python is doing other work while C++ runs...")
        # await the result from C++
        result = await pool.async_send_msg(loop, b"hello world")
        if(result.code):
            print("rpc failed, reason:", result.err_msg)
        else:
            view = result.str_view()
            print("Python received result from C++:", view[:result.data_size].tobytes())
    
    buf = bytearray(2 * 1024)
    result, len = await pool.async_send_msg_with_outbuf(loop, b"hello world", buf)
    print(result)
    print(f"Python received result with out buffer from C++: {buf[:len]}")

if __name__ == "__main__":
    # init server and client pool
    server = py_coro_rpc.coro_rpc_server(2, "0.0.0.0:9004", handle_msg, 10)
    r = server.async_start()
    print("server start", r)

    pool = py_coro_rpc.py_coro_rpc_client_pool("0.0.0.0:9004")

    asyncio.run(async_request(pool))
