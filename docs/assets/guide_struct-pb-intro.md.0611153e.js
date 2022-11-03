import{_ as e,c as t,o as s,a}from"./app.8a8ca21b.js";const n="/yalantinglibs/assets/pb_format.17b6c761.jpeg",o="/yalantinglibs/assets/struct_pb_overview.d35af7a0.jpeg",b=JSON.parse('{"title":"struct_pb Introduction","description":"","frontmatter":{},"headers":[{"level":2,"title":"Motivation","slug":"motivation","link":"#motivation","children":[]},{"level":2,"title":"Background","slug":"background","link":"#background","children":[{"level":3,"title":"Message Structure","slug":"message-structure","link":"#message-structure","children":[]},{"level":3,"title":"Base 128 Varints","slug":"base-128-varints","link":"#base-128-varints","children":[]}]},{"level":2,"title":"Design","slug":"design","link":"#design","children":[]},{"level":2,"title":"Type Mapping","slug":"type-mapping","link":"#type-mapping","children":[{"level":3,"title":"Overview","slug":"overview","link":"#overview","children":[]},{"level":3,"title":"Basic","slug":"basic","link":"#basic","children":[]}]},{"level":2,"title":"File Mapping","slug":"file-mapping","link":"#file-mapping","children":[]},{"level":2,"title":"protoc Plugin","slug":"protoc-plugin","link":"#protoc-plugin","children":[]},{"level":2,"title":"TODO","slug":"todo","link":"#todo","children":[]},{"level":2,"title":"Compatibility","slug":"compatibility","link":"#compatibility","children":[]},{"level":2,"title":"Limitation","slug":"limitation","link":"#limitation","children":[]},{"level":2,"title":"Acknowledge","slug":"acknowledge","link":"#acknowledge","children":[]},{"level":2,"title":"Reference","slug":"reference","link":"#reference","children":[]}],"relativePath":"guide/struct-pb-intro.md","lastUpdated":null}'),l={name:"guide/struct-pb-intro.md"},r=a('<h1 id="struct-pb-introduction" tabindex="-1">struct_pb Introduction <a class="header-anchor" href="#struct-pb-introduction" aria-hidden="true">#</a></h1><h2 id="motivation" tabindex="-1">Motivation <a class="header-anchor" href="#motivation" aria-hidden="true">#</a></h2><p>Protocol buffers are a language-neutral, platform-neutral extensible mechanism for serializing structured data. So many programs use protobuf as their serialization library. It is convienient for our customers to use <code>struct_pack</code> if we (<code>struct_pack</code>) can compatible with protobuf binary format. That is why we create <code>struct_pb</code>.</p><h2 id="background" tabindex="-1">Background <a class="header-anchor" href="#background" aria-hidden="true">#</a></h2><p>In this section, we introduce the <a href="https://developers.google.com/protocol-buffers/docs/encoding" target="_blank" rel="noreferrer">protocol buffer wire format</a>, which defines the details of how protobuf message is sent on the wire and how much space it consumes on disk.</p><h3 id="message-structure" tabindex="-1">Message Structure <a class="header-anchor" href="#message-structure" aria-hidden="true">#</a></h3><p>A protocol buffer message is a series of key-value pairs. The binary version of a message just uses the field&#39;s number as the key -- the name and declared type for each field can only be determined on the decoding end by referencing the message type&#39;s definition. <img src="'+n+`" alt=""></p><div class="language-cpp"><button title="Copy Code" class="copy"></button><span class="lang">cpp</span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">Tag </span><span style="color:#89DDFF;">=</span><span style="color:#A6ACCD;"> </span><span style="color:#89DDFF;">(</span><span style="color:#A6ACCD;">field_number </span><span style="color:#89DDFF;">&lt;&lt;</span><span style="color:#A6ACCD;"> </span><span style="color:#F78C6C;">3</span><span style="color:#89DDFF;">)</span><span style="color:#A6ACCD;"> </span><span style="color:#89DDFF;">|</span><span style="color:#A6ACCD;"> wire_type</span></span>
<span class="line"></span></code></pre></div><h3 id="base-128-varints" tabindex="-1">Base 128 Varints <a class="header-anchor" href="#base-128-varints" aria-hidden="true">#</a></h3><p>Variable-width integers, or <em>varints</em>, are at the core of the wire format. They allow encoding unsigned 64-bit integers using anywhere between one and ten bytes, with small values using fewer bytes.</p><h2 id="design" tabindex="-1">Design <a class="header-anchor" href="#design" aria-hidden="true">#</a></h2><p><img src="`+o+`" alt=""></p><h2 id="type-mapping" tabindex="-1">Type Mapping <a class="header-anchor" href="#type-mapping" aria-hidden="true">#</a></h2><p>proto3 first, see also <a href="https://developers.google.com/protocol-buffers/docs/proto3#scalar" target="_blank" rel="noreferrer">Protocol Buffers Language Guide (proto3)</a></p><h3 id="overview" tabindex="-1">Overview <a class="header-anchor" href="#overview" aria-hidden="true">#</a></h3><p>Scalar Value Types with no modifier (a.k.a <strong>singular</strong>) -&gt; T</p><p>Scalar Value Types with <strong>optional</strong> -&gt; <code>std::optional &lt;T&gt;</code></p><p>any type with <strong>repeat</strong> -&gt; <code>std::vector&lt;T&gt;</code></p><p>types with <strong>map</strong> -&gt; <code>std::map&lt;K, V&gt;</code></p><p>any message type -&gt; <code>std::optional &lt;T&gt;</code></p><p>enum -&gt; enum class</p><p>oneof -&gt; <code>std::variant &lt;std::monostate, ...&gt;</code></p><p>Note:</p><ul><li>singular. You cannot determine whether it was parsed from the wire. It will be serialized to the wire unless it is the default value. see also <a href="https://github.com/protocolbuffers/protobuf/blob/main/docs/field_presence.md" target="_blank" rel="noreferrer">Field Presence</a>.</li><li>optional. You can check to see if the value was explicitly set.</li><li>repeat. In proto3, repeated fields of scalar numeric types use packed encoding by default.</li><li>map. The key of map can be any integral or string type except enum. The value of map can be any type except another map.</li><li>enum. Every enum definition must contain a constant that maps to zero as its first element. Enumerator constants must be in the range of a 32-bit integer. During deserialization, unrecognized enum values will be preserved in the message</li><li>oneof. If you set an oneof field to the default value (such as setting an int32 oneof field to 0), the &quot;case&quot; of that oneof field will be set, and the value will be serialized on the wire.</li><li>default value <ul><li>For numeric types, the default is 0.</li><li>For enums, the default is the zero-valued enumerator.</li><li>For strings, bytes, and repeated fields, the default is the zero-length value.</li><li>For messages, the default is the language-specific null value.</li></ul></li></ul><h3 id="basic" tabindex="-1">Basic <a class="header-anchor" href="#basic" aria-hidden="true">#</a></h3><table><thead><tr><th>.proto Type</th><th>struct_pb Type</th><th>pb native C++ type</th><th>Notes</th></tr></thead><tbody><tr><td>double</td><td>double</td><td>double</td><td>8 bytes</td></tr><tr><td>float</td><td>float</td><td>float</td><td>4 bytes</td></tr><tr><td>int32</td><td>int32</td><td>int32</td><td>Uses variable-length encoding.</td></tr><tr><td>int64</td><td>int64</td><td>int64</td><td></td></tr><tr><td>uint32</td><td>uint32</td><td>uint32</td><td></td></tr><tr><td>uint64</td><td>uint64</td><td>uint64</td><td></td></tr><tr><td>sint32</td><td>int32</td><td>int32</td><td>ZigZag + variable-length encoding.</td></tr><tr><td>sint64</td><td>int64</td><td>int64</td><td></td></tr><tr><td>fixed32</td><td>uint32</td><td>uint32</td><td>4 bytes</td></tr><tr><td>fixed64</td><td>uint64</td><td>uint64</td><td>8 bytes</td></tr><tr><td>sfixed32</td><td>int32</td><td>int32</td><td>4 bytes,$2^{28}$</td></tr><tr><td>sfixed64</td><td>int64</td><td>int64</td><td>8 bytes,$2^{56}$</td></tr><tr><td>bool</td><td>bool</td><td>bool</td><td></td></tr><tr><td>string</td><td>std::string</td><td>string</td><td>$len &lt; 2^{32}$</td></tr><tr><td>bytes</td><td>std::string</td><td>string</td><td></td></tr><tr><td>enum</td><td>enum class: int {}</td><td>enum: int {}</td><td></td></tr><tr><td>oneof</td><td>std::variant&lt;std::monostate, ...&gt;</td><td></td><td></td></tr></tbody></table><p>Note:</p><ul><li>for enum, we use <code>enum class</code> instead of <code>enum</code></li><li>for oneof, we use <code>std::variant</code> with first template argument <code>std::monostate</code></li></ul><h2 id="file-mapping" tabindex="-1">File Mapping <a class="header-anchor" href="#file-mapping" aria-hidden="true">#</a></h2><p>each <code>xxx.proto</code>file generates corresponding <code>xxx.struct_pb.h</code> and <code>xxx.struct_pb.cc</code> all dependencies will convert to include corresponding headers.</p><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">syntax = &quot;proto3&quot;;</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span>
<span class="line"><span style="color:#A6ACCD;">import &quot;foo.proto&quot;;</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span>
<span class="line"><span style="color:#A6ACCD;">message Bar {</span></span>
<span class="line"><span style="color:#A6ACCD;">    Foo f = 2;</span></span>
<span class="line"><span style="color:#A6ACCD;">}</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><p>mapping to cpp</p><div class="language-cpp"><button title="Copy Code" class="copy"></button><span class="lang">cpp</span><pre class="shiki"><code><span class="line"><span style="color:#89DDFF;">#include</span><span style="color:#A6ACCD;"> </span><span style="color:#89DDFF;">&quot;</span><span style="color:#C3E88D;">foo.struct_pb.h</span><span style="color:#89DDFF;">&quot;</span></span>
<span class="line"><span style="color:#C792EA;">struct</span><span style="color:#F07178;"> </span><span style="color:#FFCB6B;">Bar</span><span style="color:#F07178;"> </span><span style="color:#89DDFF;">{</span></span>
<span class="line"><span style="color:#FFCB6B;">std</span><span style="color:#89DDFF;">::</span><span style="color:#F07178;">unique_ptr</span><span style="color:#89DDFF;">&lt;</span><span style="color:#F07178;">Foo</span><span style="color:#89DDFF;">&gt;</span><span style="color:#F07178;"> f</span><span style="color:#89DDFF;">;</span></span>
<span class="line"><span style="color:#89DDFF;">};</span></span>
<span class="line"></span>
<span class="line"></span></code></pre></div><h2 id="protoc-plugin" tabindex="-1">protoc Plugin <a class="header-anchor" href="#protoc-plugin" aria-hidden="true">#</a></h2><div class="language-bash"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">protoc --plugin=path/to/protoc-gen-structpb --structpb_out=</span><span style="color:#89DDFF;">$</span><span style="color:#A6ACCD;">OUT_PATH xxx.proto</span></span>
<span class="line"></span></code></pre></div><p>e.g.</p><div class="language-shell"><button title="Copy Code" class="copy"></button><span class="lang">shell</span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">protoc --plugin=/tmp/protoc-gen-structpb --structpb_out=. conformance.proto</span></span>
<span class="line"></span></code></pre></div><h2 id="todo" tabindex="-1">TODO <a class="header-anchor" href="#todo" aria-hidden="true">#</a></h2><p>-[ ] using value instead of <code>std::unique_ptr</code> when no circle dependencies</p><p>for example, we convert the message <code>SearchResponse</code> to c++ struct <code>SearchResponse_v1</code></p><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">message SearchResponse {</span></span>
<span class="line"><span style="color:#A6ACCD;">  Result result = 1;</span></span>
<span class="line"><span style="color:#A6ACCD;">}</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span>
<span class="line"><span style="color:#A6ACCD;">message Result {</span></span>
<span class="line"><span style="color:#A6ACCD;">  string url = 1;</span></span>
<span class="line"><span style="color:#A6ACCD;">}</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><div class="language-cpp"><button title="Copy Code" class="copy"></button><span class="lang">cpp</span><pre class="shiki"><code><span class="line"><span style="color:#C792EA;">struct</span><span style="color:#F07178;"> </span><span style="color:#FFCB6B;">SearchResponse_v1</span><span style="color:#F07178;"> </span><span style="color:#89DDFF;">{</span></span>
<span class="line"><span style="color:#F07178;">    </span><span style="color:#FFCB6B;">std</span><span style="color:#89DDFF;">::</span><span style="color:#F07178;">unique_ptr</span><span style="color:#89DDFF;">&lt;</span><span style="color:#F07178;">Result</span><span style="color:#89DDFF;">&gt;</span><span style="color:#F07178;"> result</span><span style="color:#89DDFF;">;</span></span>
<span class="line"><span style="color:#89DDFF;">};</span></span>
<span class="line"></span></code></pre></div><p>we can optimize the pointer overhead if we can convert the message <code>SearchResponse</code> to c++ struct <code>SearchResponse_v2</code></p><div class="language-cpp"><button title="Copy Code" class="copy"></button><span class="lang">cpp</span><pre class="shiki"><code><span class="line"><span style="color:#C792EA;">struct</span><span style="color:#F07178;"> </span><span style="color:#FFCB6B;">SearchResponse_v2</span><span style="color:#F07178;"> </span><span style="color:#89DDFF;">{</span></span>
<span class="line"><span style="color:#F07178;">    Result result</span><span style="color:#89DDFF;">;</span></span>
<span class="line"><span style="color:#89DDFF;">};</span></span>
<span class="line"></span></code></pre></div><h2 id="compatibility" tabindex="-1">Compatibility <a class="header-anchor" href="#compatibility" aria-hidden="true">#</a></h2><p>see also</p><ul><li><a href="https://developers.google.com/protocol-buffers/docs/proto3#updating" target="_blank" rel="noreferrer">Language Guide (proto3) -- Updating A Message Type</a></li><li><a href="https://developers.google.com/protocol-buffers/docs/proto3#backwards-compatibility_issues" target="_blank" rel="noreferrer">oneof compatibility</a></li></ul><h2 id="limitation" tabindex="-1">Limitation <a class="header-anchor" href="#limitation" aria-hidden="true">#</a></h2><ul><li>SGROUP(deprecated) and EGROUP(deprecated) are not support</li><li>Reflection not support</li><li>proto2 extension not support</li></ul><h2 id="acknowledge" tabindex="-1">Acknowledge <a class="header-anchor" href="#acknowledge" aria-hidden="true">#</a></h2><ul><li><a href="https://embeddedproto.com/" target="_blank" rel="noreferrer">Embedded Proto</a>: an easy to use C++ Protocol Buffer implementation specifically suited for microcontrollers</li><li><a href="https://github.com/protobuf-c/protobuf-c" target="_blank" rel="noreferrer">protobuf-c</a>: Protocol Buffers implementation in C</li><li><a href="https://github.com/mapbox/protozero" target="_blank" rel="noreferrer">protozero</a>: Minimalist protocol buffer decoder and encoder in C++</li><li><a href="https://github.com/PragmaTwice/protopuf" target="_blank" rel="noreferrer">protopuf</a>: A little, highly templated, and protobuf-compatible serialization/deserialization header-only library written in C++20</li></ul><h2 id="reference" tabindex="-1">Reference <a class="header-anchor" href="#reference" aria-hidden="true">#</a></h2><ul><li><a href="https://developers.google.com/protocol-buffers/docs/reference/other#plugins" target="_blank" rel="noreferrer">Protocol Buffers Compiler Plugins</a></li></ul>`,53),i=[r];function p(c,d,u,h,g,f){return s(),t("div",null,i)}const m=e(l,[["render",p]]);export{b as __pageData,m as default};
