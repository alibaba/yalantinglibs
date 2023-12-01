/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
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
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#if __has_include(<span>)
#include <span>
#endif

#include "derived_helper.hpp"
#include "foreach_macro.h"
#include "marco.h"
#include "util.h"

namespace struct_pack {

enum sp_config : uint64_t {
  DEFAULT = 0,
  DISABLE_TYPE_INFO = 0b1,
  ENABLE_TYPE_INFO = 0b10,
  DISABLE_ALL_META_INFO = 0b11,
  ENCODING_WITH_VARINT = 0b100,
  USE_FAST_VARINT = 0b1000
};

namespace detail {

template <typename... Args>
using get_args_type = remove_cvref_t<typename std::conditional<
    sizeof...(Args) == 1, std::tuple_element_t<0, std::tuple<Args...>>,
    std::tuple<Args...>>::type>;

template <typename U>
constexpr auto get_types();

template <typename T, template <typename, typename, std::size_t> typename Op,
          typename... Contexts, std::size_t... I>
constexpr void for_each_impl(std::index_sequence<I...>, Contexts &...contexts) {
  using type = decltype(get_types<T>());
  (Op<T, std::tuple_element_t<I, type>, I>{}(contexts...), ...);
}

template <typename T, template <typename, typename, std::size_t> typename Op,
          typename... Contexts>
constexpr void for_each(Contexts &...contexts) {
  using type = decltype(get_types<T>());
  for_each_impl<T, Op>(std::make_index_sequence<std::tuple_size_v<type>>(),
                       contexts...);
}

template <typename T>
constexpr std::size_t members_count();
template <typename T>
constexpr std::size_t pack_align();
template <typename T>
constexpr std::size_t alignment();
}  // namespace detail

template <typename T>
constexpr std::size_t members_count = detail::members_count<T>();
template <typename T>
constexpr std::size_t pack_alignment_v = 0;
template <typename T>
constexpr std::size_t alignment_v = 0;

#if __cpp_concepts >= 201907L

template <typename T>
concept writer_t = requires(T t) {
  t.write((const char *)nullptr, std::size_t{});
};

template <typename T>
concept reader_t = requires(T t) {
  t.read((char *)nullptr, std::size_t{});
  t.ignore(std::size_t{});
  t.tellg();
};

template <typename T>
concept view_reader_t = reader_t<T> && requires(T t) {
  { t.read_view(std::size_t{}) } -> std::convertible_to<const char *>;
};

#else

template <typename T, typename = void>
struct writer_t_impl : std::false_type {};

template <typename T>
struct writer_t_impl<T, std::void_t<decltype(std::declval<T>().write(
                            (const char *)nullptr, std::size_t{}))>>
    : std::true_type {};

template <typename T>
constexpr bool writer_t = writer_t_impl<T>::value;

template <typename T, typename = void>
struct reader_t_impl : std::false_type {};

template <typename T>
struct reader_t_impl<
    T, std::void_t<decltype(std::declval<T>().read((char *)nullptr,
                                                   std::size_t{})),
                   decltype(std::declval<T>().ignore(std::size_t{})),
                   decltype(std::declval<T>().tellg())>> : std::true_type {};

template <typename T>
constexpr bool reader_t = reader_t_impl<T>::value;

template <typename T, typename = void>
struct view_reader_t_impl : std::false_type {};

template <typename T>
struct view_reader_t_impl<
    T, std::void_t<decltype(std::declval<T>().read_view(std::size_t{}))>>
    : std::true_type {};

template <typename T>
constexpr bool view_reader_t = reader_t<T> &&view_reader_t_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L

template <typename T>
concept check_reader_t = reader_t<T> && requires(T t) {
  t.check(std::size_t{});
};

template <typename T>
concept can_reserve = requires(T t) {
  t.reserve(std::size_t{});
};

template <typename T>
concept can_shrink_to_fit = requires(T t) {
  t.shrink_to_fit();
};

#else

template <typename T, typename = void>
struct check_reader_t_impl : std::false_type {};

template <typename T>
struct check_reader_t_impl<
    T, std::void_t<decltype(std::declval<T>().check(std::size_t{}))>>
    : std::true_type {};

template <typename T>
constexpr bool check_reader_t = reader_t<T> &&check_reader_t_impl<T>::value;

template <typename T, typename = void>
struct can_reserve_impl : std::false_type {};

template <typename T>
struct can_reserve_impl<
    T, std::void_t<decltype(std::declval<T>().reserve(std::size_t{}))>>
    : std::true_type {};

template <typename T>
constexpr bool can_reserve = can_reserve_impl<T>::value;

template <typename T, typename = void>
struct can_shrink_to_fit_impl : std::false_type {};

template <typename T>
struct can_shrink_to_fit_impl<
    T, std::void_t<decltype(std::declval<T>().shrink_to_fit())>>
    : std::true_type {};

template <typename T>
constexpr bool can_shrink_to_fit = can_shrink_to_fit_impl<T>::value;

#endif

template <typename T, uint64_t version = 0>
struct compatible;

// clang-format off
namespace detail {

#if __cpp_concepts >= 201907L

template <typename T>
concept has_user_defined_id = requires {
  typename std::integral_constant<std::size_t, T::struct_pack_id>;
};

template <typename T>
concept has_user_defined_id_ADL = requires {
  typename std::integral_constant<std::size_t,
                                  struct_pack_id((T*)nullptr)>;
};

#else

template <typename T, typename = void>
struct has_user_defined_id_impl : std::false_type {};

template <typename T>
struct has_user_defined_id_impl<
    T, std::void_t<std::integral_constant<std::size_t, T::struct_pack_id>>>
    : std::true_type {};

template <typename T>
constexpr bool has_user_defined_id = has_user_defined_id_impl<T>::value;

template <std::size_t sz>
struct constant_checker{};

template <typename T, typename = void>
struct has_user_defined_id_ADL_impl : std::false_type {};

#ifdef _MSC_VER
// FIXME: we can't check if it's compile-time calculated in msvc with C++17
template <typename T>
struct has_user_defined_id_ADL_impl<
    T, std::void_t<decltype(struct_pack_id((T*)nullptr))>>
    : std::true_type {};
#else

template <typename T>
struct has_user_defined_id_ADL_impl<
    T, std::void_t<constant_checker<struct_pack_id((T*)nullptr)>>>
    : std::true_type {};

#endif

template <typename T>
constexpr bool has_user_defined_id_ADL = has_user_defined_id_ADL_impl<T>::value;

#endif



#if __cpp_concepts >= 201907L
template <typename T>
concept is_base_class = requires (T* t) {
  std::is_same_v<uint32_t,decltype(t->get_struct_pack_id())>;
  typename struct_pack::detail::derived_class_set_t<T>;
};
#else
template <typename T, typename = void>
struct is_base_class_impl : std::false_type {};

template <typename T>
struct is_base_class_impl<
    T, std::void_t<
    std::enable_if<std::is_same_v<decltype(((T*)nullptr)->get_struct_pack_id()), uint32_t>>,
    typename struct_pack::detail::derived_class_set_t<T>>>
    : std::true_type {};
template <typename T>
constexpr bool is_base_class=is_base_class_impl<T>::value;

#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept deserialize_view = requires(Type container) {
    container.size();
    container.data();
  };
#else

template <typename T, typename = void>
struct deserialize_view_impl : std::false_type {};
template <typename T>
struct deserialize_view_impl<
    T, std::void_t<decltype(std::declval<T>().size()),decltype(std::declval<T>().data())>>
    : std::true_type {};

template <typename Type>
constexpr bool deserialize_view = deserialize_view_impl<Type>::value;

#endif

  struct memory_writer {
    char *buffer;
    STRUCT_PACK_INLINE void write(const char *data, std::size_t len) {
      memcpy(buffer, data, len);
      buffer += len;
    }
  };


#if __cpp_concepts >= 201907L
  template <typename Type>
  concept container_adapter = requires(Type container) {
    typename remove_cvref_t<Type>::value_type;
    container.size();
    container.pop();
  };
#else
  template <typename T, typename = void>
  struct container_adapter_impl : std::false_type {};

