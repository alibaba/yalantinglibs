/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "yaLanTingLibs", "index.html", [
    [ "coro_rpc简介", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html", [
      [ "coro_rpc的易用性", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md1", [
        [ "rpc_server端", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md2", null ],
        [ "rpc函数支持任意参数", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md3", null ]
      ] ],
      [ "和grpc、brpc比较易用性", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md4", [
        [ "rpc易用性比较", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md5", null ],
        [ "异步编程模型比较", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md6", null ]
      ] ],
      [ "coro_rpc更多特色", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md7", [
        [ "同时支持实时任务和延时任务", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md8", null ],
        [ "服务端同时支持协程和异步回调", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md9", null ]
      ] ],
      [ "benchmark", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md10", [
        [ "测试环境", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md11", null ],
        [ "测试case", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md12", [
          [ "极限qps测试", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md13", null ],
          [ "ping-pong测试", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md14", null ],
          [ "长尾测试", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md15", null ]
        ] ],
        [ "benchmark备注", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md16", null ]
      ] ],
      [ "使用约束", "md_src_coro_rpc_doc_coro_rpc_introduction_cn.html#autotoc_md17", null ]
    ] ],
    [ "struct_pack简介", "md_src_struct_pack_doc__introduction__c_n.html", [
      [ "序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md19", [
        [ "基本用法", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md20", null ],
        [ "指定序列化返回的容器类型", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md21", null ],
        [ "将序列化结果保存到已有的容器尾部", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md22", null ],
        [ "将序列化结果保存到指针指向的内存中。", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md23", null ],
        [ "多参数序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md24", null ],
        [ "将序列化结果保存到输出流", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md25", null ]
      ] ],
      [ "反序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md26", [
        [ "基本用法", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md27", null ],
        [ "从指针指向的内存中反序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md28", null ],
        [ "反序列化（将结果保存到已有的对象中）", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md29", null ],
        [ "多参数反序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md30", null ],
        [ "从输入流中反序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md31", null ],
        [ "部分反序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md32", null ]
      ] ],
      [ "支持序列化所有的STL容器、自定义容器和optional", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md33", null ],
      [ "自定义功能支持", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md34", [
        [ "自定义类型的序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md35", null ],
        [ "序列化到自定义的输出流", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md36", null ],
        [ "从自定义的输入流中反序列化", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md37", null ]
      ] ],
      [ "支持可变长编码：", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md38", null ],
      [ "benchmark", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md39", [
        [ "测试方法", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md40", null ],
        [ "测试对象", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md41", null ],
        [ "测试环境", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md42", null ],
        [ "测试结果", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md43", null ]
      ] ],
      [ "向前/向后兼容性", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md44", null ],
      [ "为什么struct_pack更快？", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md45", null ],
      [ "附录", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md46", [
        [ "关于struct_pack类型系统", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md47", null ],
        [ "关于struct_pack的编码与布局", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md48", null ],
        [ "测试代码", "md_src_struct_pack_doc__introduction__c_n.html#autotoc_md49", null ]
      ] ]
    ] ],
    [ "模块", "modules.html", "modules" ],
    [ "类", "annotated.html", [
      [ "类列表", "annotated.html", "annotated_dup" ],
      [ "类索引", "classes.html", null ],
      [ "类成员", "functions.html", [
        [ "全部", "functions.html", null ],
        [ "函数", "functions_func.html", null ]
      ] ]
    ] ],
    [ "文件", "files.html", [
      [ "文件列表", "files.html", "files_dup" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html"
];

var SYNCONMSG = '点击 关闭 面板同步';
var SYNCOFFMSG = '点击 开启 面板同步';