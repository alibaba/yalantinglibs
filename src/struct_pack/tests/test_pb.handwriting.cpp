#include "test_pb.handwriting.hpp"


template <>
constexpr std::size_t first_field_number<test2> = 2;


template <>
constexpr std::size_t first_field_number<test3> = 3;


template <>
constexpr std::size_t first_field_number<test4> = 4;


template <>
constexpr std::size_t first_field_number<my_test_enum> = 6;

template <>
constexpr std::size_t first_field_number<my_test_sint64> = 2;

template <>
constexpr std::size_t first_field_number<my_test_map> = 3;
template <>
constexpr field_number_array_t<my_test_field_number_random>
    field_number_seq<my_test_field_number_random>{6, 3, 4, 5, 1, 128};

template<>
constexpr oneof_field_number_array_t<sample_message_oneof, 0>
    oneof_field_number_seq<sample_message_oneof, 0>{
        10, 8, 4, 9
    };