export default {
  lang: 'en-US',
  title: 'yalantinglibs',
  description: 'A collection of C++20 libraries, include async_simple, coro_rpc and struct_pack.',
  base: '/yalantinglibs/',
  lastUpdated: true,
  ignoreDeadLinks: false,
  outDir: "../docs",
  locales: {
    "/": {
      lang: 'en-US',
      title: 'yalantinglibs',
      description: 'A collection of C++20 libraries, include async_simple, coro_rpc and struct_pack.',
    },
    "/zh/": {
      lang: 'zh-CN',
      title: '雅兰亭库',
      description: 'C++20程序库，包含async_simple, coro_rpc and struct_pack.',
    },
  },
  head: [],

  themeConfig: {
    locales: {
      "/": {
        nav: nav(),
        sidebar: {
          "/guide/": sidebarGuide(),
        },
      },
      "/zh/": {
        nav: nav(),
        sidebar: {
          "/zh/guide/": sidebarGuideZh(),
        },
        selectLanguageName: '简体中文',
        selectLanguageText: '选择语言',
        selectLanguageAriaLabel: '选择语言',
      }
    },
    socialLinks: [
      {icon: 'github', link: 'https://github.com/alibaba/yalantinglibs'}
    ],

    footer: {
      message: 'This website is released under the MIT License.',
      copyright: 'Copyright © 2022 yalantinglibs contributors'
    },
  }
}

function nav() {
  return [
    {text: 'Guide', link: '/guide/what-is-yalantinglibs', activeMatch: '/guide/'},
    {
      text: "Language",
      items: [
        {
          text: "English", link: "/guide/what-is-yalantinglibs"
        },
        {
          text: "简体中文", link: '/zh/guide/what-is-yalantinglibs'
        }
      ]
    },
    {
      text: 'Github Issues',
      link: 'https://github.com/alibaba/yalantinglibs/issues'
    }
  ]
}

function sidebarGuide() {
  return [
    {
      text: 'struct_pack',
      collapsible: true,
      items: [
        {text: 'What is struct_pack?', link: '/guide/struct-pack-intro'},
        {
          text: 'API Reference',
          link: "https://alibaba.github.io/yalantinglibs/en/html/group__struct__pack.html"
        }
      ]
    },
    {
      text: 'coro_rpc',
      collapsible: true,
      items: [
        {text: 'What is coro_rpc?', link: '/guide/coro-rpc-intro'},
      ]
    }
  ]
}

function sidebarGuideZh() {
  return [
    {
      text: 'struct_pack',
      collapsible: true,
      items: [
        {text: 'struct_pack简介', link: '/zh/guide/struct-pack-intro'},
        {text: 'struct_pack类型系统', link: '/zh/guide/struct-pack-type-system'},
        {text: 'struct_pack布局', link: '/zh/guide/struct-pack-layout'},
        {
          text: 'API Reference',
          link: "https://alibaba.github.io/yalantinglibs/cn/html/group__struct__pack.html"
        }
      ]
    },
    {
      text: 'coro_rpc',
      collapsible: true,
      items: [
        {text: 'coro_rpc简介', link: '/zh/guide/coro-rpc-intro'},
      ]
    }
  ]
}
