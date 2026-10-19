#include "sie/cp437.hpp"

#include <array>

namespace cratchit::sie {

namespace {

// Unicode code points for bytes 0x80-0xFF
constexpr std::array<char32_t, 128> upper_half{
    U'Ç', U'ü', U'é', U'â', U'ä', U'à', U'å', U'ç', U'ê', U'ë', U'è', U'ï', U'î', U'ì', U'Ä', U'Å',
    U'É', U'æ', U'Æ', U'ô', U'ö', U'ò', U'û', U'ù', U'ÿ', U'Ö', U'Ü', U'¢', U'£', U'¥', U'₧', U'ƒ',
    U'á', U'í', U'ó', U'ú', U'ñ', U'Ñ', U'ª', U'º', U'¿', U'⌐', U'¬', U'½', U'¼', U'¡', U'«', U'»',
    U'░', U'▒', U'▓', U'│', U'┤', U'╡', U'╢', U'╖', U'╕', U'╣', U'║', U'╗', U'╝', U'╜', U'╛', U'┐',
    U'└', U'┴', U'┬', U'├', U'─', U'┼', U'╞', U'╟', U'╚', U'╔', U'╩', U'╦', U'╠', U'═', U'╬', U'╧',
    U'╨', U'╤', U'╥', U'╙', U'╘', U'╒', U'╓', U'╫', U'╪', U'┘', U'┌', U'█', U'▄', U'▌', U'▐', U'▀',
    U'α', U'ß', U'Γ', U'π', U'Σ', U'σ', U'µ', U'τ', U'Φ', U'Θ', U'Ω', U'δ', U'∞', U'φ', U'ε', U'∩',
    U'≡', U'±', U'≥', U'≤', U'⌠', U'⌡', U'÷', U'≈', U'°', U'∙', U'·', U'√', U'ⁿ', U'²', U'■', U' ',
};

void append_utf8(std::string& out, char32_t cp) {
  if (cp < 0x80) {
    out += static_cast<char>(cp);
  } else if (cp < 0x800) {
    out += static_cast<char>(0xC0 | (cp >> 6));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    out += static_cast<char>(0xE0 | (cp >> 12));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  }
}

} // namespace

std::string cp437_to_utf8(std::string_view cp437) {
  std::string out;
  out.reserve(cp437.size());
  for (char const byte : cp437) {
    auto const c = static_cast<unsigned char>(byte);
    if (c < 0x80) {
      out += static_cast<char>(c);
    } else {
      append_utf8(out, upper_half[c - 0x80]);
    }
  }
  return out;
}

} // namespace cratchit::sie
