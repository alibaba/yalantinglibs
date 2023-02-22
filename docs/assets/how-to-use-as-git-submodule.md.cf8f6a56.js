import{_ as s,c as a,o as n,a as e}from"./app.8c31cbc3.js";const d=JSON.parse('{"title":"Use as Git Submodule","description":"","frontmatter":{},"headers":[],"relativePath":"how-to-use-as-git-submodule.md","lastUpdated":1672883611000}'),l={name:"how-to-use-as-git-submodule.md"},o=e(`<h1 id="use-as-git-submodule" tabindex="-1">Use as Git Submodule <a class="header-anchor" href="#use-as-git-submodule" aria-hidden="true">#</a></h1><p>Thanks to <a href="https://github.com/piperoc" target="_blank" rel="noreferrer">@piperoc</a>, and the demo code is <a href="https://github.com/PikachuHyA/yalantinglibs_as_submodule_demo" target="_blank" rel="noreferrer">here</a>.</p><p><strong>Step 1</strong>: create your project directory mkdir <code>~/code/awesome-solution</code> and cd <code>~/code/awesome-solution</code>.</p><p><strong>Step 2</strong>: make it a git repo by calling <code>git init</code> (this will initialize and empty git repo)</p><p><strong>Step 3</strong>: add the library as a <em><strong>true submodule</strong></em> with <code>git submodule add https://github.com/alibaba/yalantinglibs.git</code> . At this point you should have a file system structure like:</p><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">awesome-solution</span></span>
<span class="line"><span style="color:#A6ACCD;">\u251C\u2500\u2500 .git</span></span>
<span class="line"><span style="color:#A6ACCD;">\u251C\u2500\u2500 .gitmodules</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2514\u2500\u2500 yalantinglibs</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><p><strong>Step 4</strong>: there are often more than one project using the library in the solution. In our demo, there would be a client app and a server app that both use the library. Those apps would be separate projects, so the file system will start looking like:</p><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">awesome-solution</span></span>
<span class="line"><span style="color:#A6ACCD;">\u251C\u2500\u2500 CMakeLists.txt</span></span>
<span class="line"><span style="color:#A6ACCD;">\u251C\u2500\u2500 client</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2502   \u251C\u2500\u2500 CMakeLists.txt</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2502   \u251C\u2500\u2500 client.cpp</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2502   \u251C\u2500\u2500 service.h</span></span>
<span class="line"><span style="color:#A6ACCD;">\u251C\u2500\u2500\u2500 server</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2502    \u251C\u2500\u2500 CMakeLists.txt</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2502    \u251C\u2500\u2500 server.cpp</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2502    \u251C\u2500\u2500 service.h</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2514\u2500\u2500\u2500 yalantinglibs</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><p><strong>Step 5</strong>: here&#39;s the root CMakeLists.txt (it&#39;s very important to set the compiler flags if you are running g++)</p><div class="language-cmake"><button title="Copy Code" class="copy"></button><span class="lang">cmake</span><pre class="shiki"><code><span class="line"></span>
<span class="line"><span style="color:#89DDFF;">cmake_minimum_required</span><span style="color:#A6ACCD;">(VERSION 3.15)</span></span>
<span class="line"></span>
<span class="line"><span style="color:#89DDFF;">project</span><span style="color:#A6ACCD;">(awesome-solution LANGUAGES CXX)</span></span>
<span class="line"><span style="color:#89DDFF;">set</span><span style="color:#A6ACCD;">(CMAKE_CXX_STANDARD 20)</span></span>
<span class="line"><span style="color:#89DDFF;">set</span><span style="color:#A6ACCD;">(CMAKE_CXX_STANDARD_REQUIRED </span><span style="color:#89DDFF;">ON</span><span style="color:#A6ACCD;">)</span></span>
<span class="line"><span style="color:#676E95;"># it&#39;s very important to set the compiler flags</span></span>
<span class="line"><span style="color:#676E95;"># if you use gcc/g++</span></span>
<span class="line"><span style="color:#89DDFF;">if</span><span style="color:#A6ACCD;"> (CMAKE_CXX_COMPILER_ID </span><span style="color:#89DDFF;">STREQUAL</span><span style="color:#A6ACCD;"> </span><span style="color:#C3E88D;">&quot;GNU&quot;</span><span style="color:#A6ACCD;">)</span></span>
<span class="line"><span style="color:#89DDFF;">    set</span><span style="color:#A6ACCD;">(CMAKE_CXX_FLAGS </span><span style="color:#C3E88D;">&quot;\${CMAKE_CXX_FLAGS} -fcoroutines&quot;</span><span style="color:#A6ACCD;">)</span></span>
<span class="line"><span style="color:#89DDFF;">    if</span><span style="color:#A6ACCD;"> (CMAKE_CXX_COMPILER_VERSION </span><span style="color:#89DDFF;">MATCHES</span><span style="color:#A6ACCD;"> </span><span style="color:#C3E88D;">&quot;12.*&quot;</span><span style="color:#A6ACCD;">)</span></span>
<span class="line"><span style="color:#89DDFF;">        set</span><span style="color:#A6ACCD;">(CMAKE_CXX_FLAGS </span><span style="color:#C3E88D;">&quot;\${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized&quot;</span><span style="color:#A6ACCD;">)</span></span>
<span class="line"><span style="color:#89DDFF;">    endif</span><span style="color:#A6ACCD;">()</span></span>
<span class="line"><span style="color:#89DDFF;">endif</span><span style="color:#A6ACCD;">()</span></span>
<span class="line"><span style="color:#89DDFF;">set</span><span style="color:#A6ACCD;">(CMAKE_EXPORT_COMPILE_COMMANDS </span><span style="color:#89DDFF;">ON</span><span style="color:#A6ACCD;">)</span></span>
<span class="line"></span>
<span class="line"><span style="color:#89DDFF;">add_subdirectory</span><span style="color:#A6ACCD;">(client)</span></span>
<span class="line"><span style="color:#89DDFF;">add_subdirectory</span><span style="color:#A6ACCD;">(server)</span></span>
<span class="line"><span style="color:#89DDFF;">add_subdirectory</span><span style="color:#A6ACCD;">(yalantinglibs)</span></span>
<span class="line"></span>
<span class="line"></span></code></pre></div>`,10),p=[o];function t(c,r,i,C,A,y){return n(),a("div",null,p)}const u=s(l,[["render",t]]);export{d as __pageData,u as default};
