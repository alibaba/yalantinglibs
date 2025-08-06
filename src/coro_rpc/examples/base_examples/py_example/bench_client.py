import asyncio
import py_coro_rpc

def send_request(pool):
    buf = bytes(2 * 1024 *1024)
    while(True):
        pool.sync_send_msg1(buf) #17420
        # pool.sync_send_msg(b"hello world") #17272

if __name__ == "__main__":
    pool = py_coro_rpc.py_coro_rpc_client_pool("0.0.0.0:9004")
    send_request(pool)
    
    asyncio.run(async_request(pool))