  template <typename T>
  struct container_adapter_impl<T, std::void_t<
    typename remove_cvref_t<T>::value_type,
    decltype(std::declval<T>().size()),
    decltype(std::declval<T>().pop())>>
      : std::true_type {};

  template <typename T>
  constexpr bool container_adapter = container_adapter_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept container = requires(Type container) {
    typename remove_cvref_t<Type>::value_type;
    container.size();
    container.begin();
    container.end();
  };
#else
  template <typename T, typename = void>
  struct container_impl : std::false_type {};

  template <typename T>
  struct container_impl<T, std::void_t<
    typename remove_cvref_t<T>::value_type,
    decltype(std::declval<T>().size()),
    decltype(std::declval<T>().begin()),
    decltype(std::declval<T>().end())>>
      : std::true_type {};

  template <typename T>
  constexpr bool container = container_impl<T>::value;
#endif  

  template <typename Type>
  constexpr bool is_char_t = std::is_same_v<Type, signed char> ||
      std::is_same_v<Type, char> || std::is_same_v<Type, unsigned char> ||
      std::is_same_v<Type, wchar_t> || std::is_same_v<Type, char16_t> ||
      std::is_same_v<Type, char32_t>
#ifdef __cpp_lib_char8_t
 || std::is_same_v<Type, char8_t>
#endif
;


#if __cpp_concepts >= 201907L
  template <typename Type>
  concept string = container<Type> && requires(Type container) {
    requires is_char_t<typename remove_cvref_t<Type>::value_type>;
    container.length();
    container.data();
  };
#else
  template <typename T, typename = void>
  struct string_impl : std::false_type {};

  template <typename T>
  struct string_impl<T, std::void_t<
    std::enable_if_t<is_char_t<typename remove_cvref_t<T>::value_type>>,
    decltype(std::declval<T>().length()),
    decltype(std::declval<T>().data())>> 
      : std::true_type {};

  template <typename T>
  constexpr bool string = string_impl<T>::value && container<T>;
#endif  

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept string_view = string<Type> && !requires(Type container) {
    container.resize(std::size_t{});
  };
#else
  template <typename T, typename = void>
  struct string_view_impl : std::true_type {};

  template <typename T>
  struct string_view_impl<T, std::void_t<
    decltype(std::declval<T>().resize(std::size_t{}))>> 
      : std::false_type {};

  template <typename T>
  constexpr bool string_view = string<T> && string_view_impl<T>::value;
#endif  

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept span = container<Type> && requires(Type t) {
    Type{(typename Type::value_type*)nullptr ,std::size_t{} };
    t.subspan(std::size_t{},std::size_t{});
  };
#else
  template <typename T, typename = void>
  struct span_impl : std::false_type {};

  template <typename T>
  struct span_impl<T, std::void_t<
    decltype(T{(typename T::value_type*)nullptr ,std::size_t{}}),
    decltype(std::declval<T>().subspan(std::size_t{},std::size_t{}))>> 
      : std::true_type {};

  template <typename T>
  constexpr bool span = container<T> && span_impl<T>::value;
#endif 


#if __cpp_concepts >= 201907L
  template <typename Type>
  concept dynamic_span = span<Type> && Type::extent == SIZE_MAX;
#else
  template <typename T, typename = void>
  struct dynamic_span_impl : std::false_type {};

  template <typename T>
  struct dynamic_span_impl<T, std::void_t<
    std::enable_if_t<(T::extent == SIZE_MAX)>>> 
      : std::true_type {};

  template <typename T>
  constexpr bool dynamic_span = span<T> && dynamic_span_impl<T>::value;
#endif
  template <typename Type>
  constexpr bool static_span = span<Type> && !dynamic_span<Type>;


#if __cpp_lib_span >= 202002L && __cpp_concepts>=201907L

  template <typename Type>
  concept continuous_container =
      string<Type> || (container<Type> && requires(Type container) {
        std::span{container};
      });

#else

  template <typename Type>
  constexpr inline bool is_std_basic_string_v = false;

  template <typename... args>
  constexpr inline bool is_std_basic_string_v<std::basic_string<args...>> =
      true;

  template <typename Type>
  constexpr inline bool is_std_vector_v = false;

  template <typename... args>
  constexpr inline bool is_std_vector_v<std::vector<args...>> = true;

  template <typename Type>
  constexpr bool continuous_container =
      string<Type> || (container<Type> && (is_std_vector_v<Type> || is_std_basic_string_v<Type>));
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept map_container = container<Type> && requires(Type container) {
    typename remove_cvref_t<Type>::mapped_type;
  };
#else
template <typename T, typename = void>
  struct map_container_impl : std::false_type {};

  template <typename T>
  struct map_container_impl<T, std::void_t<
    typename remove_cvref_t<T>::mapped_type>> 
      : std::true_type {};

  template <typename T>
  constexpr bool map_container = container<T> && map_container_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept set_container = container<Type> && requires {
    typename remove_cvref_t<Type>::key_type;
  };
#else
  template <typename T, typename = void>
  struct set_container_impl : std::false_type {};

  template <typename T>
  struct set_container_impl<T, std::void_t<
    typename remove_cvref_t<T>::key_type>> 
      : std::true_type {};

  template <typename T>
  constexpr bool set_container = container<T> && set_container_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept bitset = requires (Type t){
    t.flip();
    t.set();
    t.reset();
    t.count();
  } && (Type{}.size()+7)/8 == sizeof(Type);
#else
  template <typename T, typename = void>
  struct bitset_impl : std::false_type {};


  template <typename T>
  struct bitset_impl<T, std::void_t<
    decltype(std::declval<T>().flip()),
    decltype(std::declval<T>().set()),
    decltype(std::declval<T>().reset()),
    decltype(std::declval<T>().count()),
    decltype(std::declval<T>().size())>> 
      : std::true_type {};

  template<typename T>
  constexpr bool bitset_size_checker() {
    if constexpr (bitset_impl<T>::value) {
      return (T{}.size()+7)/8==sizeof(T);
    }
    else {
      return false;
    }
  }

  template <typename T>
  constexpr bool bitset = bitset_impl<T>::value && bitset_size_checker<T>();
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept tuple = requires(Type tuple) {
    std::get<0>(tuple);
    sizeof(std::tuple_size<remove_cvref_t<Type>>);
  };
#else
template <typename T, typename = void>
  struct tuple_impl : std::false_type {};

  template <typename T>
  struct tuple_impl<T, std::void_t<
    decltype(std::get<0>(std::declval<T>())),
    decltype(sizeof(std::tuple_size<remove_cvref_t<T>>::value))>> 
      : std::true_type {};

  template <typename T>
  constexpr bool tuple = tuple_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept user_defined_refl = std::is_same_v<decltype(STRUCT_PACK_REFL_FLAG(std::declval<Type&>())),Type&>;
#else
  template <typename T, typename = void>
  struct user_defined_refl_impl : std::false_type {};

  template <typename T>
  struct user_defined_refl_impl<T, std::void_t<
    std::enable_if_t<std::is_same_v<decltype(STRUCT_PACK_REFL_FLAG(std::declval<T&>())),T&>>>>
      : std::true_type {};

  template <typename T>
  constexpr bool user_defined_refl = user_defined_refl_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept user_defined_config_by_ADL = std::is_same_v<decltype(set_sp_config(std::declval<Type*>())),struct_pack::sp_config>;
#else
  template <typename T, typename = void>
  struct user_defined_config_by_ADL_impl : std::false_type {};

  template <typename T>
  struct user_defined_config_by_ADL_impl<T, std::void_t<
    std::enable_if_t<std::is_same_v<decltype(set_sp_config(std::declval<T*>())),struct_pack::sp_config>>>>
      : std::true_type {};

  template <typename T>
  constexpr bool user_defined_config_by_ADL = user_defined_config_by_ADL_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept user_defined_config = requires {
    Type::struct_pack_config;
  };
#else
  template <typename T, typename = void>
  struct user_defined_config_impl : std::false_type {};

  template <typename T>
  struct user_defined_config_impl<T, std::void_t<decltype(T::struct_pack_config)>>
      : std::true_type {};

