#include <ylt/util/meta_numeric_conversion.hpp>
#include <ylt/util/sha3_constexpr.hpp>

namespace ylt {
constexpr void test_meta_numeric_conversion() {
  static_assert(to_hexadecimal_meta_string(static_cast<std::uint32_t>(
                    0x12345)) == refvalue::meta_string{"00012345"});

  static_assert(
      to_hexadecimal_meta_string(static_cast<std::uint32_t>(0x12345), false) ==
      refvalue::meta_string{"45230100"});

  static_assert(to_hexadecimal_meta_string(static_cast<std::uint64_t>(
                    0x6789ABCD)) == refvalue::meta_string{"000000006789ABCD"});

  static_assert(to_hexadecimal_meta_string(
                    static_cast<std::uint64_t>(0x6789ABCD), false) ==
                refvalue::meta_string{"CDAB896700000000"});

  static_assert(to_hexadecimal_meta_string(
                    std::array<std::uint8_t, 4>{0x12, 0x34, 0x60, 0xAB}) ==
                refvalue::meta_string{"123460AB"});
}

constexpr void test_sha3_constexpr() noexcept {
  static_assert(
      to_hexadecimal_meta_string(
          sha3_digest<sha3_type::sha3_224>("This is a UTF-8 string.")) ==
      refvalue::meta_string{
          "E41EA1F40E1378DFA4A0847D1BF7EACEF488622BFA9839DAA6C64FE8"});

  static_assert(
      to_hexadecimal_meta_string(
          sha3_digest<sha3_type::sha3_256>("Hello World!")) ==
      refvalue::meta_string{
          "D0E47486BBF4C16ACAC26F8B653592973C1362909F90262877089F9C8A4536AF"});

  static_assert(
      to_hexadecimal_meta_string(sha3_digest<sha3_type::sha3_384>(
          "Whenever you want to, I can help.")) ==
      refvalue::meta_string{"4ACB502EEC4FE8ECDA6E0A8D386FFE6B9B24B8CE2E22F6C8AB"
                            "C729EC521D361CD883044B720458DCC7472906CC49D9D5"});

  static_assert(
      to_hexadecimal_meta_string(
          sha3_digest<sha3_type::sha3_512>("Forgive me and leave me alone.")) ==
      refvalue::meta_string{
          "6377D5E3BBA8BB2DC3A956E458F00F0BED5716B0AB419F593DCD7BFE16FB5C4D7F42"
          "FC2B06C7DB073CE849A6122845558D58C7BBA2CDCD01A6F046FF9B35BD3D"});
}
}  // namespace ylt

int main() {}