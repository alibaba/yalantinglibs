import asyncio
import py_example

async def async_get(url):
    loop = asyncio.get_running_loop()
    future = loop.create_future()    
    def cpp_callback(message):
        def set_result():
            future.set_result(message)

        loop.call_soon_threadsafe(set_result)

    caller = py_example.caller(cpp_callback)
    caller.async_get(url)
    result = await future
    print(result)

async def main():
    await async_get("http://taobao.com")

if __name__ == "__main__":
    asyncio.run(main())