  template <typename T>
  constexpr bool user_defined_config = user_defined_config_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept tuple_size = requires(Type tuple) {
    std::tuple_size<remove_cvref_t<Type>>::value;
  };
#else
  template <typename T, typename = void>
  struct tuple_size_impl : std::false_type {};

  template <typename T>
  struct tuple_size_impl<T, std::void_t<
    decltype(std::tuple_size<remove_cvref_t<T>>::value)>> 
      : std::true_type {};

  template <typename T>
  constexpr bool tuple_size = tuple_size_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept array = requires(Type arr) {
    arr.size();
    std::tuple_size<remove_cvref_t<Type>>{};
  };
#else
  template <typename T, typename = void>
  struct array_impl : std::false_type {};

  template <typename T>
  struct array_impl<T, std::void_t<
    decltype(std::declval<T>().size()),
    decltype(std::tuple_size<remove_cvref_t<T>>{})>> 
      : std::true_type {};

  template <typename T>
  constexpr bool array = array_impl<T>::value;
#endif


  template <class T>
  constexpr bool c_array =
      std::is_array_v<T> && std::extent_v<remove_cvref_t<T>> > 0;

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept pair = requires(Type p) {
    typename remove_cvref_t<Type>::first_type;
    typename remove_cvref_t<Type>::second_type;
    p.first;
    p.second;
  };
#else
  template <typename T, typename = void>
  struct pair_impl : std::false_type {};

  template <typename T>
  struct pair_impl<T, std::void_t<
    typename remove_cvref_t<T>::first_type,
    typename remove_cvref_t<T>::second_type,
    decltype(std::declval<T>().first),
    decltype(std::declval<T>().second)>> 
      : std::true_type {};

  template <typename T>
  constexpr bool pair = pair_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept expected = requires(Type e) {
    typename remove_cvref_t<Type>::value_type;
    typename remove_cvref_t<Type>::error_type;
    typename remove_cvref_t<Type>::unexpected_type;
    e.has_value();
    e.error();
    requires std::is_same_v<void,
                            typename remove_cvref_t<Type>::value_type> ||
        requires(Type e) {
      e.value();
    };
  };
#else
  template <typename T, typename = void>
  struct expected_impl : std::false_type {};

  template <typename T>
  struct expected_impl<T, std::void_t<
    typename remove_cvref_t<T>::value_type,
    typename remove_cvref_t<T>::error_type,
    typename remove_cvref_t<T>::unexpected_type,
    decltype(std::declval<T>().has_value()),
    decltype(std::declval<T>().error())>> 
      : std::true_type {};
    //TODO: check e.value()
  template <typename T>
  constexpr bool expected = expected_impl<T>::value;
#endif

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept unique_ptr = requires(Type ptr) {
    ptr.operator*();
    typename remove_cvref_t<Type>::element_type;
  }
  &&!requires(Type ptr, Type ptr2) { ptr = ptr2; };
#else
  template <typename T, typename = void>
  struct unique_ptr_impl : std::false_type {};

  template <typename T>
  struct unique_ptr_impl<T, std::void_t<
    typename remove_cvref_t<T>::element_type,
    decltype(std::declval<T>().operator*())>> 
      : std::true_type {};

  template <typename T>
  constexpr bool unique_ptr = unique_ptr_impl<T>::value;
#endif


#if __cpp_concepts >= 201907L
  template <typename Type>
  concept optional = !expected<Type> && requires(Type optional) {
    optional.value();
    optional.has_value();
    optional.operator*();
    typename remove_cvref_t<Type>::value_type;
  };
#else
  template <typename T, typename = void>
  struct optional_impl : std::false_type {};

  template <typename T>
  struct optional_impl<T, std::void_t<
    decltype(std::declval<T>().value()),
    decltype(std::declval<T>().has_value()),
    decltype(std::declval<T>().operator*()),
    typename remove_cvref_t<T>::value_type>> 
      : std::true_type {};

  template <typename T>
  constexpr bool optional = !expected<T> && optional_impl<T>::value;
#endif





  template <typename Type>
  constexpr inline bool is_compatible_v = false;

  template <typename Type, uint64_t version>
  constexpr inline bool is_compatible_v<compatible<Type,version>> = true;

  template <typename Type>
  constexpr inline bool is_variant_v = false;

  template <typename... args>
  constexpr inline bool is_variant_v<std::variant<args...>> = true;

  template <typename T>
  constexpr inline bool is_trivial_tuple = false;

  template <typename T>
  class varint;

  template <typename T>
  class sint;

  template <typename T>
  constexpr bool varintable_t =
      std::is_same_v<T, varint<int32_t>> || std::is_same_v<T, varint<int64_t>> ||
      std::is_same_v<T, varint<uint32_t>> || std::is_same_v<T, varint<uint64_t>>;
  template <typename T>
  constexpr bool sintable_t =
      std::is_same_v<T, sint<int32_t>> || std::is_same_v<T, sint<int64_t>>;

  template <typename T, uint64_t parent_tag = 0 >
  constexpr bool varint_t = varintable_t<T> || sintable_t<T> || ((parent_tag&struct_pack::ENCODING_WITH_VARINT) && (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T,int64_t> || std::is_same_v<T,uint64_t>));

  template <typename Type>
  constexpr inline bool is_trivial_view_v = false;

  template <typename... Args>
  constexpr uint64_t get_parent_tag();

  template <typename T, bool ignore_compatible_field = false, uint64_t parent_tag = 0>
  struct is_trivial_serializable {
    private:
      template<typename U,uint64_t parent_tag_=0, std::size_t... I>
      static constexpr bool class_visit_helper(std::index_sequence<I...>) {
        return (is_trivial_serializable<std::tuple_element_t<I, U>,
                                            ignore_compatible_field,parent_tag_>::value &&
                    ...);
      }
      static constexpr bool solve() {
        if constexpr (std::is_same_v<T,std::monostate>) {
          return true;
        }
        else if constexpr (std::is_abstract_v<T>) {
          return false;
        }
        else if constexpr (varint_t<T,parent_tag>) {
          return false;
        }
        else if constexpr (is_compatible_v<T> || is_trivial_view_v<T>) {
          return ignore_compatible_field;
        }
        else if constexpr (std::is_enum_v<T> || std::is_fundamental_v<T>  || bitset<T>
#if (__GNUC__ || __clang__) && defined(STRUCT_PACK_ENABLE_INT128)
        || std::is_same_v<__int128,T> || std::is_same_v<unsigned __int128,T>
#endif
        ) {
          return true;
        }
        else if constexpr (array<T>) {
          return is_trivial_serializable<typename T::value_type,
                                        ignore_compatible_field>::value;
        }
        else if constexpr (c_array<T>) {
          return is_trivial_serializable<typename std::remove_all_extents<T>::type,
                                        ignore_compatible_field>::value;
        }
        else if constexpr (!pair<T> && tuple<T> && !is_trivial_tuple<T>) {
          return false;
        }
        else if constexpr (user_defined_refl<T>) {
          return false;
        }
        else if constexpr (container<T> || optional<T> || is_variant_v<T> ||
                          unique_ptr<T> || expected<T> || container_adapter<T>) {
          return false;
        }
        else if constexpr (pair<T>) {
          return is_trivial_serializable<typename T::first_type,
                                        ignore_compatible_field>::value &&
                is_trivial_serializable<typename T::second_type,
                                        ignore_compatible_field>::value;
        }
        else if constexpr (is_trivial_tuple<T>) {
          return class_visit_helper<T>(std::make_index_sequence<std::tuple_size_v<T>>{});
        }
        else if constexpr (std::is_class_v<T>) {
          constexpr auto tag = get_parent_tag<T>();
          using U = decltype(get_types<T>());
          return class_visit_helper<U , tag>(std::make_index_sequence<std::tuple_size_v<U>>{});
        }
        else
          return false;
      }

