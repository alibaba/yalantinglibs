/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author's Email: metabeyond@outlook.com
 * Author's Github: https://github.com/refvalue/
 * Description: this source file contains an implementation of a compile-time
 * meta-string that can be utilized as a non-type parameter of a template.
 */
#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

namespace refvalue {
template <std::size_t N>
struct meta_string {
  std::array<char, N + 1> elements;

  consteval meta_string() noexcept : elements{} {}

  consteval meta_string(const char (&data)[N + 1]) noexcept
      : elements{std::to_array(data)} {}

  template <std::size_t... Ns>
  consteval meta_string(std::span<const char, Ns>... data) noexcept
      : elements{} {
    auto iter = elements.begin();

    ((iter = std::copy(data.begin(), data.end(), iter)), ...);
  }

  template <std::size_t... Ns>
  consteval meta_string(const meta_string<Ns>&... data) noexcept : elements{} {
    auto iter = elements.begin();

    ((iter = std::copy(data.begin(), data.end(), iter)), ...);
  }

  template <std::same_as<char>... Ts>
  consteval meta_string(Ts... chars) noexcept requires(sizeof...(Ts) == N)
      : elements{chars...} {}

  constexpr char& operator[](std::size_t index) noexcept {
    return elements[index];
  }

  constexpr const char& operator[](std::size_t index) const noexcept {
    return elements[index];
  }

  constexpr operator std::string_view() const noexcept {
    return std::string_view{elements.data(), size()};
  }

  constexpr bool empty() const noexcept { return size() == 0; }

  constexpr std::size_t size() const noexcept { return N; }

  constexpr char& front() noexcept { return elements.front(); }

  constexpr const char& front() const noexcept { return elements.front(); }

  constexpr char& back() noexcept { return elements[size() - 1]; }

  constexpr const char& back() const noexcept { return elements[size() - 1]; }

  constexpr auto begin() noexcept { return elements.begin(); }

  constexpr auto begin() const noexcept { return elements.begin(); }

  constexpr auto end() noexcept { return elements.begin() + size(); }

  constexpr auto end() const noexcept { return elements.begin() + size(); }

  constexpr char* data() noexcept { return elements.data(); }

  constexpr const char* data() const noexcept { return elements.data(); };

  constexpr const char* c_str() const noexcept { return elements.data(); }

  constexpr bool contains(char c) const noexcept {
    return std::find(begin(), end(), c) != end();
  }

  constexpr bool contains(std::string_view str) const noexcept {
    return str.size() <= size()
               ? std::search(begin(), end(), str.begin(), str.end()) != end()
               : false;
  }
};

template <std::size_t N>
meta_string(const char (&)[N]) -> meta_string<N - 1>;

template <std::size_t... Ns>
meta_string(std::span<const char, Ns>...) -> meta_string<(Ns + ...)>;

template <std::size_t... Ns>
meta_string(const meta_string<Ns>&...) -> meta_string<(Ns + ...)>;

template <std::same_as<char>... Ts>
meta_string(Ts...) -> meta_string<sizeof...(Ts)>;

template <std::size_t M, std::size_t N>
constexpr auto operator<=>(const meta_string<M>& left,
                           const meta_string<N>& right) noexcept {
  return static_cast<std::string_view>(left).compare(
             static_cast<std::string_view>(right)) <=> 0;
}

template <std::size_t M, std::size_t N>
constexpr bool operator==(const meta_string<M>& left,
                          const meta_string<N>& right) noexcept {
  return static_cast<std::string_view>(left) ==
         static_cast<std::string_view>(right);
}

template <std::size_t M, std::size_t N>
consteval auto operator+(const meta_string<M>& left,
                         const meta_string<N>& right) noexcept {
  return meta_string{left, right};
}

template <meta_string S, meta_string Delim>
struct split_of {
  static constexpr auto value = [] {
    constexpr std::string_view view{S};
    constexpr auto group_count = std::count_if(S.begin(), S.end(),
                                               [](char c) {
                                                 return Delim.contains(c);
                                               }) +
                                 1;
    std::array<std::string_view, group_count> result{};

    auto iter = result.begin();

    for (std::size_t start_index = 0, end_index = view.find_first_of(Delim);;
         start_index = end_index + 1,
                     end_index = view.find_first_of(Delim, start_index)) {
      *(iter++) = view.substr(start_index, end_index - start_index);

      if (end_index == std::string_view::npos) {
        break;
      }
    }

    return result;
  }();
};

template <meta_string S, meta_string Delim>
inline constexpr auto&& split_of_v = split_of<S, Delim>::value;

template <meta_string S, meta_string Delim>
struct split {
  static constexpr std::string_view view{S};
  static constexpr auto value = [] {
    constexpr auto group_count = [] {
      std::size_t count{};
      std::size_t index{};

      while ((index = view.find(Delim, index)) != std::string_view::npos) {
        count++;
        index += Delim.size();
      }

      return count + 1;
    }();
    std::array<std::string_view, group_count> result{};

    auto iter = result.begin();

    for (std::size_t start_index = 0, end_index = view.find(Delim);;
         start_index = end_index + Delim.size(),
                     end_index = view.find(Delim, start_index)) {
      *(iter++) = view.substr(start_index, end_index - start_index);

      if (end_index == std::string_view::npos) {
        break;
      }
    }

    return result;
  }();
};

template <meta_string S, meta_string Delim>
inline constexpr auto&& split_v = split<S, Delim>::value;

template <meta_string S, char C>
struct remove_char {
  static constexpr auto value = [] {
    struct removal_metadata {
      decltype(S) result;
      std::size_t actual_size;
    };

    constexpr auto metadata = [] {
      auto result = S;
      auto removal_end = std::remove(result.begin(), result.end(), C);

      return removal_metadata{
          .result{std::move(result)},
          .actual_size{static_cast<std::size_t>(removal_end - result.begin())}};
    }();

    meta_string<metadata.actual_size> result;

    std::copy(metadata.result.begin(),
              metadata.result.begin() + metadata.actual_size, result.begin());

    return result;
  }();
};

template <meta_string S, char C>
inline constexpr auto&& remove_char_v = remove_char<S, C>::value;

template <meta_string S, meta_string Removal>
struct remove {
  static constexpr auto groups = split_v<S, Removal>;
  static constexpr auto value = [] {
    return []<std::size_t... Is>(std::index_sequence<Is...>) {
      return meta_string{std::span<const char, groups[Is].size()>{
          groups[Is].data(), groups[Is].size()}...};
    }
    (std::make_index_sequence<groups.size()>{});
  }();
};

template <meta_string S, meta_string Removal>
inline constexpr auto&& remove_v = remove<S, Removal>::value;
}  // namespace refvalue
