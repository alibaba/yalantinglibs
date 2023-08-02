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

#include "marco.h"

namespace struct_pack {
namespace detail {

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

[[noreturn]] inline void unreachable() {
  // Uses compiler specific extensions if possible.
  // Even if no extension is used, undefined behavior is still raised by
  // an empty function body and the noreturn attribute.
#ifdef __GNUC__  // GCC, Clang, ICC
  __builtin_unreachable();
#elif defined(_MSC_VER)  // msvc
  __assume(false);
#endif
}

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

template <typename T>
concept seek_reader_t = reader_t<T> && requires(T t) {
  t.seekg(std::size_t{});
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

template <typename T, typename = void>
struct seek_reader_t_impl : std::false_type {};

template <typename T>
struct seek_reader_t_impl<
    T, std::void_t<decltype(std::declval<T>().seekg(std::size_t{}))>>
    : std::true_type {};

template <typename T>
constexpr bool seek_reader_t = reader_t<T> &&seek_reader_t_impl<T>::value;

#endif

template <typename T, uint64_t version = 0>
struct compatible;

// clang-format off
namespace detail {

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

  template <typename T>
  constexpr bool varint_t = varintable_t<T> || sintable_t<T>;

  template <typename Type>
  constexpr inline bool is_trivial_view_v = false;

  template <typename T, bool ignore_compatible_field = false>
  struct is_trivial_serializable {
    private:
      template<typename U, std::size_t... I>
      static constexpr bool class_visit_helper(std::index_sequence<I...>) {
        return (is_trivial_serializable<std::tuple_element_t<I, U>,
                                            ignore_compatible_field>::value &&
                    ...);
      }
      static constexpr bool solve() {
        if constexpr (std::is_same_v<T,std::monostate>) {
          return true;
        }
        else if constexpr (is_compatible_v<T> || is_trivial_view_v<T>) {
          return ignore_compatible_field;
        }
        else if constexpr (std::is_enum_v<T> || std::is_fundamental_v<T> 
#if __GNUC__ || __clang__
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
                          unique_ptr<T> || expected<T> || container_adapter<T> ||
                          varint_t<T>) {
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
          using U = decltype(get_types<T>());
          return class_visit_helper<U>(std::make_index_sequence<std::tuple_size_v<U>>{});
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
                                                            Args &...args) {
  switch (index) {
    case 0:
      return Func::template run<0>(args...);
    case 1:
      return Func::template run<1>(args...);
    case 2:
      return Func::template run<2>(args...);
    case 3:
      return Func::template run<3>(args...);
    case 4:
      return Func::template run<4>(args...);
    case 5:
      return Func::template run<5>(args...);
    case 6:
      return Func::template run<6>(args...);
    case 7:
      return Func::template run<7>(args...);
    case 8:
      return Func::template run<8>(args...);
    case 9:
      return Func::template run<9>(args...);
    case 10:
      return Func::template run<10>(args...);
    case 11:
      return Func::template run<11>(args...);
    case 12:
      return Func::template run<12>(args...);
    case 13:
      return Func::template run<13>(args...);
    case 14:
      return Func::template run<14>(args...);
    case 15:
      return Func::template run<15>(args...);
    case 16:
      return Func::template run<16>(args...);
    case 17:
      return Func::template run<17>(args...);
    case 18:
      return Func::template run<18>(args...);
    case 19:
      return Func::template run<19>(args...);
    case 20:
      return Func::template run<20>(args...);
    case 21:
      return Func::template run<21>(args...);
    case 22:
      return Func::template run<22>(args...);
    case 23:
      return Func::template run<23>(args...);
    case 24:
      return Func::template run<24>(args...);
    case 25:
      return Func::template run<25>(args...);
    case 26:
      return Func::template run<26>(args...);
    case 27:
      return Func::template run<27>(args...);
    case 28:
      return Func::template run<28>(args...);
    case 29:
      return Func::template run<29>(args...);
    case 30:
      return Func::template run<30>(args...);
    case 31:
      return Func::template run<31>(args...);
    case 32:
      return Func::template run<32>(args...);
    case 33:
      return Func::template run<33>(args...);
    case 34:
      return Func::template run<34>(args...);
    case 35:
      return Func::template run<35>(args...);
    case 36:
      return Func::template run<36>(args...);
    case 37:
      return Func::template run<37>(args...);
    case 38:
      return Func::template run<38>(args...);
    case 39:
      return Func::template run<39>(args...);
    case 40:
      return Func::template run<40>(args...);
    case 41:
      return Func::template run<41>(args...);
    case 42:
      return Func::template run<42>(args...);
    case 43:
      return Func::template run<43>(args...);
    case 44:
      return Func::template run<44>(args...);
    case 45:
      return Func::template run<45>(args...);
    case 46:
      return Func::template run<46>(args...);
    case 47:
      return Func::template run<47>(args...);
    case 48:
      return Func::template run<48>(args...);
    case 49:
      return Func::template run<49>(args...);
    case 50:
      return Func::template run<50>(args...);
    case 51:
      return Func::template run<51>(args...);
    case 52:
      return Func::template run<52>(args...);
    case 53:
      return Func::template run<53>(args...);
    case 54:
      return Func::template run<54>(args...);
    case 55:
      return Func::template run<55>(args...);
    case 56:
      return Func::template run<56>(args...);
    case 57:
      return Func::template run<57>(args...);
    case 58:
      return Func::template run<58>(args...);
    case 59:
      return Func::template run<59>(args...);
    case 60:
      return Func::template run<60>(args...);
    case 61:
      return Func::template run<61>(args...);
    case 62:
      return Func::template run<62>(args...);
    case 63:
      return Func::template run<63>(args...);
    case 64:
      return Func::template run<64>(args...);
    case 65:
      return Func::template run<65>(args...);
    case 66:
      return Func::template run<66>(args...);
    case 67:
      return Func::template run<67>(args...);
    case 68:
      return Func::template run<68>(args...);
    case 69:
      return Func::template run<69>(args...);
    case 70:
      return Func::template run<70>(args...);
    case 71:
      return Func::template run<71>(args...);
    case 72:
      return Func::template run<72>(args...);
    case 73:
      return Func::template run<73>(args...);
    case 74:
      return Func::template run<74>(args...);
    case 75:
      return Func::template run<75>(args...);
    case 76:
      return Func::template run<76>(args...);
    case 77:
      return Func::template run<77>(args...);
    case 78:
      return Func::template run<78>(args...);
    case 79:
      return Func::template run<79>(args...);
    case 80:
      return Func::template run<80>(args...);
    case 81:
      return Func::template run<81>(args...);
    case 82:
      return Func::template run<82>(args...);
    case 83:
      return Func::template run<83>(args...);
    case 84:
      return Func::template run<84>(args...);
    case 85:
      return Func::template run<85>(args...);
    case 86:
      return Func::template run<86>(args...);
    case 87:
      return Func::template run<87>(args...);
    case 88:
      return Func::template run<88>(args...);
    case 89:
      return Func::template run<89>(args...);
    case 90:
      return Func::template run<90>(args...);
    case 91:
      return Func::template run<91>(args...);
    case 92:
      return Func::template run<92>(args...);
    case 93:
      return Func::template run<93>(args...);
    case 94:
      return Func::template run<94>(args...);
    case 95:
      return Func::template run<95>(args...);
    case 96:
      return Func::template run<96>(args...);
    case 97:
      return Func::template run<97>(args...);
    case 98:
      return Func::template run<98>(args...);
    case 99:
      return Func::template run<99>(args...);
    case 100:
      return Func::template run<100>(args...);
    case 101:
      return Func::template run<101>(args...);
    case 102:
      return Func::template run<102>(args...);
    case 103:
      return Func::template run<103>(args...);
    case 104:
      return Func::template run<104>(args...);
    case 105:
      return Func::template run<105>(args...);
    case 106:
      return Func::template run<106>(args...);
    case 107:
      return Func::template run<107>(args...);
    case 108:
      return Func::template run<108>(args...);
    case 109:
      return Func::template run<109>(args...);
    case 110:
      return Func::template run<110>(args...);
    case 111:
      return Func::template run<111>(args...);
    case 112:
      return Func::template run<112>(args...);
    case 113:
      return Func::template run<113>(args...);
    case 114:
      return Func::template run<114>(args...);
    case 115:
      return Func::template run<115>(args...);
    case 116:
      return Func::template run<116>(args...);
    case 117:
      return Func::template run<117>(args...);
    case 118:
      return Func::template run<118>(args...);
    case 119:
      return Func::template run<119>(args...);
    case 120:
      return Func::template run<120>(args...);
    case 121:
      return Func::template run<121>(args...);
    case 122:
      return Func::template run<122>(args...);
    case 123:
      return Func::template run<123>(args...);
    case 124:
      return Func::template run<124>(args...);
    case 125:
      return Func::template run<125>(args...);
    case 126:
      return Func::template run<126>(args...);
    case 127:
      return Func::template run<127>(args...);
    case 128:
      return Func::template run<128>(args...);
    case 129:
      return Func::template run<129>(args...);
    case 130:
      return Func::template run<130>(args...);
    case 131:
      return Func::template run<131>(args...);
    case 132:
      return Func::template run<132>(args...);
    case 133:
      return Func::template run<133>(args...);
    case 134:
      return Func::template run<134>(args...);
    case 135:
      return Func::template run<135>(args...);
    case 136:
      return Func::template run<136>(args...);
    case 137:
      return Func::template run<137>(args...);
    case 138:
      return Func::template run<138>(args...);
    case 139:
      return Func::template run<139>(args...);
    case 140:
      return Func::template run<140>(args...);
    case 141:
      return Func::template run<141>(args...);
    case 142:
      return Func::template run<142>(args...);
    case 143:
      return Func::template run<143>(args...);
    case 144:
      return Func::template run<144>(args...);
    case 145:
      return Func::template run<145>(args...);
    case 146:
      return Func::template run<146>(args...);
    case 147:
      return Func::template run<147>(args...);
    case 148:
      return Func::template run<148>(args...);
    case 149:
      return Func::template run<149>(args...);
    case 150:
      return Func::template run<150>(args...);
    case 151:
      return Func::template run<151>(args...);
    case 152:
      return Func::template run<152>(args...);
    case 153:
      return Func::template run<153>(args...);
    case 154:
      return Func::template run<154>(args...);
    case 155:
      return Func::template run<155>(args...);
    case 156:
      return Func::template run<156>(args...);
    case 157:
      return Func::template run<157>(args...);
    case 158:
      return Func::template run<158>(args...);
    case 159:
      return Func::template run<159>(args...);
    case 160:
      return Func::template run<160>(args...);
    case 161:
      return Func::template run<161>(args...);
    case 162:
      return Func::template run<162>(args...);
    case 163:
      return Func::template run<163>(args...);
    case 164:
      return Func::template run<164>(args...);
    case 165:
      return Func::template run<165>(args...);
    case 166:
      return Func::template run<166>(args...);
    case 167:
      return Func::template run<167>(args...);
    case 168:
      return Func::template run<168>(args...);
    case 169:
      return Func::template run<169>(args...);
    case 170:
      return Func::template run<170>(args...);
    case 171:
      return Func::template run<171>(args...);
    case 172:
      return Func::template run<172>(args...);
    case 173:
      return Func::template run<173>(args...);
    case 174:
      return Func::template run<174>(args...);
    case 175:
      return Func::template run<175>(args...);
    case 176:
      return Func::template run<176>(args...);
    case 177:
      return Func::template run<177>(args...);
    case 178:
      return Func::template run<178>(args...);
    case 179:
      return Func::template run<179>(args...);
    case 180:
      return Func::template run<180>(args...);
    case 181:
      return Func::template run<181>(args...);
    case 182:
      return Func::template run<182>(args...);
    case 183:
      return Func::template run<183>(args...);
    case 184:
      return Func::template run<184>(args...);
    case 185:
      return Func::template run<185>(args...);
    case 186:
      return Func::template run<186>(args...);
    case 187:
      return Func::template run<187>(args...);
    case 188:
      return Func::template run<188>(args...);
    case 189:
      return Func::template run<189>(args...);
    case 190:
      return Func::template run<190>(args...);
    case 191:
      return Func::template run<191>(args...);
    case 192:
      return Func::template run<192>(args...);
    case 193:
      return Func::template run<193>(args...);
    case 194:
      return Func::template run<194>(args...);
    case 195:
      return Func::template run<195>(args...);
    case 196:
      return Func::template run<196>(args...);
    case 197:
      return Func::template run<197>(args...);
    case 198:
      return Func::template run<198>(args...);
    case 199:
      return Func::template run<199>(args...);
    case 200:
      return Func::template run<200>(args...);
    case 201:
      return Func::template run<201>(args...);
    case 202:
      return Func::template run<202>(args...);
    case 203:
      return Func::template run<203>(args...);
    case 204:
      return Func::template run<204>(args...);
    case 205:
      return Func::template run<205>(args...);
    case 206:
      return Func::template run<206>(args...);
    case 207:
      return Func::template run<207>(args...);
    case 208:
      return Func::template run<208>(args...);
    case 209:
      return Func::template run<209>(args...);
    case 210:
      return Func::template run<210>(args...);
    case 211:
      return Func::template run<211>(args...);
    case 212:
      return Func::template run<212>(args...);
    case 213:
      return Func::template run<213>(args...);
    case 214:
      return Func::template run<214>(args...);
    case 215:
      return Func::template run<215>(args...);
    case 216:
      return Func::template run<216>(args...);
    case 217:
      return Func::template run<217>(args...);
    case 218:
      return Func::template run<218>(args...);
    case 219:
      return Func::template run<219>(args...);
    case 220:
      return Func::template run<220>(args...);
    case 221:
      return Func::template run<221>(args...);
    case 222:
      return Func::template run<222>(args...);
    case 223:
      return Func::template run<223>(args...);
    case 224:
      return Func::template run<224>(args...);
    case 225:
      return Func::template run<225>(args...);
    case 226:
      return Func::template run<226>(args...);
    case 227:
      return Func::template run<227>(args...);
    case 228:
      return Func::template run<228>(args...);
    case 229:
      return Func::template run<229>(args...);
    case 230:
      return Func::template run<230>(args...);
    case 231:
      return Func::template run<231>(args...);
    case 232:
      return Func::template run<232>(args...);
    case 233:
      return Func::template run<233>(args...);
    case 234:
      return Func::template run<234>(args...);
    case 235:
      return Func::template run<235>(args...);
    case 236:
      return Func::template run<236>(args...);
    case 237:
      return Func::template run<237>(args...);
    case 238:
      return Func::template run<238>(args...);
    case 239:
      return Func::template run<239>(args...);
    case 240:
      return Func::template run<240>(args...);
    case 241:
      return Func::template run<241>(args...);
    case 242:
      return Func::template run<242>(args...);
    case 243:
      return Func::template run<243>(args...);
    case 244:
      return Func::template run<244>(args...);
    case 245:
      return Func::template run<245>(args...);
    case 246:
      return Func::template run<246>(args...);
    case 247:
      return Func::template run<247>(args...);
    case 248:
      return Func::template run<248>(args...);
    case 249:
      return Func::template run<249>(args...);
    case 250:
      return Func::template run<250>(args...);
    case 251:
      return Func::template run<251>(args...);
    case 252:
      return Func::template run<252>(args...);
    case 253:
      return Func::template run<253>(args...);
    case 254:
      return Func::template run<254>(args...);
    case 255:
      return Func::template run<255>(args...);
    default:
      unreachable();
      // index shouldn't bigger than 256
  }
}  // namespace detail
}  // namespace detail
}  // namespace struct_pack

// clang-format off
#define STRUCT_PACK_ARG_COUNT(...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_INTERNAL_ARG_COUNT(0, ##__VA_ARGS__,\
	64, 63, 62, 61, 60, \
	59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
	49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
	39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
	29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
	19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
	 9,  8,  7,  6,  5,  4,  3,  2,  1,  0))

#define STRUCT_PACK_INTERNAL_ARG_COUNT(\
	 _0,  _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, \
	_10, _11, _12, _13, _14, _15, _16, _17, _18, _19, \
	_20, _21, _22, _23, _24, _25, _26, _27, _28, _29, \
	_30, _31, _32, _33, _34, _35, _36, _37, _38, _39, \
	_40, _41, _42, _43, _44, _45, _46, _47, _48, _49, \
	_50, _51, _52, _53, _54, _55, _56, _57, _58, _59, \
	_60, _61, _62, _63, _64, N, ...) N

#define STRUCT_PACK_CONCAT_(l, r) l ## r
#define STRUCT_PACK_CONCAT(l, r) STRUCT_PACK_CONCAT_(l, r)

#define STRUCT_PACK_MARCO_EXPAND(...) __VA_ARGS__

#define STRUCT_PACK_DOARG0(s,f,o) 
#define STRUCT_PACK_DOARG1(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG0(s,f,__VA_ARGS__))  s f(0,t)
#define STRUCT_PACK_DOARG2(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG1(s,f,__VA_ARGS__))  s f(1,t) 
#define STRUCT_PACK_DOARG3(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG2(s,f,__VA_ARGS__))  s f(2,t)
#define STRUCT_PACK_DOARG4(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG3(s,f,__VA_ARGS__))  s f(3,t)
#define STRUCT_PACK_DOARG5(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG4(s,f,__VA_ARGS__))  s f(4,t)
#define STRUCT_PACK_DOARG6(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG5(s,f,__VA_ARGS__))  s f(5,t)
#define STRUCT_PACK_DOARG7(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG6(s,f,__VA_ARGS__))  s f(6,t)
#define STRUCT_PACK_DOARG8(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG7(s,f,__VA_ARGS__)) s f(7,t)
#define STRUCT_PACK_DOARG9(s,f,t,...)  STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG8(s,f,__VA_ARGS__)) s f(8,t)
#define STRUCT_PACK_DOARG10(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG9(s,f,__VA_ARGS__)) s f(9,t)
#define STRUCT_PACK_DOARG11(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG10(s,f,__VA_ARGS__)) s f(10,t)
#define STRUCT_PACK_DOARG12(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG11(s,f,__VA_ARGS__)) s f(11,t)
#define STRUCT_PACK_DOARG13(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG12(s,f,__VA_ARGS__)) s f(12,t)
#define STRUCT_PACK_DOARG14(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG13(s,f,__VA_ARGS__)) s f(13,t)
#define STRUCT_PACK_DOARG15(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG14(s,f,__VA_ARGS__)) s f(14,t)
#define STRUCT_PACK_DOARG16(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG15(s,f,__VA_ARGS__)) s f(15,t)
#define STRUCT_PACK_DOARG17(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG16(s,f,__VA_ARGS__)) s f(16,t)
#define STRUCT_PACK_DOARG18(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG17(s,f,__VA_ARGS__)) s f(17,t)
#define STRUCT_PACK_DOARG19(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG18(s,f,__VA_ARGS__)) s f(18,t)
#define STRUCT_PACK_DOARG20(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG19(s,f,__VA_ARGS__)) s f(19,t)
#define STRUCT_PACK_DOARG21(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG20(s,f,__VA_ARGS__)) s f(20,t)
#define STRUCT_PACK_DOARG22(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG21(s,f,__VA_ARGS__)) s f(21,t)
#define STRUCT_PACK_DOARG23(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG22(s,f,__VA_ARGS__)) s f(22,t)
#define STRUCT_PACK_DOARG24(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG23(s,f,__VA_ARGS__)) s f(23,t)
#define STRUCT_PACK_DOARG25(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG24(s,f,__VA_ARGS__)) s f(24,t)
#define STRUCT_PACK_DOARG26(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG25(s,f,__VA_ARGS__)) s f(25,t)
#define STRUCT_PACK_DOARG27(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG26(s,f,__VA_ARGS__)) s f(26,t)
#define STRUCT_PACK_DOARG28(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG27(s,f,__VA_ARGS__)) s f(27,t)
#define STRUCT_PACK_DOARG29(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG28(s,f,__VA_ARGS__)) s f(28,t)
#define STRUCT_PACK_DOARG30(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG29(s,f,__VA_ARGS__)) s f(29,t)
#define STRUCT_PACK_DOARG31(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG30(s,f,__VA_ARGS__)) s f(30,t)
#define STRUCT_PACK_DOARG32(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG31(s,f,__VA_ARGS__)) s f(31,t)
#define STRUCT_PACK_DOARG33(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG32(s,f,__VA_ARGS__)) s f(32,t)
#define STRUCT_PACK_DOARG34(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG33(s,f,__VA_ARGS__)) s f(33,t)
#define STRUCT_PACK_DOARG35(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG34(s,f,__VA_ARGS__)) s f(34,t)
#define STRUCT_PACK_DOARG36(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG35(s,f,__VA_ARGS__)) s f(35,t)
#define STRUCT_PACK_DOARG37(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG36(s,f,__VA_ARGS__)) s f(36,t)
#define STRUCT_PACK_DOARG38(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG37(s,f,__VA_ARGS__)) s f(37,t)
#define STRUCT_PACK_DOARG39(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG38(s,f,__VA_ARGS__)) s f(38,t)
#define STRUCT_PACK_DOARG40(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG39(s,f,__VA_ARGS__)) s f(39,t)
#define STRUCT_PACK_DOARG41(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG40(s,f,__VA_ARGS__)) s f(40,t)
#define STRUCT_PACK_DOARG42(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG41(s,f,__VA_ARGS__)) s f(41,t)
#define STRUCT_PACK_DOARG43(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG42(s,f,__VA_ARGS__)) s f(42,t)
#define STRUCT_PACK_DOARG44(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG43(s,f,__VA_ARGS__)) s f(43,t)
#define STRUCT_PACK_DOARG45(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG44(s,f,__VA_ARGS__)) s f(44,t)
#define STRUCT_PACK_DOARG46(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG45(s,f,__VA_ARGS__)) s f(45,t)
#define STRUCT_PACK_DOARG47(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG46(s,f,__VA_ARGS__)) s f(46,t)
#define STRUCT_PACK_DOARG48(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG47(s,f,__VA_ARGS__)) s f(47,t)
#define STRUCT_PACK_DOARG49(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG48(s,f,__VA_ARGS__)) s f(48,t)
#define STRUCT_PACK_DOARG50(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG49(s,f,__VA_ARGS__)) s f(49,t)
#define STRUCT_PACK_DOARG51(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG50(s,f,__VA_ARGS__)) s f(50,t)
#define STRUCT_PACK_DOARG52(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG51(s,f,__VA_ARGS__)) s f(51,t)
#define STRUCT_PACK_DOARG53(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG52(s,f,__VA_ARGS__)) s f(52,t)
#define STRUCT_PACK_DOARG54(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG53(s,f,__VA_ARGS__)) s f(53,t)
#define STRUCT_PACK_DOARG55(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG54(s,f,__VA_ARGS__)) s f(54,t)
#define STRUCT_PACK_DOARG56(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG55(s,f,__VA_ARGS__)) s f(55,t)
#define STRUCT_PACK_DOARG57(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG56(s,f,__VA_ARGS__)) s f(56,t)
#define STRUCT_PACK_DOARG58(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG57(s,f,__VA_ARGS__)) s f(57,t)
#define STRUCT_PACK_DOARG59(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG58(s,f,__VA_ARGS__)) s f(58,t)
#define STRUCT_PACK_DOARG60(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG59(s,f,__VA_ARGS__)) s f(59,t)
#define STRUCT_PACK_DOARG61(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG60(s,f,__VA_ARGS__)) s f(60,t)
#define STRUCT_PACK_DOARG62(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG61(s,f,__VA_ARGS__)) s f(61,t)
#define STRUCT_PACK_DOARG63(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG62(s,f,__VA_ARGS__)) s f(62,t)
#define STRUCT_PACK_DOARG64(s,f,t,...) STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_DOARG63(s,f,__VA_ARGS__)) s f(63,t)

#define STRUCT_PACK_MAKE_ARGS0(Type)
#define STRUCT_PACK_MAKE_ARGS1(Type)     Type
#define STRUCT_PACK_MAKE_ARGS2(Type)     STRUCT_PACK_MAKE_ARGS1(Type), Type
#define STRUCT_PACK_MAKE_ARGS3(Type)     STRUCT_PACK_MAKE_ARGS2(Type), Type
#define STRUCT_PACK_MAKE_ARGS4(Type)     STRUCT_PACK_MAKE_ARGS3(Type), Type
#define STRUCT_PACK_MAKE_ARGS5(Type)     STRUCT_PACK_MAKE_ARGS4(Type), Type
#define STRUCT_PACK_MAKE_ARGS6(Type)     STRUCT_PACK_MAKE_ARGS5(Type), Type
#define STRUCT_PACK_MAKE_ARGS7(Type)     STRUCT_PACK_MAKE_ARGS6(Type), Type
#define STRUCT_PACK_MAKE_ARGS8(Type)     STRUCT_PACK_MAKE_ARGS7(Type), Type
#define STRUCT_PACK_MAKE_ARGS9(Type)     STRUCT_PACK_MAKE_ARGS8(Type), Type
#define STRUCT_PACK_MAKE_ARGS10(Type)    STRUCT_PACK_MAKE_ARGS9(Type), Type
#define STRUCT_PACK_MAKE_ARGS11(Type)    STRUCT_PACK_MAKE_ARGS10(Type), Type
#define STRUCT_PACK_MAKE_ARGS12(Type)    STRUCT_PACK_MAKE_ARGS11(Type), Type
#define STRUCT_PACK_MAKE_ARGS13(Type)    STRUCT_PACK_MAKE_ARGS12(Type), Type
#define STRUCT_PACK_MAKE_ARGS14(Type)    STRUCT_PACK_MAKE_ARGS13(Type), Type
#define STRUCT_PACK_MAKE_ARGS15(Type)    STRUCT_PACK_MAKE_ARGS14(Type), Type
#define STRUCT_PACK_MAKE_ARGS16(Type)    STRUCT_PACK_MAKE_ARGS15(Type), Type
#define STRUCT_PACK_MAKE_ARGS17(Type)    STRUCT_PACK_MAKE_ARGS16(Type), Type
#define STRUCT_PACK_MAKE_ARGS18(Type)    STRUCT_PACK_MAKE_ARGS17(Type), Type
#define STRUCT_PACK_MAKE_ARGS19(Type)    STRUCT_PACK_MAKE_ARGS18(Type), Type
#define STRUCT_PACK_MAKE_ARGS20(Type)    STRUCT_PACK_MAKE_ARGS19(Type), Type
#define STRUCT_PACK_MAKE_ARGS21(Type)    STRUCT_PACK_MAKE_ARGS20(Type), Type
#define STRUCT_PACK_MAKE_ARGS22(Type)    STRUCT_PACK_MAKE_ARGS21(Type), Type
#define STRUCT_PACK_MAKE_ARGS23(Type)    STRUCT_PACK_MAKE_ARGS22(Type), Type
#define STRUCT_PACK_MAKE_ARGS24(Type)    STRUCT_PACK_MAKE_ARGS23(Type), Type
#define STRUCT_PACK_MAKE_ARGS25(Type)    STRUCT_PACK_MAKE_ARGS24(Type), Type
#define STRUCT_PACK_MAKE_ARGS26(Type)    STRUCT_PACK_MAKE_ARGS25(Type), Type
#define STRUCT_PACK_MAKE_ARGS27(Type)    STRUCT_PACK_MAKE_ARGS26(Type), Type
#define STRUCT_PACK_MAKE_ARGS28(Type)    STRUCT_PACK_MAKE_ARGS27(Type), Type
#define STRUCT_PACK_MAKE_ARGS29(Type)    STRUCT_PACK_MAKE_ARGS28(Type), Type
#define STRUCT_PACK_MAKE_ARGS30(Type)    STRUCT_PACK_MAKE_ARGS29(Type), Type
#define STRUCT_PACK_MAKE_ARGS31(Type)    STRUCT_PACK_MAKE_ARGS30(Type), Type
#define STRUCT_PACK_MAKE_ARGS32(Type)    STRUCT_PACK_MAKE_ARGS31(Type), Type
#define STRUCT_PACK_MAKE_ARGS33(Type)    STRUCT_PACK_MAKE_ARGS32(Type), Type
#define STRUCT_PACK_MAKE_ARGS34(Type)    STRUCT_PACK_MAKE_ARGS33(Type), Type
#define STRUCT_PACK_MAKE_ARGS35(Type)    STRUCT_PACK_MAKE_ARGS34(Type), Type
#define STRUCT_PACK_MAKE_ARGS36(Type)    STRUCT_PACK_MAKE_ARGS35(Type), Type
#define STRUCT_PACK_MAKE_ARGS37(Type)    STRUCT_PACK_MAKE_ARGS36(Type), Type
#define STRUCT_PACK_MAKE_ARGS38(Type)    STRUCT_PACK_MAKE_ARGS37(Type), Type
#define STRUCT_PACK_MAKE_ARGS39(Type)    STRUCT_PACK_MAKE_ARGS38(Type), Type
#define STRUCT_PACK_MAKE_ARGS40(Type)    STRUCT_PACK_MAKE_ARGS39(Type), Type
#define STRUCT_PACK_MAKE_ARGS41(Type)    STRUCT_PACK_MAKE_ARGS40(Type), Type
#define STRUCT_PACK_MAKE_ARGS42(Type)    STRUCT_PACK_MAKE_ARGS41(Type), Type
#define STRUCT_PACK_MAKE_ARGS43(Type)    STRUCT_PACK_MAKE_ARGS42(Type), Type
#define STRUCT_PACK_MAKE_ARGS44(Type)    STRUCT_PACK_MAKE_ARGS43(Type), Type
#define STRUCT_PACK_MAKE_ARGS45(Type)    STRUCT_PACK_MAKE_ARGS44(Type), Type
#define STRUCT_PACK_MAKE_ARGS46(Type)    STRUCT_PACK_MAKE_ARGS45(Type), Type
#define STRUCT_PACK_MAKE_ARGS47(Type)    STRUCT_PACK_MAKE_ARGS46(Type), Type
#define STRUCT_PACK_MAKE_ARGS48(Type)    STRUCT_PACK_MAKE_ARGS47(Type), Type
#define STRUCT_PACK_MAKE_ARGS49(Type)    STRUCT_PACK_MAKE_ARGS48(Type), Type
#define STRUCT_PACK_MAKE_ARGS50(Type)    STRUCT_PACK_MAKE_ARGS49(Type), Type
#define STRUCT_PACK_MAKE_ARGS51(Type)    STRUCT_PACK_MAKE_ARGS50(Type), Type
#define STRUCT_PACK_MAKE_ARGS52(Type)    STRUCT_PACK_MAKE_ARGS51(Type), Type
#define STRUCT_PACK_MAKE_ARGS53(Type)    STRUCT_PACK_MAKE_ARGS52(Type), Type
#define STRUCT_PACK_MAKE_ARGS54(Type)    STRUCT_PACK_MAKE_ARGS53(Type), Type
#define STRUCT_PACK_MAKE_ARGS55(Type)    STRUCT_PACK_MAKE_ARGS54(Type), Type
#define STRUCT_PACK_MAKE_ARGS56(Type)    STRUCT_PACK_MAKE_ARGS55(Type), Type
#define STRUCT_PACK_MAKE_ARGS57(Type)    STRUCT_PACK_MAKE_ARGS56(Type), Type
#define STRUCT_PACK_MAKE_ARGS58(Type)    STRUCT_PACK_MAKE_ARGS57(Type), Type
#define STRUCT_PACK_MAKE_ARGS59(Type)    STRUCT_PACK_MAKE_ARGS58(Type), Type
#define STRUCT_PACK_MAKE_ARGS60(Type)    STRUCT_PACK_MAKE_ARGS59(Type), Type
#define STRUCT_PACK_MAKE_ARGS61(Type)    STRUCT_PACK_MAKE_ARGS60(Type), Type
#define STRUCT_PACK_MAKE_ARGS62(Type)    STRUCT_PACK_MAKE_ARGS61(Type), Type
#define STRUCT_PACK_MAKE_ARGS63(Type)    STRUCT_PACK_MAKE_ARGS62(Type), Type
#define STRUCT_PACK_MAKE_ARGS64(Type)    STRUCT_PACK_MAKE_ARGS63(Type), Type


#define STRUCT_PACK_MAKE_ARGS(Type,Count) \
  STRUCT_PACK_CONCAT(STRUCT_PACK_MAKE_ARGS,Count)(Type)

#define STRUCT_PACK_EXPAND_EACH_(sepatator,fun,...) \
		STRUCT_PACK_MARCO_EXPAND(STRUCT_PACK_CONCAT(STRUCT_PACK_DOARG,STRUCT_PACK_ARG_COUNT(__VA_ARGS__))(sepatator,fun,__VA_ARGS__))
#define STRUCT_PACK_EXPAND_EACH(sepatator,fun,...) \
		STRUCT_PACK_EXPAND_EACH_(sepatator,fun,__VA_ARGS__)

#define STRUCT_PACK_RETURN_ELEMENT(Idx, X) \
if constexpr (Idx == I) {\
    return c.X;\
}\

#define STRUCT_PACK_GET_INDEX(Idx, Type) \
auto& STRUCT_PACK_GET_##Idx(Type& c) {\
    return STRUCT_PACK_GET<STRUCT_PACK_FIELD_COUNT_IMPL<Type>()-1-Idx>(c);\
}\

#define STRUCT_PACK_GET_INDEX_CONST(Idx, Type) \
const auto& STRUCT_PACK_GET_##Idx(const Type& c) {\
    return STRUCT_PACK_GET<STRUCT_PACK_FIELD_COUNT_IMPL<Type>()-1-Idx>(c);\
}\

#define STRUCT_PACK_REFL(Type,...) \
Type& STRUCT_PACK_REFL_FLAG(Type& t) {return t;} \
template<typename T> \
constexpr std::size_t STRUCT_PACK_FIELD_COUNT_IMPL(); \
template<> \
constexpr std::size_t STRUCT_PACK_FIELD_COUNT_IMPL<Type>() {return STRUCT_PACK_ARG_COUNT(__VA_ARGS__);} \
decltype(auto) STRUCT_PACK_FIELD_COUNT(const Type &){ \
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