    public:
      static inline constexpr bool value = is_trivial_serializable::solve();
  };

}
template <typename T, typename = std::enable_if_t<detail::is_trivial_serializable<T>::value>>
struct trivial_view;
namespace detail {

#if __cpp_concepts < 201907L

template <typename T, typename = void>
struct trivially_copyable_container_impl : std::false_type {};

template <typename T>
struct trivially_copyable_container_impl<T, std::void_t<std::enable_if_t<is_trivial_serializable<typename T::value_type>::value>>>
    : std::true_type {};

template <typename Type>
constexpr bool trivially_copyable_container =
    continuous_container<Type> && trivially_copyable_container_impl<Type>::value;

#else

template <typename Type>
constexpr bool trivially_copyable_container =
    continuous_container<Type> &&
    requires(Type container) {
      requires is_trivial_serializable<typename Type::value_type>::value;
    };

#endif

  template <typename Type>
  constexpr inline bool is_trivial_view_v<struct_pack::trivial_view<Type>> = true;

  struct UniversalVectorType {
    template <typename T>
    operator std::vector<T>();
  };

  struct UniversalType {
    template <typename T>
    operator T();
  };

  struct UniversalIntegralType {
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    operator T();
  };

  struct UniversalNullptrType {
    operator std::nullptr_t();
  };

  struct UniversalOptionalType {
  template <typename U, typename = std::enable_if_t<optional<U>>>
  operator U();
  };

  struct UniversalCompatibleType {
    template <typename U, typename = std::enable_if_t<is_compatible_v<U>>>
    operator U();
  };

  template <typename T, typename construct_param_t, typename = void, typename... Args>
  struct is_constructable_impl : std::false_type {};
  template <typename T, typename construct_param_t, typename... Args>
  struct is_constructable_impl<T, construct_param_t,
  std::void_t<
    decltype(T{{Args{}}..., {construct_param_t{}}})>, Args...>
      : std::true_type {};

  template <typename T, typename construct_param_t, typename... Args>
  constexpr bool is_constructable=is_constructable_impl<T,construct_param_t,void,Args...>::value;

  template <typename T, typename... Args>
  constexpr std::size_t members_count_impl() {
    if constexpr (is_constructable<T,UniversalVectorType,Args...>) {
      return members_count_impl<T, Args..., UniversalVectorType>();
    }
    else if constexpr (is_constructable<T,UniversalType,Args...>) {
      return members_count_impl<T, Args..., UniversalType>();
    }
    else if constexpr (is_constructable<T,UniversalOptionalType,Args...>) {
      return members_count_impl<T, Args..., UniversalOptionalType>();
    }
    else if constexpr (is_constructable<T,UniversalIntegralType,Args...>) {
      return members_count_impl<T, Args..., UniversalIntegralType>();
    }
    else if constexpr (is_constructable<T,UniversalNullptrType,Args...>) {
      return members_count_impl<T, Args..., UniversalNullptrType>();
    }
    else if constexpr (is_constructable<T,UniversalCompatibleType,Args...>) {
      return members_count_impl<T, Args..., UniversalCompatibleType>();
    }
    else {
      return sizeof...(Args);
    }
  }

  template <typename T>
  constexpr std::size_t members_count() {
    using type = remove_cvref_t<T>;
    if constexpr (user_defined_refl<type>) {
      return decltype(STRUCT_PACK_FIELD_COUNT(std::declval<type>()))::value;
    }
    else if constexpr (tuple_size<type>) {
      return std::tuple_size<type>::value;
    }
    else {
      return members_count_impl<type>();
    }
  }

  constexpr static auto MaxVisitMembers = 64;

  template<typename Object,typename Visitor>
  constexpr decltype(auto) STRUCT_PACK_INLINE visit_members_by_user_defined_refl(Object &&object,
                                                            Visitor &&visitor);

  template<typename Object,typename Visitor>
  constexpr decltype(auto) STRUCT_PACK_INLINE visit_members_by_structure_binding(Object &&object,
                                                            Visitor &&visitor);

  template<typename Object,typename Visitor>
  constexpr decltype(auto) STRUCT_PACK_INLINE visit_members(Object &&object,
                                                            Visitor &&visitor) {
    using type = remove_cvref_t<decltype(object)>;
    if constexpr (user_defined_refl<type>) {
      return visit_members_by_user_defined_refl(object,visitor);
    }
    else {
      return visit_members_by_structure_binding(object,visitor);
    }
  }

