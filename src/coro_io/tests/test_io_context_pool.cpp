#include <async_simple/coro/SyncAwait.h>
#include <doctest.h>

#include <string>
#include <thread>
#include <ylt/coro_io/io_context_pool.hpp>

using namespace async_simple::coro;

TEST_CASE("test ExecutorWrapper user data functionality") {
  SUBCASE("test set_data and get_data") {
    asio::io_context io_ctx;
    coro_io::ExecutorWrapper<> executor(io_ctx.get_executor());

    // 测试存储和获取字符串数据
    std::string test_key = "test_string";
    std::string test_value = "Hello World";
    executor.set_data(test_key, test_value);

    std::string* retrieved_value = executor.get_data<std::string>(test_key);
    CHECK(retrieved_value != nullptr);
    CHECK(*retrieved_value == test_value);
  }

  SUBCASE("test get_data with non-existent key") {
    asio::io_context io_ctx;
    coro_io::ExecutorWrapper<> executor(io_ctx.get_executor());

    std::string* retrieved_value =
        executor.get_data<std::string>("non_existent_key");
    CHECK(retrieved_value == nullptr);
  }

  SUBCASE("test set_data and get_data with different types") {
    asio::io_context io_ctx;
    coro_io::ExecutorWrapper<> executor(io_ctx.get_executor());

    // 存储整数
    executor.set_data("int_value", 42);
    int* int_ptr = executor.get_data<int>("int_value");
    CHECK(int_ptr != nullptr);
    CHECK(*int_ptr == 42);

    // 存储布尔值
    executor.set_data("bool_value", true);
    bool* bool_ptr = executor.get_data<bool>("bool_value");
    CHECK(bool_ptr != nullptr);
    CHECK(*bool_ptr == true);

    // 存储浮点数
    executor.set_data("float_value", 3.14f);
    float* float_ptr = executor.get_data<float>("float_value");
    CHECK(float_ptr != nullptr);
    CHECK(*float_ptr == 3.14f);
  }

  SUBCASE("test get_data_with_default") {
    asio::io_context io_ctx;
    coro_io::ExecutorWrapper<> executor(io_ctx.get_executor());

    // 测试获取不存在的键，应该创建默认值
    int* default_int = executor.get_data_with_default<int>("default_int");
    CHECK(default_int != nullptr);
    CHECK(*default_int == 0);  // int类型的默认值

    // 修改默认值
    *default_int = 100;
    int* retrieved_int = executor.get_data<int>("default_int");
    CHECK(retrieved_int != nullptr);
    CHECK(*retrieved_int == 100);

    // 测试自定义类型的默认值
    std::string* default_str =
        executor.get_data_with_default<std::string>("default_str");
    CHECK(default_str != nullptr);
    CHECK(*default_str == "");  // string类型的默认值
  }

  SUBCASE("test clear_all_data") {
    asio::io_context io_ctx;
    coro_io::ExecutorWrapper<> executor(io_ctx.get_executor());

    // 设置一些数据
    executor.set_data("key1", std::string("value1"));
    executor.set_data("key2", 123);

    // 验证数据存在
    std::string* str_ptr = executor.get_data<std::string>("key1");
    int* int_ptr = executor.get_data<int>("key2");
    CHECK(str_ptr != nullptr);
    CHECK(int_ptr != nullptr);
    CHECK(*str_ptr == "value1");
    CHECK(*int_ptr == 123);

    // 清除所有数据
    executor.clear_all_data();

    // 验证数据已被清除
    str_ptr = executor.get_data<std::string>("key1");
    int_ptr = executor.get_data<int>("key2");
    CHECK(str_ptr == nullptr);
    CHECK(int_ptr == nullptr);

    // 但默认值应该被创建
    str_ptr = executor.get_data_with_default<std::string>("key1");
    CHECK(str_ptr != nullptr);
    CHECK(*str_ptr == "");
  }

  SUBCASE("test ExecutorWrapper with io_context_pool") {
    coro_io::io_context_pool pool(2);

    // 获取执行器并测试数据存储
    auto* executor = pool.get_executor();
    executor->set_data("pool_data", std::string("test_data"));

    std::string* retrieved = executor->get_data<std::string>("pool_data");
    CHECK(retrieved != nullptr);
    CHECK(*retrieved == "test_data");

    // 测试从不同线程获取的执行器具有独立的数据存储
    auto* executor2 = pool.get_executor();
    std::string* retrieved2 = executor2->get_data<std::string>("pool_data");
    CHECK(retrieved2 == nullptr);
    executor2->set_data("pool_data", std::string("test_data2"));
    retrieved2 = executor2->get_data<std::string>("pool_data");
    CHECK(retrieved2 != nullptr);
    CHECK(*retrieved2 == "test_data2");
    CHECK(*retrieved == "test_data");
    CHECK(retrieved != retrieved2);
  }

  SUBCASE("test multiple data types in same executor") {
    asio::io_context io_ctx;
    coro_io::ExecutorWrapper<> executor(io_ctx.get_executor());

    // 存储多种类型的数据
    executor.set_data("string_val", std::string("test"));
    executor.set_data("int_val", 42);
    executor.set_data("bool_val", true);
    executor.set_data("double_val", 3.14159);

    // 检索并验证所有数据
    auto* str_val = executor.get_data<std::string>("string_val");
    auto* int_val = executor.get_data<int>("int_val");
    auto* bool_val = executor.get_data<bool>("bool_val");
    auto* double_val = executor.get_data<double>("double_val");

    CHECK(str_val != nullptr);
    CHECK(int_val != nullptr);
    CHECK(bool_val != nullptr);
    CHECK(double_val != nullptr);

    CHECK(*str_val == "test");
    CHECK(*int_val == 42);
    CHECK(*bool_val == true);
    CHECK(*double_val == 3.14159);
  }

  SUBCASE("test executor data persistence after tasks") {
    asio::io_context io_ctx;
    coro_io::ExecutorWrapper<> executor(io_ctx.get_executor());

    // 设置数据
    executor.set_data("persistent_data", 100);

    // 执行一个任务，确保数据仍然存在
    bool task_executed = false;
    executor.schedule([&task_executed, &executor]() {
      task_executed = true;
      auto* data = executor.get_data<int>("persistent_data");
      CHECK(data != nullptr);
      CHECK(*data == 100);
    });

    // 运行一次循环来处理任务
    io_ctx.run_one();

    CHECK(task_executed);

    // 验证任务执行后数据仍然存在
    auto* data_after_task = executor.get_data<int>("persistent_data");
    CHECK(data_after_task != nullptr);
    CHECK(*data_after_task == 100);
  }
}