// @ts-check
import { defineConfigWithTheme } from 'vitepress'

export default defineConfigWithTheme({
  lang: 'en-US',
  title: 'yalantinglibs',
  description: 'A collection of C++20 libraries, include async_simple, coro_rpc and struct_pack.',
  base: '/yalantinglibs/',
  outDir: '../docs',
  cleanUrls: 'without-subfolders',
  lastUpdated: true,
  ignoreDeadLinks: false,
  shouldPreload: (link: string, page: string): boolean =>  {
    return false
  },
  locales: {
    '/': {
      lang: 'en-US',
      title: 'yalantinglibs',
      description: 'A collection of C++20 libraries, include async_simple, coro_rpc and struct_pack.',
    },
    '/zh/': {
      lang: 'zh-CN',
      title: '雅兰亭库',
      description: 'C++20程序库，包含async_simple、coro_rpc和struct_pack.',
    },
  },
  head: [],
  themeConfig: {
    nav: nav('Guide', 'Language', '/en'),
    sidebar: {
      '/en/guide/': sidebarGuide(),
    },
    locales: {
      '/zh/': {
        label: '中文',
        selectText: '选择语言',
        nav: nav('指南', '语言', '/zh'),
        sidebar: {
          '/zh/guide/': sidebarGuideZh(),
        },
      }
    },
    socialLinks: [
      {
        icon: 'github', link: 'https://github.com/alibaba/yalantinglibs'
      }
    ],
    footer: {
      message: 'This website is released under the MIT License.',
      copyright: 'Copyright © 2022 yalantinglibs contributors'
    },
  }
})

function nav(guideText: string, LanguageText: string, lan: string) {
  return [
    {
      text: guideText,
      link:  `${lan}/guide/what-is-yalantinglibs`, activeMatch: '/guide/'
    },
    {
      text: LanguageText,
      items: [
        {
          text: 'English', link: '/en/'
        },
        {
          text: '简体中文', link: '/zh/',
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
      text: 'how to use?',
      collapsible: true,
      items: [
        {text: 'Use as Git Submodule', link: '/en/guide/how-to-use-as-git-submodule'},
        {text: 'Use by CMake find_package', link: '/en/guide/how-to-use-by-cmake-find_package'}
      ]
    },
    {
      text: 'struct_pack',
      collapsible: true,
      items: [
        {
          text: 'What is struct_pack?', link: '/en/guide/struct-pack-intro'
        },
        {
          text: 'struct_pack layout', link: '/en/guide/struct-pack-layout'
        },
        {
          text: 'struct_pack type system', link: '/en/guide/struct-pack-type-system'
        },
        {
          text: 'API Reference',
          link: 'https://alibaba.github.io/yalantinglibs/html/group__struct__pack.html'
        }
      ]
    },
    {
      text: 'struct_pb',
      collapsible: true,
      items: [
        {
          text: 'What is struct_pb?', link: '/en/guide/struct-pb-intro'
        },
        {
          text: 'Quick Start', link: '/en/guide/struct-pb-quick-start'
        },
        {
          text: 'Supported Features', link: '/en/guide/struct-pb-supported-features'
        },
        {
          text: 'Guide (proto3)', link: '/en/guide/struct-pb-guide-proto3'
        },
        {
          text: 'Generating your struct', link: '/en/guide/struct-pb-generating-your-struct'
        },
        {
          text: 'struct_pb API', link: '/en/guide/struct-pb-api'
        },
      ]
    },
    {
      text: 'coro_rpc',
      collapsible: true,
      items: [
        {
          text: 'What is coro_rpc?', link: '/en/guide/coro-rpc-intro'
        },
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
        {
          text: 'struct_pack简介', link: '/zh/guide/struct-pack-intro'
        },
        {
          text: 'struct_pack类型系统', link: '/zh/guide/struct-pack-type-system'
        },
        {
          text: 'struct_pack布局', link: '/zh/guide/struct-pack-layout'
        },
        {
          text: 'API Reference',
          link: 'https://alibaba.github.io/yalantinglibs/cn/html/group__struct__pack.html'
        }
      ]
    },
    {
      text: 'coro_rpc',
      collapsible: true,
      items: [
        {
          text: 'coro_rpc简介', link: '/zh/guide/coro-rpc-intro'
        },
      ]
    }
  ]
}