  template<typename Object,typename Visitor>
  constexpr decltype(auto) STRUCT_PACK_INLINE visit_members_by_user_defined_refl(Object &&object,
                                                            Visitor &&visitor) {
    using type = remove_cvref_t<decltype(object)>;
    constexpr auto Count = decltype(STRUCT_PACK_FIELD_COUNT(object))::value;
    
    static_assert(Count <= MaxVisitMembers, "exceed max visit members");
    if constexpr (Count >= 0) {
      if constexpr (Count==1) {  return visitor(STRUCT_PACK_GET_0(object));
      }
      else if constexpr (Count==2) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object));
      }
      else if constexpr (Count==3) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object));
      }
      else if constexpr (Count==4) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object));
      }
      else if constexpr (Count==5) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object));
      }
      else if constexpr (Count==6) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object));
      }
      else if constexpr (Count==7) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object));
      }
      else if constexpr (Count==8) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object));
      }
      else if constexpr (Count==9) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object));
      }
      else if constexpr (Count==10) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object));
      }
      else if constexpr (Count==11) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object));
      }
      else if constexpr (Count==12) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object));
      }
      else if constexpr (Count==13) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object));
      }
      else if constexpr (Count==14) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object));
      }
      else if constexpr (Count==15) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object));
      }
      else if constexpr (Count==16) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object));
      }
      else if constexpr (Count==17) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object));
      }
      else if constexpr (Count==18) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object));
      }
      else if constexpr (Count==19) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object));
      }
      else if constexpr (Count==20) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object));
      }
      else if constexpr (Count==21) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object));
      }
      else if constexpr (Count==22) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object));
      }
      else if constexpr (Count==23) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object));
      }
      else if constexpr (Count==24) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object));
      }
      else if constexpr (Count==25) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object));
      }
      else if constexpr (Count==26) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object));
      }
      else if constexpr (Count==27) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object));
      }
      else if constexpr (Count==28) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object));
      }
      else if constexpr (Count==29) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object));
      }
      else if constexpr (Count==30) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object));
      }
      else if constexpr (Count==31) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object));
      }
      else if constexpr (Count==32) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object));
      }
      else if constexpr (Count==33) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object));
      }
      else if constexpr (Count==34) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object));
      }
      else if constexpr (Count==35) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object));
      }
      else if constexpr (Count==36) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object));
      }
      else if constexpr (Count==37) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object));
      }
      else if constexpr (Count==38) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object));
      }
      else if constexpr (Count==39) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object));
      }
      else if constexpr (Count==40) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object));
      }
      else if constexpr (Count==41) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object));
      }
      else if constexpr (Count==42) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object));
      }
      else if constexpr (Count==43) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object));
      }
      else if constexpr (Count==44) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object));
      }
      else if constexpr (Count==45) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object));
      }
      else if constexpr (Count==46) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object));
      }
      else if constexpr (Count==47) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object));
      }
      else if constexpr (Count==48) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object));
      }
      else if constexpr (Count==49) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object));
      }
      else if constexpr (Count==50) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object));
      }
      else if constexpr (Count==51) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object));
      }
      else if constexpr (Count==52) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object));
      }
      else if constexpr (Count==53) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object));
      }
      else if constexpr (Count==54) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object));
      }
      else if constexpr (Count==55) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object));
      }
      else if constexpr (Count==56) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object));
      }
      else if constexpr (Count==57) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object),STRUCT_PACK_GET_56(object));
      }
      else if constexpr (Count==58) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object),STRUCT_PACK_GET_56(object),STRUCT_PACK_GET_57(object));
      }
      else if constexpr (Count==59) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object),STRUCT_PACK_GET_56(object),STRUCT_PACK_GET_57(object),STRUCT_PACK_GET_58(object));
      }
      else if constexpr (Count==60) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object),STRUCT_PACK_GET_56(object),STRUCT_PACK_GET_57(object),STRUCT_PACK_GET_58(object),STRUCT_PACK_GET_59(object));
      }
      else if constexpr (Count==61) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object),STRUCT_PACK_GET_56(object),STRUCT_PACK_GET_57(object),STRUCT_PACK_GET_58(object),STRUCT_PACK_GET_59(object),STRUCT_PACK_GET_60(object));
      }
      else if constexpr (Count==62) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object),STRUCT_PACK_GET_56(object),STRUCT_PACK_GET_57(object),STRUCT_PACK_GET_58(object),STRUCT_PACK_GET_59(object),STRUCT_PACK_GET_60(object),STRUCT_PACK_GET_61(object));
      }
      else if constexpr (Count==63) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object),STRUCT_PACK_GET_56(object),STRUCT_PACK_GET_57(object),STRUCT_PACK_GET_58(object),STRUCT_PACK_GET_59(object),STRUCT_PACK_GET_60(object),STRUCT_PACK_GET_61(object),STRUCT_PACK_GET_62(object));
      }
      else if constexpr (Count==64) {  return visitor(STRUCT_PACK_GET_0(object),STRUCT_PACK_GET_1(object),STRUCT_PACK_GET_2(object),STRUCT_PACK_GET_3(object),STRUCT_PACK_GET_4(object),STRUCT_PACK_GET_5(object),STRUCT_PACK_GET_6(object),STRUCT_PACK_GET_7(object),STRUCT_PACK_GET_8(object),STRUCT_PACK_GET_9(object),STRUCT_PACK_GET_10(object),STRUCT_PACK_GET_11(object),STRUCT_PACK_GET_12(object),STRUCT_PACK_GET_13(object),STRUCT_PACK_GET_14(object),STRUCT_PACK_GET_15(object),STRUCT_PACK_GET_16(object),STRUCT_PACK_GET_17(object),STRUCT_PACK_GET_18(object),STRUCT_PACK_GET_19(object),STRUCT_PACK_GET_20(object),STRUCT_PACK_GET_21(object),STRUCT_PACK_GET_22(object),STRUCT_PACK_GET_23(object),STRUCT_PACK_GET_24(object),STRUCT_PACK_GET_25(object),STRUCT_PACK_GET_26(object),STRUCT_PACK_GET_27(object),STRUCT_PACK_GET_28(object),STRUCT_PACK_GET_29(object),STRUCT_PACK_GET_30(object),STRUCT_PACK_GET_31(object),STRUCT_PACK_GET_32(object),STRUCT_PACK_GET_33(object),STRUCT_PACK_GET_34(object),STRUCT_PACK_GET_35(object),STRUCT_PACK_GET_36(object),STRUCT_PACK_GET_37(object),STRUCT_PACK_GET_38(object),STRUCT_PACK_GET_39(object),STRUCT_PACK_GET_40(object),STRUCT_PACK_GET_41(object),STRUCT_PACK_GET_42(object),STRUCT_PACK_GET_43(object),STRUCT_PACK_GET_44(object),STRUCT_PACK_GET_45(object),STRUCT_PACK_GET_46(object),STRUCT_PACK_GET_47(object),STRUCT_PACK_GET_48(object),STRUCT_PACK_GET_49(object),STRUCT_PACK_GET_50(object),STRUCT_PACK_GET_51(object),STRUCT_PACK_GET_52(object),STRUCT_PACK_GET_53(object),STRUCT_PACK_GET_54(object),STRUCT_PACK_GET_55(object),STRUCT_PACK_GET_56(object),STRUCT_PACK_GET_57(object),STRUCT_PACK_GET_58(object),STRUCT_PACK_GET_59(object),STRUCT_PACK_GET_60(object),STRUCT_PACK_GET_61(object),STRUCT_PACK_GET_62(object),STRUCT_PACK_GET_63(object));
      }      
    }
    else  {
      static_assert(!sizeof(type), "empty struct/class is not allowed!");
    }
  }

  template<typename Object,typename Visitor>
  constexpr decltype(auto) STRUCT_PACK_INLINE visit_members_by_structure_binding(Object &&object,
                                                            Visitor &&visitor) {
    using type = remove_cvref_t<decltype(object)>;
    constexpr auto Count = struct_pack::members_count<type>;
    if constexpr (Count == 0 && std::is_class_v<type> &&
                  !std::is_same_v<type, std::monostate>) {
      static_assert(!sizeof(type), "1. If the struct is empty, which is not allowed in struct_pack type system.\n"
      "2. If the strut is not empty, it means struct_pack can't calculate your struct members' count. You can use macro STRUCT_PACK_REFL(Typename, field1, field2...).");
    }
    static_assert(Count <= MaxVisitMembers, "exceed max visit members");
    // If you see any structured binding error in the follow line, it
    // means struct_pack can't calculate your struct's members count
    // correctly. 
    // The best way is use macro STRUCT_PACK_REFL(Typename, field1, field2...)
    // See the src/struct_pack/example/non_aggregated_type.cpp for more details.
    //
    // You can also to mark it manually.
    // For example, there is a struct named Hello,
    // and it has 3 members.
    //
    // You can mark it as:
    //
    // template <>
    // constexpr size_t struct_pack::members_count<Hello> = 3;

    if constexpr (Count == 0) {
      return visitor();
    }
    else if constexpr (Count == 1) {
      auto &&[a1] = object;
      return visitor(a1);
    }
    else if constexpr (Count == 2) {
      auto &&[a1, a2] = object;
      return visitor(a1, a2);
    }
    else if constexpr (Count == 3) {
      auto &&[a1, a2, a3] = object;
      return visitor(a1, a2, a3);
    }
    else if constexpr (Count == 4) {
      auto &&[a1, a2, a3, a4] = object;
      return visitor(a1, a2, a3, a4);
    }
    else if constexpr (Count == 5) {
      auto &&[a1, a2, a3, a4, a5] = object;
      return visitor(a1, a2, a3, a4, a5);
    }
    else if constexpr (Count == 6) {
      auto &&[a1, a2, a3, a4, a5, a6] = object;
      return visitor(a1, a2, a3, a4, a5, a6);
    }
    else if constexpr (Count == 7) {
      auto &&[a1, a2, a3, a4, a5, a6, a7] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7);
    }
    else if constexpr (Count == 8) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8);
    }
    else if constexpr (Count == 9) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }
    else if constexpr (Count == 10) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }
    else if constexpr (Count == 11) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    }
    else if constexpr (Count == 12) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    }
    else if constexpr (Count == 13) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
    }
    else if constexpr (Count == 14) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14);
    }
    else if constexpr (Count == 15) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
              a15] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15);
    }
    else if constexpr (Count == 16) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16);
    }
    else if constexpr (Count == 17) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17);
    }
    else if constexpr (Count == 18) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18);
    }
    else if constexpr (Count == 19) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19);
    }
    else if constexpr (Count == 20) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20);
    }
    else if constexpr (Count == 21) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21);
    }
    else if constexpr (Count == 22) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22);
    }
    else if constexpr (Count == 23) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23);
    }
    else if constexpr (Count == 24) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24);
    }
    else if constexpr (Count == 25) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
                     a25);
    }
    else if constexpr (Count == 26) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26);
    }
    else if constexpr (Count == 27) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27);
    }
    else if constexpr (Count == 28) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28);
    }
    else if constexpr (Count == 29) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29);
    }
    else if constexpr (Count == 30) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30);
    }
    else if constexpr (Count == 31) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31);
    }
    else if constexpr (Count == 32) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32);
    }
    else if constexpr (Count == 33) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33);
    }
    else if constexpr (Count == 34) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34);
    }
    else if constexpr (Count == 35) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35);
    }
    else if constexpr (Count == 36) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36);
    }
    else if constexpr (Count == 37) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36,
                     a37);
    }
    else if constexpr (Count == 38) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38);
    }
    else if constexpr (Count == 39) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39);
    }
    else if constexpr (Count == 40) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40);
    }
    else if constexpr (Count == 41) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41);
    }
    else if constexpr (Count == 42) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42);
    }
    else if constexpr (Count == 43) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43);
    }
    else if constexpr (Count == 44) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44);
    }
    else if constexpr (Count == 45) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45);
    }
    else if constexpr (Count == 46) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46);
    }
    else if constexpr (Count == 47) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47);
    }
    else if constexpr (Count == 48) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48);
    }
    else if constexpr (Count == 49) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48,
                     a49);
    }
    else if constexpr (Count == 50) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50);
    }
    else if constexpr (Count == 51) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51);
    }
    else if constexpr (Count == 52) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52);
    }
    else if constexpr (Count == 53) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53);
    }
    else if constexpr (Count == 54) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54] =
          object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54);
    }
    else if constexpr (Count == 55) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55);
    }
    else if constexpr (Count == 56) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56);
    }
    else if constexpr (Count == 57) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57);
    }
    else if constexpr (Count == 58) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58);
    }
    else if constexpr (Count == 59) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59);
    }
    else if constexpr (Count == 60) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60);
    }
    else if constexpr (Count == 61) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60, a61] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60,
                     a61);
    }
    else if constexpr (Count == 62) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60, a61, a62] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61,
                     a62);
    }
    else if constexpr (Count == 63) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60, a61, a62, a63] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61,
                     a62, a63);
    }
    else if constexpr (Count == 64) {
      auto &&[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
              a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28,
              a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41,
              a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54,
              a55, a56, a57, a58, a59, a60, a61, a62, a63, a64] = object;
      return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,
                     a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25,
                     a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37,
                     a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49,
                     a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61,
                     a62, a63, a64);
    }
  }
