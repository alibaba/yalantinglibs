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
// clang-format off
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
#include "ylt/reflection/member_count.hpp"

#if __has_include(<span>)
#include <span>
#endif

#include "derived_helper.hpp"
#include "foreach_macro.h"
#include "marco.h"
#include "util.h"
#include "ylt/reflection/template_switch.hpp"
#include "ylt/reflection/member_ptr.hpp"


#if __cpp_concepts >= 201907L
#include <concepts>
#endif

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
concept can_reserve = requires(T t) {
  t.reserve(std::size_t{});
};

template <typename T>
concept can_shrink_to_fit = requires(T t) {
  t.shrink_to_fit();
};

#else

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

template<typename T>
constexpr decltype(auto) delay_sp_config_eval() {
  if constexpr (sizeof(T)==0) {
    return (T*)nullptr;
  }
  else {
    return (sp_config*)nullptr;
  }
}

#if __cpp_concepts >= 201907L
  template<typename T>
  concept has_default_config = std::is_same_v<decltype(set_default(decltype(delay_sp_config_eval<T>()){})),struct_pack::sp_config>;
#else
  template <typename T, typename = void>
  struct has_default_config_impl : std::false_type {};

  template <typename T>
  struct has_default_config_impl<T, std::void_t<
    std::enable_if_t<std::is_same_v<decltype(set_default(decltype(delay_sp_config_eval<T>()){})),struct_pack::sp_config>>>>
      : std::true_type {};

  template <typename T>
  constexpr bool has_default_config = has_default_config_impl<T>::value;
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

struct memory_reader;

#if __cpp_concepts >= 201907L
  template <typename Type>
  concept user_defined_serialization = requires (Type& t) {
    sp_serialize_to(std::declval<struct_pack::detail::memory_writer&>(),(const Type&)t);
    {sp_deserialize_to(std::declval<struct_pack::detail::memory_reader&>(),t)} -> std::same_as<struct_pack::err_code>;
    {sp_get_needed_size((const Type&)t)}->std::same_as<std::size_t>;
  };
  template <typename Type>
  concept user_defined_type_name = requires {
    { sp_set_type_name((Type*)nullptr) } -> std::same_as<std::string_view>;
  };
