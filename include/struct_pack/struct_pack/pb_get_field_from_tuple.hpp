
template <typename T, std::size_t FieldIndex>
[[nodiscard]] STRUCT_PACK_INLINE auto& get_field(T& t) {
  static_assert(!detail::optional<T>);
  constexpr auto Count = detail::member_count<T>();
  constexpr auto Index = FieldIndex;
  static_assert(Index >= 0 && Index <= Count);
  if constexpr (Count == 1) {
    auto& [a1] = t;
    return std::get<Index>(std::forward_as_tuple(a1));
  }
  else if constexpr (Count == 2) {
    auto& [a1, a2] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2));
  }
  else if constexpr (Count == 3) {
    auto& [a1, a2, a3] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3));
  }
  else if constexpr (Count == 4) {
    auto& [a1, a2, a3, a4] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4));
  }
  else if constexpr (Count == 5) {
    auto& [a1, a2, a3, a4, a5] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5));
  }
  else if constexpr (Count == 6) {
    auto& [a1, a2, a3, a4, a5, a6] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6));
  }
  else if constexpr (Count == 7) {
    auto& [a1, a2, a3, a4, a5, a6, a7] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7));
  }
  else if constexpr (Count == 8) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8] = t;
    return std::get<Index>(
        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8));
  }
  else if constexpr (Count == 9) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = t;
    return std::get<Index>(
        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9));
  }
  else if constexpr (Count == 10) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = t;
    return std::get<Index>(
        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10));
  }
  else if constexpr (Count == 11) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = t;
    return std::get<Index>(
        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11));
  }
  else if constexpr (Count == 12) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
                                                 a9, a10, a11, a12));
  }
  else if constexpr (Count == 13) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
                                                 a9, a10, a11, a12, a13));
  }
  else if constexpr (Count == 14) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
                                                 a9, a10, a11, a12, a13, a14));
  }
  else if constexpr (Count == 15) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] =
        t;
    return std::get<Index>(std::forward_as_tuple(
        a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15));
  }
  else {
    static_assert(!sizeof(T), "wait for add hard code");
  }
}

//template <typename T, std::size_t FieldIndex>
//[[nodiscard]] STRUCT_PACK_INLINE decltype(auto) get_field(T& t) {
//  // auto fn = ;
//  return detail::visit_members(t, []<typename ...Args>(Args&& ...args) {
//    return std::get<FieldIndex>(std::forward_as_tuple(std::forward<Args>(args)...));
//  });
//}
//template <typename T, std::size_t FieldIndex>
//[[nodiscard]] STRUCT_PACK_INLINE decltype(auto) get_field(const T& t) {
//  // auto fn = ;
//  return detail::visit_members(t, []<typename ...Args>(Args&& ...args) {
//    return std::get<FieldIndex>(std::forward_as_tuple(std::forward<Args>(args)...));
//  });
//}
template <typename T, std::size_t FieldIndex>
[[nodiscard]] STRUCT_PACK_INLINE const auto& get_field(const T& t) {
  static_assert(!detail::optional<T>);
  constexpr auto Count = detail::member_count<T>();
  constexpr auto Index = FieldIndex;
  static_assert(Index >= 0 && Index <= Count);
  if constexpr (Count == 1) {
    auto&& [a1] = t;
    return std::get<Index>(std::forward_as_tuple(a1));
  }
  else if constexpr (Count == 2) {
    auto&& [a1, a2] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2));
  }
  else if constexpr (Count == 3) {
    auto&& [a1, a2, a3] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3));
  }
  else if constexpr (Count == 4) {
    auto&& [a1, a2, a3, a4] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4));
  }
  else if constexpr (Count == 5) {
    auto&& [a1, a2, a3, a4, a5] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5));
  }
  else if constexpr (Count == 6) {
    auto&& [a1, a2, a3, a4, a5, a6] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6));
  }
  else if constexpr (Count == 7) {
    auto&& [a1, a2, a3, a4, a5, a6, a7] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7));
  }
  else if constexpr (Count == 8) {
    auto&& [a1, a2, a3, a4, a5, a6, a7, a8] = t;
    return std::get<Index>(
        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8));
  }
  else if constexpr (Count == 9) {
    auto&& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = t;
    return std::get<Index>(
        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9));
  }
  else if constexpr (Count == 10) {
    auto&& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = t;
    return std::get<Index>(
        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10));
  }
  else if constexpr (Count == 11) {
    auto&& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = t;
    return std::get<Index>(
        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11));
  }
  else if constexpr (Count == 12) {
    auto&& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
                                                 a9, a10, a11, a12));
  }
  else if constexpr (Count == 13) {
    auto&& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
                                                 a9, a10, a11, a12, a13));
  }
  else if constexpr (Count == 14) {
    auto&& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = t;
    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
                                                 a9, a10, a11, a12, a13, a14));
  }
  else if constexpr (Count == 15) {
    auto&& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] =
        t;
    return std::get<Index>(std::forward_as_tuple(
        a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15));
  }
  else {
    static_assert(!sizeof(T), "wait for add hard code");
  }
}
