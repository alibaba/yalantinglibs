
template <typename T, std::size_t FieldIndex>
[[nodiscard]] STRUCT_PACK_INLINE auto& get_field(T& t) {
  static_assert(!detail::optional<T>);
  constexpr auto Count = detail::member_count<T>();
  constexpr auto Index = FieldIndex;
  static_assert(Index >= 0 && Index <= Count);
  if constexpr (Count == 1) {
    auto& [a1] = t;
    return a1;
  }
  else if constexpr (Count == 2) {
    auto& [a1, a2] = t;
    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else {
      return a2;
    }
  }
  else if constexpr (Count == 3) {
    auto& [a1, a2, a3] = t;
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3));
    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else {
      return a3;
    }
  }
  else if constexpr (Count == 4) {
    auto& [a1, a2, a3, a4] = t;
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4));

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else {
      return a4;
    }
  }
  else if constexpr (Count == 5) {
    auto& [a1, a2, a3, a4, a5] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else {
      return a5;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5));
  }
  else if constexpr (Count == 6) {
    auto& [a1, a2, a3, a4, a5, a6] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else {
      return a6;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6));
  }
  else if constexpr (Count == 7) {
    auto& [a1, a2, a3, a4, a5, a6, a7] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else {
      return a7;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7));
  }
  else if constexpr (Count == 8) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else {
      return a8;
    }
    //    return std::get<Index>(
    //        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8));
  }
  else if constexpr (Count == 9) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else {
      return a9;
    }
    //    return std::get<Index>(
    //        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9));
  }
  else if constexpr (Count == 10) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else {
      return a10;
    }
    //    return std::get<Index>(
    //        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10));
  }
  else if constexpr (Count == 11) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else {
      return a11;
    }
    //    return std::get<Index>(
    //        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11));
  }
  else if constexpr (Count == 12) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else if constexpr (FieldIndex == 10) {
      return a11;
    }
    else {
      return a12;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
    //                                                 a9, a10, a11, a12));
  }
  else if constexpr (Count == 13) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else if constexpr (FieldIndex == 10) {
      return a11;
    }
    else if constexpr (FieldIndex == 11) {
      return a12;
    }
    else {
      return a13;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
    //                                                 a9, a10, a11, a12, a13));
  }
  else if constexpr (Count == 14) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else if constexpr (FieldIndex == 10) {
      return a11;
    }
    else if constexpr (FieldIndex == 11) {
      return a12;
    }
    else if constexpr (FieldIndex == 12) {
      return a13;
    }
    else {
      return a14;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
    //                                                 a9, a10, a11, a12, a13, a14));
  }
  else if constexpr (Count == 15) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] =
        t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else if constexpr (FieldIndex == 10) {
      return a11;
    }
    else if constexpr (FieldIndex == 11) {
      return a12;
    }
    else if constexpr (FieldIndex == 12) {
      return a13;
    }
    else if constexpr (FieldIndex == 13) {
      return a14;
    }
    else {
      return a15;
    }
    //    return std::get<Index>(std::forward_as_tuple(
    //        a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15));
  }
  else {
    static_assert(!sizeof(T), "wait for add hard code");
  }
}
template <typename T, std::size_t FieldIndex>
[[nodiscard]] STRUCT_PACK_INLINE auto& get_field(const T& t) {
  static_assert(!detail::optional<T>);
  constexpr auto Count = detail::member_count<T>();
  constexpr auto Index = FieldIndex;
  static_assert(Index >= 0 && Index <= Count);
  if constexpr (Count == 1) {
    auto& [a1] = t;
    return a1;
  }
  else if constexpr (Count == 2) {
    auto& [a1, a2] = t;
    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else {
      return a2;
    }
  }
  else if constexpr (Count == 3) {
    auto& [a1, a2, a3] = t;
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3));
    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else {
      return a3;
    }
  }
  else if constexpr (Count == 4) {
    auto& [a1, a2, a3, a4] = t;
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4));

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else {
      return a4;
    }
  }
  else if constexpr (Count == 5) {
    auto& [a1, a2, a3, a4, a5] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else {
      return a5;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5));
  }
  else if constexpr (Count == 6) {
    auto& [a1, a2, a3, a4, a5, a6] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else {
      return a6;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6));
  }
  else if constexpr (Count == 7) {
    auto& [a1, a2, a3, a4, a5, a6, a7] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else {
      return a7;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7));
  }
  else if constexpr (Count == 8) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else {
      return a8;
    }
    //    return std::get<Index>(
    //        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8));
  }
  else if constexpr (Count == 9) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else {
      return a9;
    }
    //    return std::get<Index>(
    //        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9));
  }
  else if constexpr (Count == 10) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else {
      return a10;
    }
    //    return std::get<Index>(
    //        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10));
  }
  else if constexpr (Count == 11) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else {
      return a11;
    }
    //    return std::get<Index>(
    //        std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11));
  }
  else if constexpr (Count == 12) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else if constexpr (FieldIndex == 10) {
      return a11;
    }
    else {
      return a12;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
    //                                                 a9, a10, a11, a12));
  }
  else if constexpr (Count == 13) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else if constexpr (FieldIndex == 10) {
      return a11;
    }
    else if constexpr (FieldIndex == 11) {
      return a12;
    }
    else {
      return a13;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
    //                                                 a9, a10, a11, a12, a13));
  }
  else if constexpr (Count == 14) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else if constexpr (FieldIndex == 10) {
      return a11;
    }
    else if constexpr (FieldIndex == 11) {
      return a12;
    }
    else if constexpr (FieldIndex == 12) {
      return a13;
    }
    else {
      return a14;
    }
    //    return std::get<Index>(std::forward_as_tuple(a1, a2, a3, a4, a5, a6, a7, a8,
    //                                                 a9, a10, a11, a12, a13, a14));
  }
  else if constexpr (Count == 15) {
    auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] =
        t;

    if constexpr (FieldIndex == 0) {
      return a1;
    }
    else if constexpr (FieldIndex == 1) {
      return a2;
    }
    else if constexpr (FieldIndex == 2) {
      return a3;
    }
    else if constexpr (FieldIndex == 3) {
      return a4;
    }
    else if constexpr (FieldIndex == 4) {
      return a5;
    }
    else if constexpr (FieldIndex == 5) {
      return a6;
    }
    else if constexpr (FieldIndex == 6) {
      return a7;
    }
    else if constexpr (FieldIndex == 7) {
      return a8;
    }
    else if constexpr (FieldIndex == 8) {
      return a9;
    }
    else if constexpr (FieldIndex == 9) {
      return a10;
    }
    else if constexpr (FieldIndex == 10) {
      return a11;
    }
    else if constexpr (FieldIndex == 11) {
      return a12;
    }
    else if constexpr (FieldIndex == 12) {
      return a13;
    }
    else if constexpr (FieldIndex == 13) {
      return a14;
    }
    else {
      return a15;
    }
    //    return std::get<Index>(std::forward_as_tuple(
    //        a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15));
  }
  else {
    static_assert(!sizeof(T), "wait for add hard code");
  }
}