#else

  template <typename T, typename = void>
  struct user_defined_serialization_impl : std::false_type {};

  template <typename T>
  struct user_defined_serialization_impl<T, std::void_t<
    decltype(sp_serialize_to(std::declval<struct_pack::detail::memory_writer&>(),std::declval<const T&>())),
    std::enable_if<std::is_same_v<decltype(sp_deserialize_to(std::declval<struct_pack::detail::memory_reader&>(),std::declval<T&>())), struct_pack::err_code>,
    std::enable_if<std::is_same_v<decltype(sp_get_needed_size(std::declval<const T&>())), std::string_view>>>>>
      : std::true_type {};

  template <typename Type>
  constexpr bool user_defined_serialization = user_defined_serialization_impl<Type>::value;

  template <typename T, typename = void>
  struct user_defined_type_name_impl : std::false_type {};

  template <typename T>
  struct user_defined_type_name_impl<T, std::void_t<
    std::enable_if<std::is_same_v<decltype(sp_set_type_name((T*)nullptr)), std::string_view>>>>
      : std::true_type {};

  template <typename Type>
  constexpr bool user_defined_type_name = user_defined_type_name_impl<Type>::value;
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
        if constexpr (user_defined_serialization<T>) {
          return false;
        }
        else if constexpr (std::is_same_v<T,std::monostate>) {
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
        else if constexpr (container<T> || ylt::reflection::optional<T> || is_variant_v<T> ||
                          unique_ptr<T> || ylt::reflection::expected<T> || container_adapter<T>) {
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
      return ylt::reflection::members_count_v<type>;
    }
  }

  constexpr static auto MaxVisitMembers = 256;

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
  template <typename Object, typename Visitor>
  constexpr decltype(auto) STRUCT_PACK_INLINE
  visit_members_by_user_defined_refl(Object &&o, Visitor &&visitor) {
    using type = remove_cvref_t<decltype(o)>;
    constexpr auto Count = decltype(STRUCT_PACK_FIELD_COUNT(o))::value;

    static_assert(Count <= MaxVisitMembers, "exceed max visit members");
    if constexpr (Count >= 0) {
      if constexpr (Count == 1) {
        return visitor(_SPG0(o));
      }
      else if constexpr (Count == 2) {
        return visitor(_SPG0(o), _SPG1(o));
      }
      else if constexpr (Count == 3) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o));
      }
      else if constexpr (Count == 4) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o));
      }
      else if constexpr (Count == 5) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o));
      }
      else if constexpr (Count == 6) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o));
      }
      else if constexpr (Count == 7) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o));
      }
      else if constexpr (Count == 8) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o));
      }
      else if constexpr (Count == 9) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o));
      }
      else if constexpr (Count == 10) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o));
      }
      else if constexpr (Count == 11) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o));
      }
      else if constexpr (Count == 12) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o));
      }
      else if constexpr (Count == 13) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o));
      }
      else if constexpr (Count == 14) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o));
      }
      else if constexpr (Count == 15) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o));
      }
      else if constexpr (Count == 16) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o));
      }
      else if constexpr (Count == 17) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o));
      }
      else if constexpr (Count == 18) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o));
      }
      else if constexpr (Count == 19) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o));
      }
      else if constexpr (Count == 20) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o));
      }
      else if constexpr (Count == 21) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o));
      }
      else if constexpr (Count == 22) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o));
      }
      else if constexpr (Count == 23) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o));
      }
      else if constexpr (Count == 24) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o));
      }
      else if constexpr (Count == 25) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o));
      }
      else if constexpr (Count == 26) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o));
      }
      else if constexpr (Count == 27) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o));
      }
      else if constexpr (Count == 28) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o));
      }
      else if constexpr (Count == 29) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o));
      }
      else if constexpr (Count == 30) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o));
      }
      else if constexpr (Count == 31) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o));
      }
      else if constexpr (Count == 32) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o));
      }
      else if constexpr (Count == 33) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o));
      }
      else if constexpr (Count == 34) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o));
      }
      else if constexpr (Count == 35) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o));
      }
      else if constexpr (Count == 36) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o));
      }
      else if constexpr (Count == 37) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o));
      }
      else if constexpr (Count == 38) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o));
      }
      else if constexpr (Count == 39) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o), _SPG38(o));
      }
      else if constexpr (Count == 40) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o));
      }
      else if constexpr (Count == 41) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o));
      }
      else if constexpr (Count == 42) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o));
      }
      else if constexpr (Count == 43) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o),
                       _SPG40(o), _SPG41(o), _SPG42(o));
      }
      else if constexpr (Count == 44) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o),
                       _SPG40(o), _SPG41(o), _SPG42(o), _SPG43(o));
      }
      else if constexpr (Count == 45) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o),
                       _SPG40(o), _SPG41(o), _SPG42(o), _SPG43(o), _SPG44(o));
      }
      else if constexpr (Count == 46) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o));
      }
      else if constexpr (Count == 47) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o));
      }
      else if constexpr (Count == 48) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o));
      }
      else if constexpr (Count == 49) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o),
                       _SPG40(o), _SPG41(o), _SPG42(o), _SPG43(o), _SPG44(o),
                       _SPG45(o), _SPG46(o), _SPG47(o), _SPG48(o));
      }
      else if constexpr (Count == 50) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o),
                       _SPG40(o), _SPG41(o), _SPG42(o), _SPG43(o), _SPG44(o),
                       _SPG45(o), _SPG46(o), _SPG47(o), _SPG48(o), _SPG49(o));
      }
      else if constexpr (Count == 51) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o));
      }
      else if constexpr (Count == 52) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o));
      }
      else if constexpr (Count == 53) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o));
      }
      else if constexpr (Count == 54) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o));
      }
      else if constexpr (Count == 55) {
        return visitor(_SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o),
                       _SPG5(o), _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o),
                       _SPG10(o), _SPG11(o), _SPG12(o), _SPG13(o), _SPG14(o),
                       _SPG15(o), _SPG16(o), _SPG17(o), _SPG18(o), _SPG19(o),
                       _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o), _SPG24(o),
                       _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
                       _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o),
                       _SPG35(o), _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o),
                       _SPG40(o), _SPG41(o), _SPG42(o), _SPG43(o), _SPG44(o),
                       _SPG45(o), _SPG46(o), _SPG47(o), _SPG48(o), _SPG49(o),
                       _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o), _SPG54(o));
      }
      else if constexpr (Count == 56) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o));
      }
      else if constexpr (Count == 57) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o));
      }
      else if constexpr (Count == 58) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o));
      }
      else if constexpr (Count == 59) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o));
      }
      else if constexpr (Count == 60) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o));
      }
      else if constexpr (Count == 61) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o));
      }
      else if constexpr (Count == 62) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o));
      }
      else if constexpr (Count == 63) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o));
      }
      else if constexpr (Count == 64) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o));
      }
      else if constexpr (Count == 65) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o));
      }
      else if constexpr (Count == 66) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o));
      }
      else if constexpr (Count == 67) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o));
      }
      else if constexpr (Count == 68) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o));
      }
      else if constexpr (Count == 69) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o));
      }
      else if constexpr (Count == 70) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o));
      }
      else if constexpr (Count == 71) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o));
      }
      else if constexpr (Count == 72) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o));
      }
      else if constexpr (Count == 73) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o));
      }
      else if constexpr (Count == 74) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o));
      }
      else if constexpr (Count == 75) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o));
      }
      else if constexpr (Count == 76) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o));
      }
      else if constexpr (Count == 77) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o));
      }
      else if constexpr (Count == 78) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o));
      }
      else if constexpr (Count == 79) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o));
      }
      else if constexpr (Count == 80) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o));
      }
      else if constexpr (Count == 81) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o));
      }
      else if constexpr (Count == 82) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o));
      }
      else if constexpr (Count == 83) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o));
      }
      else if constexpr (Count == 84) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o));
      }
      else if constexpr (Count == 85) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o));
      }
      else if constexpr (Count == 86) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o));
      }
      else if constexpr (Count == 87) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o));
      }
      else if constexpr (Count == 88) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o));
      }
      else if constexpr (Count == 89) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o));
      }
      else if constexpr (Count == 90) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o));
      }
      else if constexpr (Count == 91) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o));
      }
      else if constexpr (Count == 92) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o));
      }
      else if constexpr (Count == 93) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o));
      }
      else if constexpr (Count == 94) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o));
      }
      else if constexpr (Count == 95) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o));
      }
      else if constexpr (Count == 96) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o));
      }
      else if constexpr (Count == 97) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o));
      }
      else if constexpr (Count == 98) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o));
      }
      else if constexpr (Count == 99) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o));
      }
      else if constexpr (Count == 100) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o));
      }
      else if constexpr (Count == 101) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o));
      }
      else if constexpr (Count == 102) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o));
      }
      else if constexpr (Count == 103) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o));
      }
      else if constexpr (Count == 104) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o));
      }
      else if constexpr (Count == 105) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o));
      }
      else if constexpr (Count == 106) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o));
      }
      else if constexpr (Count == 107) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o));
      }
      else if constexpr (Count == 108) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o));
      }
      else if constexpr (Count == 109) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o));
      }
      else if constexpr (Count == 110) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o));
      }
      else if constexpr (Count == 111) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o));
      }
      else if constexpr (Count == 112) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o));
      }
      else if constexpr (Count == 113) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o));
      }
      else if constexpr (Count == 114) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o));
      }
      else if constexpr (Count == 115) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o));
      }
      else if constexpr (Count == 116) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o));
      }
      else if constexpr (Count == 117) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o), _SPG116(o));
      }
      else if constexpr (Count == 118) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o), _SPG116(o),
            _SPG117(o));
      }
      else if constexpr (Count == 119) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o), _SPG116(o),
            _SPG117(o), _SPG118(o));
      }
      else if constexpr (Count == 120) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o), _SPG116(o),
            _SPG117(o), _SPG118(o), _SPG119(o));
      }
      else if constexpr (Count == 121) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o), _SPG116(o),
            _SPG117(o), _SPG118(o), _SPG119(o), _SPG120(o));
      }
      else if constexpr (Count == 122) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o), _SPG116(o),
            _SPG117(o), _SPG118(o), _SPG119(o), _SPG120(o), _SPG121(o));
      }
      else if constexpr (Count == 123) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o), _SPG116(o),
            _SPG117(o), _SPG118(o), _SPG119(o), _SPG120(o), _SPG121(o),
            _SPG122(o));
      }
      else if constexpr (Count == 124) {
        return visitor(
            _SPG0(o), _SPG1(o), _SPG2(o), _SPG3(o), _SPG4(o), _SPG5(o),
            _SPG6(o), _SPG7(o), _SPG8(o), _SPG9(o), _SPG10(o), _SPG11(o),
            _SPG12(o), _SPG13(o), _SPG14(o), _SPG15(o), _SPG16(o), _SPG17(o),
            _SPG18(o), _SPG19(o), _SPG20(o), _SPG21(o), _SPG22(o), _SPG23(o),
            _SPG24(o), _SPG25(o), _SPG26(o), _SPG27(o), _SPG28(o), _SPG29(o),
            _SPG30(o), _SPG31(o), _SPG32(o), _SPG33(o), _SPG34(o), _SPG35(o),
            _SPG36(o), _SPG37(o), _SPG38(o), _SPG39(o), _SPG40(o), _SPG41(o),
            _SPG42(o), _SPG43(o), _SPG44(o), _SPG45(o), _SPG46(o), _SPG47(o),
            _SPG48(o), _SPG49(o), _SPG50(o), _SPG51(o), _SPG52(o), _SPG53(o),
            _SPG54(o), _SPG55(o), _SPG56(o), _SPG57(o), _SPG58(o), _SPG59(o),
            _SPG60(o), _SPG61(o), _SPG62(o), _SPG63(o), _SPG64(o), _SPG65(o),
            _SPG66(o), _SPG67(o), _SPG68(o), _SPG69(o), _SPG70(o), _SPG71(o),
            _SPG72(o), _SPG73(o), _SPG74(o), _SPG75(o), _SPG76(o), _SPG77(o),
            _SPG78(o), _SPG79(o), _SPG80(o), _SPG81(o), _SPG82(o), _SPG83(o),
            _SPG84(o), _SPG85(o), _SPG86(o), _SPG87(o), _SPG88(o), _SPG89(o),
            _SPG90(o), _SPG91(o), _SPG92(o), _SPG93(o), _SPG94(o), _SPG95(o),
            _SPG96(o), _SPG97(o), _SPG98(o), _SPG99(o), _SPG100(o), _SPG101(o),
            _SPG102(o), _SPG103(o), _SPG104(o), _SPG105(o), _SPG106(o),
            _SPG107(o), _SPG108(o), _SPG109(o), _SPG110(o), _SPG111(o),
            _SPG112(o), _SPG113(o), _SPG114(o), _SPG115(o), _SPG116(o),
            _SPG117(o), _SPG118(o), _SPG119(o), _SPG120(o), _SPG121(o),
            _SPG122(o), _SPG123(o));
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

    return ylt::reflection::visit_members<Object, Visitor, Count>(std::forward<Object>(object), std::forward<Visitor>(visitor));
  }
}  // namespace detail
#if __cpp_concepts >= 201907L

