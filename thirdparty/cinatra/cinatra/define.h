#pragma once
#include <filesystem>
#include <string_view>
#include <unordered_map>
namespace fs = std::filesystem;

namespace cinatra {
enum class content_type {
  string,
  multipart,
  urlencoded,
  chunked,
  octet_stream,
  websocket,
  unknown,
};

enum class req_content_type { html, json, string, multipart, ranges, none };

constexpr inline auto HTML = req_content_type::html;
constexpr inline auto JSON = req_content_type::json;
constexpr inline auto TEXT = req_content_type::string;
constexpr inline auto RANGES = req_content_type::ranges;
constexpr inline auto NONE = req_content_type::none;

inline const std::string_view STATIC_RESOURCE = "cinatra_static_resource";
inline const std::string CSESSIONID = "CSESSIONID";

const static inline std::string CRCF = "\r\n";
const static inline std::string TWO_CRCF = "\r\n\r\n";
const static inline std::string BOUNDARY = "--CinatraBoundary2B8FAF4A80EDB307";
const static inline std::string MULTIPART_END =
    CRCF + "--" + BOUNDARY + "--" + TWO_CRCF;

inline std::unordered_map<std::string, std::string> g_content_type_map = {
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".js", "text/javascript"},
    {".mjs", "text/javascript"},
    {".txt", "text/plain"},
    {".vtt", "text/vtt"},
    {".apng", "image/apng"},
    {".avif", "image/avif"},
    {".bmp", "image/bmp"},
    {".gif", "image/gif"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".webp", "image/webp"},
    {".ico", "image/x-icon"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".mp4", "video/mp4"},
    {".mpeg", "video/mpeg"},
    {".webm", "video/webm"},
    {".mp3", "audio/mp3"},
    {".mpga", "audio/mpeg"},
    {".weba", "audio/webm"},
    {".wav", "audio/wave"},
    {".otf", "font/otf"},
    {".ttf", "font/ttf"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".7z", "application/x-7z-compressed"},
    {".atom", "application/atom+xml"},
    {".pdf", "application/pdf"},
    {".json", "application/json"},
    {".rss", "application/rss+xml"},
    {".tar", "application/x-tar"},
    {"xht", "application/xhtml+xml"},
    {"xhtml", "application/xhtml+xml"},
    {"xslt", "application/xslt+xml"},
    {"xml", "application/xml"},
    {"gz", "application/gzip"},
    {"zip", "application/zip"},
    {"wasm", "application/wasm"}};
struct NonSSL {};
struct SSL {};
}  // namespace cinatra