// clang-format off
template <typename Func, typename... Args>
constexpr decltype(auto) STRUCT_PACK_INLINE template_switch(std::size_t index,
                                                            Args &&...args) {
  switch (index) {
    case 0:
      return Func::template run<0>(std::forward<Args>(args)...);
    case 1:
      return Func::template run<1>(std::forward<Args>(args)...);
    case 2:
      return Func::template run<2>(std::forward<Args>(args)...);
    case 3:
      return Func::template run<3>(std::forward<Args>(args)...);
    case 4:
      return Func::template run<4>(std::forward<Args>(args)...);
    case 5:
      return Func::template run<5>(std::forward<Args>(args)...);
    case 6:
      return Func::template run<6>(std::forward<Args>(args)...);
    case 7:
      return Func::template run<7>(std::forward<Args>(args)...);
    case 8:
      return Func::template run<8>(std::forward<Args>(args)...);
    case 9:
      return Func::template run<9>(std::forward<Args>(args)...);
    case 10:
      return Func::template run<10>(std::forward<Args>(args)...);
    case 11:
      return Func::template run<11>(std::forward<Args>(args)...);
    case 12:
      return Func::template run<12>(std::forward<Args>(args)...);
    case 13:
      return Func::template run<13>(std::forward<Args>(args)...);
    case 14:
      return Func::template run<14>(std::forward<Args>(args)...);
    case 15:
      return Func::template run<15>(std::forward<Args>(args)...);
    case 16:
      return Func::template run<16>(std::forward<Args>(args)...);
    case 17:
      return Func::template run<17>(std::forward<Args>(args)...);
    case 18:
      return Func::template run<18>(std::forward<Args>(args)...);
    case 19:
      return Func::template run<19>(std::forward<Args>(args)...);
    case 20:
      return Func::template run<20>(std::forward<Args>(args)...);
    case 21:
      return Func::template run<21>(std::forward<Args>(args)...);
    case 22:
      return Func::template run<22>(std::forward<Args>(args)...);
    case 23:
      return Func::template run<23>(std::forward<Args>(args)...);
    case 24:
      return Func::template run<24>(std::forward<Args>(args)...);
    case 25:
      return Func::template run<25>(std::forward<Args>(args)...);
    case 26:
      return Func::template run<26>(std::forward<Args>(args)...);
    case 27:
      return Func::template run<27>(std::forward<Args>(args)...);
    case 28:
      return Func::template run<28>(std::forward<Args>(args)...);
    case 29:
      return Func::template run<29>(std::forward<Args>(args)...);
    case 30:
      return Func::template run<30>(std::forward<Args>(args)...);
    case 31:
      return Func::template run<31>(std::forward<Args>(args)...);
    case 32:
      return Func::template run<32>(std::forward<Args>(args)...);
    case 33:
      return Func::template run<33>(std::forward<Args>(args)...);
    case 34:
      return Func::template run<34>(std::forward<Args>(args)...);
    case 35:
      return Func::template run<35>(std::forward<Args>(args)...);
    case 36:
      return Func::template run<36>(std::forward<Args>(args)...);
    case 37:
      return Func::template run<37>(std::forward<Args>(args)...);
    case 38:
      return Func::template run<38>(std::forward<Args>(args)...);
    case 39:
      return Func::template run<39>(std::forward<Args>(args)...);
    case 40:
      return Func::template run<40>(std::forward<Args>(args)...);
    case 41:
      return Func::template run<41>(std::forward<Args>(args)...);
    case 42:
      return Func::template run<42>(std::forward<Args>(args)...);
    case 43:
      return Func::template run<43>(std::forward<Args>(args)...);
    case 44:
      return Func::template run<44>(std::forward<Args>(args)...);
    case 45:
      return Func::template run<45>(std::forward<Args>(args)...);
    case 46:
      return Func::template run<46>(std::forward<Args>(args)...);
    case 47:
      return Func::template run<47>(std::forward<Args>(args)...);
    case 48:
      return Func::template run<48>(std::forward<Args>(args)...);
    case 49:
      return Func::template run<49>(std::forward<Args>(args)...);
    case 50:
      return Func::template run<50>(std::forward<Args>(args)...);
    case 51:
      return Func::template run<51>(std::forward<Args>(args)...);
    case 52:
      return Func::template run<52>(std::forward<Args>(args)...);
    case 53:
      return Func::template run<53>(std::forward<Args>(args)...);
    case 54:
      return Func::template run<54>(std::forward<Args>(args)...);
    case 55:
      return Func::template run<55>(std::forward<Args>(args)...);
    case 56:
      return Func::template run<56>(std::forward<Args>(args)...);
    case 57:
      return Func::template run<57>(std::forward<Args>(args)...);
    case 58:
      return Func::template run<58>(std::forward<Args>(args)...);
    case 59:
      return Func::template run<59>(std::forward<Args>(args)...);
    case 60:
      return Func::template run<60>(std::forward<Args>(args)...);
    case 61:
      return Func::template run<61>(std::forward<Args>(args)...);
    case 62:
      return Func::template run<62>(std::forward<Args>(args)...);
    case 63:
      return Func::template run<63>(std::forward<Args>(args)...);
    case 64:
      return Func::template run<64>(std::forward<Args>(args)...);
    case 65:
      return Func::template run<65>(std::forward<Args>(args)...);
    case 66:
      return Func::template run<66>(std::forward<Args>(args)...);
    case 67:
      return Func::template run<67>(std::forward<Args>(args)...);
    case 68:
      return Func::template run<68>(std::forward<Args>(args)...);
    case 69:
      return Func::template run<69>(std::forward<Args>(args)...);
    case 70:
      return Func::template run<70>(std::forward<Args>(args)...);
    case 71:
      return Func::template run<71>(std::forward<Args>(args)...);
    case 72:
      return Func::template run<72>(std::forward<Args>(args)...);
    case 73:
      return Func::template run<73>(std::forward<Args>(args)...);
    case 74:
      return Func::template run<74>(std::forward<Args>(args)...);
    case 75:
      return Func::template run<75>(std::forward<Args>(args)...);
    case 76:
      return Func::template run<76>(std::forward<Args>(args)...);
    case 77:
      return Func::template run<77>(std::forward<Args>(args)...);
    case 78:
      return Func::template run<78>(std::forward<Args>(args)...);
    case 79:
      return Func::template run<79>(std::forward<Args>(args)...);
    case 80:
      return Func::template run<80>(std::forward<Args>(args)...);
    case 81:
      return Func::template run<81>(std::forward<Args>(args)...);
    case 82:
      return Func::template run<82>(std::forward<Args>(args)...);
    case 83:
      return Func::template run<83>(std::forward<Args>(args)...);
    case 84:
      return Func::template run<84>(std::forward<Args>(args)...);
    case 85:
      return Func::template run<85>(std::forward<Args>(args)...);
    case 86:
      return Func::template run<86>(std::forward<Args>(args)...);
    case 87:
      return Func::template run<87>(std::forward<Args>(args)...);
    case 88:
      return Func::template run<88>(std::forward<Args>(args)...);
    case 89:
      return Func::template run<89>(std::forward<Args>(args)...);
    case 90:
      return Func::template run<90>(std::forward<Args>(args)...);
    case 91:
      return Func::template run<91>(std::forward<Args>(args)...);
    case 92:
      return Func::template run<92>(std::forward<Args>(args)...);
    case 93:
      return Func::template run<93>(std::forward<Args>(args)...);
    case 94:
      return Func::template run<94>(std::forward<Args>(args)...);
    case 95:
      return Func::template run<95>(std::forward<Args>(args)...);
    case 96:
      return Func::template run<96>(std::forward<Args>(args)...);
    case 97:
      return Func::template run<97>(std::forward<Args>(args)...);
    case 98:
      return Func::template run<98>(std::forward<Args>(args)...);
    case 99:
      return Func::template run<99>(std::forward<Args>(args)...);
    case 100:
      return Func::template run<100>(std::forward<Args>(args)...);
    case 101:
      return Func::template run<101>(std::forward<Args>(args)...);
    case 102:
      return Func::template run<102>(std::forward<Args>(args)...);
    case 103:
      return Func::template run<103>(std::forward<Args>(args)...);
    case 104:
      return Func::template run<104>(std::forward<Args>(args)...);
    case 105:
      return Func::template run<105>(std::forward<Args>(args)...);
    case 106:
      return Func::template run<106>(std::forward<Args>(args)...);
    case 107:
      return Func::template run<107>(std::forward<Args>(args)...);
    case 108:
      return Func::template run<108>(std::forward<Args>(args)...);
    case 109:
      return Func::template run<109>(std::forward<Args>(args)...);
    case 110:
      return Func::template run<110>(std::forward<Args>(args)...);
    case 111:
      return Func::template run<111>(std::forward<Args>(args)...);
    case 112:
      return Func::template run<112>(std::forward<Args>(args)...);
    case 113:
      return Func::template run<113>(std::forward<Args>(args)...);
    case 114:
      return Func::template run<114>(std::forward<Args>(args)...);
    case 115:
      return Func::template run<115>(std::forward<Args>(args)...);
    case 116:
      return Func::template run<116>(std::forward<Args>(args)...);
    case 117:
      return Func::template run<117>(std::forward<Args>(args)...);
    case 118:
      return Func::template run<118>(std::forward<Args>(args)...);
    case 119:
      return Func::template run<119>(std::forward<Args>(args)...);
    case 120:
      return Func::template run<120>(std::forward<Args>(args)...);
    case 121:
      return Func::template run<121>(std::forward<Args>(args)...);
    case 122:
      return Func::template run<122>(std::forward<Args>(args)...);
    case 123:
      return Func::template run<123>(std::forward<Args>(args)...);
    case 124:
      return Func::template run<124>(std::forward<Args>(args)...);
    case 125:
      return Func::template run<125>(std::forward<Args>(args)...);
    case 126:
      return Func::template run<126>(std::forward<Args>(args)...);
    case 127:
      return Func::template run<127>(std::forward<Args>(args)...);
    case 128:
      return Func::template run<128>(std::forward<Args>(args)...);
    case 129:
      return Func::template run<129>(std::forward<Args>(args)...);
    case 130:
      return Func::template run<130>(std::forward<Args>(args)...);
    case 131:
      return Func::template run<131>(std::forward<Args>(args)...);
    case 132:
      return Func::template run<132>(std::forward<Args>(args)...);
    case 133:
      return Func::template run<133>(std::forward<Args>(args)...);
    case 134:
      return Func::template run<134>(std::forward<Args>(args)...);
    case 135:
      return Func::template run<135>(std::forward<Args>(args)...);
    case 136:
      return Func::template run<136>(std::forward<Args>(args)...);
    case 137:
      return Func::template run<137>(std::forward<Args>(args)...);
    case 138:
      return Func::template run<138>(std::forward<Args>(args)...);
    case 139:
      return Func::template run<139>(std::forward<Args>(args)...);
    case 140:
      return Func::template run<140>(std::forward<Args>(args)...);
    case 141:
      return Func::template run<141>(std::forward<Args>(args)...);
    case 142:
      return Func::template run<142>(std::forward<Args>(args)...);
    case 143:
      return Func::template run<143>(std::forward<Args>(args)...);
    case 144:
      return Func::template run<144>(std::forward<Args>(args)...);
    case 145:
      return Func::template run<145>(std::forward<Args>(args)...);
    case 146:
      return Func::template run<146>(std::forward<Args>(args)...);
    case 147:
      return Func::template run<147>(std::forward<Args>(args)...);
    case 148:
      return Func::template run<148>(std::forward<Args>(args)...);
    case 149:
      return Func::template run<149>(std::forward<Args>(args)...);
    case 150:
      return Func::template run<150>(std::forward<Args>(args)...);
    case 151:
      return Func::template run<151>(std::forward<Args>(args)...);
    case 152:
      return Func::template run<152>(std::forward<Args>(args)...);
    case 153:
      return Func::template run<153>(std::forward<Args>(args)...);
    case 154:
      return Func::template run<154>(std::forward<Args>(args)...);
    case 155:
      return Func::template run<155>(std::forward<Args>(args)...);
    case 156:
      return Func::template run<156>(std::forward<Args>(args)...);
    case 157:
      return Func::template run<157>(std::forward<Args>(args)...);
    case 158:
      return Func::template run<158>(std::forward<Args>(args)...);
    case 159:
      return Func::template run<159>(std::forward<Args>(args)...);
    case 160:
      return Func::template run<160>(std::forward<Args>(args)...);
    case 161:
      return Func::template run<161>(std::forward<Args>(args)...);
    case 162:
      return Func::template run<162>(std::forward<Args>(args)...);
    case 163:
      return Func::template run<163>(std::forward<Args>(args)...);
    case 164:
      return Func::template run<164>(std::forward<Args>(args)...);
    case 165:
      return Func::template run<165>(std::forward<Args>(args)...);
    case 166:
      return Func::template run<166>(std::forward<Args>(args)...);
    case 167:
      return Func::template run<167>(std::forward<Args>(args)...);
    case 168:
      return Func::template run<168>(std::forward<Args>(args)...);
    case 169:
      return Func::template run<169>(std::forward<Args>(args)...);
    case 170:
      return Func::template run<170>(std::forward<Args>(args)...);
    case 171:
      return Func::template run<171>(std::forward<Args>(args)...);
    case 172:
      return Func::template run<172>(std::forward<Args>(args)...);
    case 173:
      return Func::template run<173>(std::forward<Args>(args)...);
    case 174:
      return Func::template run<174>(std::forward<Args>(args)...);
    case 175:
      return Func::template run<175>(std::forward<Args>(args)...);
    case 176:
      return Func::template run<176>(std::forward<Args>(args)...);
    case 177:
      return Func::template run<177>(std::forward<Args>(args)...);
    case 178:
      return Func::template run<178>(std::forward<Args>(args)...);
    case 179:
      return Func::template run<179>(std::forward<Args>(args)...);
    case 180:
      return Func::template run<180>(std::forward<Args>(args)...);
    case 181:
      return Func::template run<181>(std::forward<Args>(args)...);
    case 182:
      return Func::template run<182>(std::forward<Args>(args)...);
    case 183:
      return Func::template run<183>(std::forward<Args>(args)...);
    case 184:
      return Func::template run<184>(std::forward<Args>(args)...);
    case 185:
      return Func::template run<185>(std::forward<Args>(args)...);
    case 186:
      return Func::template run<186>(std::forward<Args>(args)...);
    case 187:
      return Func::template run<187>(std::forward<Args>(args)...);
    case 188:
      return Func::template run<188>(std::forward<Args>(args)...);
    case 189:
      return Func::template run<189>(std::forward<Args>(args)...);
    case 190:
      return Func::template run<190>(std::forward<Args>(args)...);
    case 191:
      return Func::template run<191>(std::forward<Args>(args)...);
    case 192:
      return Func::template run<192>(std::forward<Args>(args)...);
    case 193:
      return Func::template run<193>(std::forward<Args>(args)...);
    case 194:
      return Func::template run<194>(std::forward<Args>(args)...);
    case 195:
      return Func::template run<195>(std::forward<Args>(args)...);
    case 196:
      return Func::template run<196>(std::forward<Args>(args)...);
    case 197:
      return Func::template run<197>(std::forward<Args>(args)...);
    case 198:
      return Func::template run<198>(std::forward<Args>(args)...);
    case 199:
      return Func::template run<199>(std::forward<Args>(args)...);
    case 200:
      return Func::template run<200>(std::forward<Args>(args)...);
    case 201:
      return Func::template run<201>(std::forward<Args>(args)...);
    case 202:
      return Func::template run<202>(std::forward<Args>(args)...);
    case 203:
      return Func::template run<203>(std::forward<Args>(args)...);
    case 204:
      return Func::template run<204>(std::forward<Args>(args)...);
    case 205:
      return Func::template run<205>(std::forward<Args>(args)...);
    case 206:
      return Func::template run<206>(std::forward<Args>(args)...);
    case 207:
      return Func::template run<207>(std::forward<Args>(args)...);
    case 208:
      return Func::template run<208>(std::forward<Args>(args)...);
    case 209:
      return Func::template run<209>(std::forward<Args>(args)...);
    case 210:
      return Func::template run<210>(std::forward<Args>(args)...);
    case 211:
      return Func::template run<211>(std::forward<Args>(args)...);
    case 212:
      return Func::template run<212>(std::forward<Args>(args)...);
    case 213:
      return Func::template run<213>(std::forward<Args>(args)...);
    case 214:
      return Func::template run<214>(std::forward<Args>(args)...);
    case 215:
      return Func::template run<215>(std::forward<Args>(args)...);
    case 216:
      return Func::template run<216>(std::forward<Args>(args)...);
    case 217:
      return Func::template run<217>(std::forward<Args>(args)...);
    case 218:
      return Func::template run<218>(std::forward<Args>(args)...);
    case 219:
      return Func::template run<219>(std::forward<Args>(args)...);
    case 220:
      return Func::template run<220>(std::forward<Args>(args)...);
    case 221:
      return Func::template run<221>(std::forward<Args>(args)...);
    case 222:
      return Func::template run<222>(std::forward<Args>(args)...);
    case 223:
      return Func::template run<223>(std::forward<Args>(args)...);
    case 224:
      return Func::template run<224>(std::forward<Args>(args)...);
    case 225:
      return Func::template run<225>(std::forward<Args>(args)...);
    case 226:
      return Func::template run<226>(std::forward<Args>(args)...);
    case 227:
      return Func::template run<227>(std::forward<Args>(args)...);
    case 228:
      return Func::template run<228>(std::forward<Args>(args)...);
    case 229:
      return Func::template run<229>(std::forward<Args>(args)...);
    case 230:
      return Func::template run<230>(std::forward<Args>(args)...);
    case 231:
      return Func::template run<231>(std::forward<Args>(args)...);
    case 232:
      return Func::template run<232>(std::forward<Args>(args)...);
    case 233:
      return Func::template run<233>(std::forward<Args>(args)...);
    case 234:
      return Func::template run<234>(std::forward<Args>(args)...);
    case 235:
      return Func::template run<235>(std::forward<Args>(args)...);
    case 236:
      return Func::template run<236>(std::forward<Args>(args)...);
    case 237:
      return Func::template run<237>(std::forward<Args>(args)...);
    case 238:
      return Func::template run<238>(std::forward<Args>(args)...);
    case 239:
      return Func::template run<239>(std::forward<Args>(args)...);
    case 240:
      return Func::template run<240>(std::forward<Args>(args)...);
    case 241:
      return Func::template run<241>(std::forward<Args>(args)...);
    case 242:
      return Func::template run<242>(std::forward<Args>(args)...);
    case 243:
      return Func::template run<243>(std::forward<Args>(args)...);
    case 244:
      return Func::template run<244>(std::forward<Args>(args)...);
    case 245:
      return Func::template run<245>(std::forward<Args>(args)...);
    case 246:
      return Func::template run<246>(std::forward<Args>(args)...);
    case 247:
      return Func::template run<247>(std::forward<Args>(args)...);
    case 248:
      return Func::template run<248>(std::forward<Args>(args)...);
    case 249:
      return Func::template run<249>(std::forward<Args>(args)...);
    case 250:
      return Func::template run<250>(std::forward<Args>(args)...);
    case 251:
      return Func::template run<251>(std::forward<Args>(args)...);
    case 252:
      return Func::template run<252>(std::forward<Args>(args)...);
    case 253:
      return Func::template run<253>(std::forward<Args>(args)...);
    case 254:
      return Func::template run<254>(std::forward<Args>(args)...);
    case 255:
      return Func::template run<255>(std::forward<Args>(args)...);
    default:
      unreachable();
      // index shouldn't bigger than 256
  }
}  // namespace detail
}  // namespace detail
}  // namespace struct_pack