template <typename T>
concept checkable_reader_t = reader_t<T> && requires(T t) {
  t.check(std::size_t{});
};

#else

template <typename T, typename = void>
struct checkable_reader_t_impl : std::false_type {};

template <typename T>
struct checkable_reader_t_impl<
    T, std::void_t<decltype(std::declval<T>().check(std::size_t{}))>>
    : std::true_type {};

template <typename T>
constexpr bool checkable_reader_t = reader_t<T> &&checkable_reader_t_impl<T>::value;
#endif
}  // namespace struct_pack



#define STRUCT_PACK_RETURN_ELEMENT(Idx, X) \
if constexpr (Idx == I) {\
    return c.X;\
}

#define STRUCT_PACK_GET_INDEX(Idx, Type)                                       \
  inline auto &_SPG##Idx(Type &c) {                                            \
    return STRUCT_PACK_GET<STRUCT_PACK_FIELD_COUNT_IMPL<Type>() - 1 - Idx>(c); \
  }

#define STRUCT_PACK_GET_INDEX_CONST(Idx, Type)                                 \
  inline const auto &_SPG##Idx(const Type &c) {                                \
    return STRUCT_PACK_GET<STRUCT_PACK_FIELD_COUNT_IMPL<Type>() - 1 - Idx>(c); \
  }

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
