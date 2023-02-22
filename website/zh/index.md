---
layout: home

title: yalantinglibs
titleTemplate: C++20库的集合，包含async_simple、coro_rpc和struct_pack。

hero:
  name: 雅兰亭库
  text: 一个C++20库集合
  actions:
    - theme: brand
      text: 入门教程
      link: /zh/guide/what-is-yalantinglibs
    - theme: alt
      text: 查看GitHub
      link: https://github.com/alibaba/yalantinglibs.git

features:
  - title: struct_pack
    details: 只有一行代码可以完成序列化和反序列化，比pb快10-50倍。
  - title: coro_rpc
    details: 非常易于使用、基于协程的高性能rpc框架。在C++20中，echo场景中的qps超过2000w。
  - title: struct_json
    details: 基于反射的json-lib，很容易实现从struct到json以及json到struct的映射。
---