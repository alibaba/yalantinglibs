import{_ as s,c as n,o as e,a}from"./app.8c31cbc3.js";const y=JSON.parse('{"title":"Build Website","description":"","frontmatter":{},"headers":[],"relativePath":"README.md","lastUpdated":1676888337000}'),l={name:"README.md"},p=a(`<h1 id="build-website" tabindex="-1">Build Website <a class="header-anchor" href="#build-website" aria-hidden="true">#</a></h1><p>The website is built by vitepress and doxygen and you can build the website in your own computer</p><p>Before build the website, <code>git</code>, <code>yarn</code>, <code>node</code> are required.</p><ul><li>clone the repo</li></ul><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">git clone https://github.com/alibaba/yalantinglibs.git</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><ul><li>checkout <code>doc</code> branch</li></ul><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">cd yalantinglibs</span></span>
<span class="line"><span style="color:#A6ACCD;">git checkout doc</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><ul><li>clone <code>doxygen-awesome-css</code> (for Doxygen)</li></ul><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">https://github.com/jothepro/doxygen-awesome-css.git</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><ul><li>install dependencies for vitepress</li></ul><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">yarn install</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><ul><li>generate website</li></ul><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">bash gen_doc.sh</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><p>the script will copy all markdown files and images to website folder</p><p>then use <code>yarn docs:build</code> and <code>doxygen Doxyfile</code> generate html</p><p>some logs</p><div class="language-"><button title="Copy Code" class="copy"></button><span class="lang"></span><pre class="shiki"><code><span class="line"><span style="color:#A6ACCD;">yarn run v1.22.19</span></span>
<span class="line"><span style="color:#A6ACCD;">$ vitepress build website</span></span>
<span class="line"><span style="color:#A6ACCD;">vitepress v1.0.0-alpha.29</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2713 building client + server bundles...</span></span>
<span class="line"><span style="color:#A6ACCD;">\u280B rendering pages...(node:77603) ExperimentalWarning: The Fetch API is an experimental feature. This feature could change at any time</span></span>
<span class="line"><span style="color:#A6ACCD;">(Use \`node --trace-warnings ...\` to show where the warning was created)</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2713 rendering pages...</span></span>
<span class="line"><span style="color:#A6ACCD;">build complete in 11.86s.</span></span>
<span class="line"><span style="color:#A6ACCD;">\u2728  Done in 12.95s.</span></span>
<span class="line"><span style="color:#A6ACCD;">Doxygen version used: 1.9.6</span></span>
<span class="line"><span style="color:#A6ACCD;">...</span></span>
<span class="line"><span style="color:#A6ACCD;">...</span></span>
<span class="line"><span style="color:#A6ACCD;">...</span></span>
<span class="line"><span style="color:#A6ACCD;">Running plantuml with JAVA...</span></span>
<span class="line"><span style="color:#A6ACCD;">type lookup cache used 63/65536 hits=63 misses=63</span></span>
<span class="line"><span style="color:#A6ACCD;">symbol lookup cache used 106/65536 hits=1267 misses=106</span></span>
<span class="line"><span style="color:#A6ACCD;">finished...</span></span>
<span class="line"><span style="color:#A6ACCD;">Done!</span></span>
<span class="line"><span style="color:#A6ACCD;"></span></span></code></pre></div><p>the config of vitepress is <code>website/.vitepress/config.ts</code></p><p>the config of doxygen is <code>Doxyfile</code> and <code>Doxyfile_cn</code></p>`,19),o=[p];function c(t,i,d,r,C,A){return e(),n("div",null,o)}const g=s(l,[["render",c]]);export{y as __pageData,g as default};