// clang-format off


#define STRUCT_PACK_RETURN_ELEMENT(Idx, X) \
if constexpr (Idx == I) {\
    return c.X;\
}\

#define STRUCT_PACK_GET_INDEX(Idx, Type) \
inline auto& STRUCT_PACK_GET_##Idx(Type& c) {\
    return STRUCT_PACK_GET<STRUCT_PACK_FIELD_COUNT_IMPL<Type>()-1-Idx>(c);\
}\

#define STRUCT_PACK_GET_INDEX_CONST(Idx, Type) \
inline const auto& STRUCT_PACK_GET_##Idx(const Type& c) {\
    return STRUCT_PACK_GET<STRUCT_PACK_FIELD_COUNT_IMPL<Type>()-1-Idx>(c);\
}\

#define STRUCT_PACK_REFL(Type,...) \
inline Type& STRUCT_PACK_REFL_FLAG(Type& t) {return t;} \
template<typename T> \
constexpr std::size_t STRUCT_PACK_FIELD_COUNT_IMPL(); \
template<> \
constexpr std::size_t STRUCT_PACK_FIELD_COUNT_IMPL<Type>() {return STRUCT_PACK_ARG_COUNT(__VA_ARGS__);} \
inline decltype(auto) STRUCT_PACK_FIELD_COUNT(const Type &){ \
  return std::integral_constant<std::size_t,STRUCT_PACK_ARG_COUNT(__VA_ARGS__)>{}; \
} \
template<std::size_t I> auto& STRUCT_PACK_GET(Type& c) { \
  STRUCT_PACK_EXPAND_EACH(,STRUCT_PACK_RETURN_ELEMENT,__VA_ARGS__) \
  else { \
	static_assert(I < STRUCT_PACK_FIELD_COUNT_IMPL<Type>()); \
  } \
} \
template<std::size_t I> const auto& STRUCT_PACK_GET(const Type& c) { \
  STRUCT_PACK_EXPAND_EACH(,STRUCT_PACK_RETURN_ELEMENT,__VA_ARGS__) \
  else { \
	static_assert(I < STRUCT_PACK_FIELD_COUNT_IMPL<Type>()); \
  } \
} \
STRUCT_PACK_EXPAND_EACH(,STRUCT_PACK_GET_INDEX,STRUCT_PACK_MAKE_ARGS(Type,STRUCT_PACK_ARG_COUNT(__VA_ARGS__))) \
STRUCT_PACK_EXPAND_EACH(,STRUCT_PACK_GET_INDEX_CONST,STRUCT_PACK_MAKE_ARGS(Type,STRUCT_PACK_ARG_COUNT(__VA_ARGS__))) \

#define STRUCT_PACK_FRIEND_DECL(Type) \
template <std::size_t I> \
friend auto& STRUCT_PACK_GET(Type& c); \
template <std::size_t I> \
friend const auto& STRUCT_PACK_GET(const Type& c);
