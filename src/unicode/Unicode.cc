#include "onmt/unicode/Unicode.h"

#include <algorithm>
#include <cstring>

#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/uscript.h>

#include "../Utils.h"


namespace onmt
{
  namespace unicode
  {

    std::string cp_to_utf8(code_point_t uc)
    {
      icu::UnicodeString uni_str(uc);
      std::string str;
      uni_str.toUTF8String(str);
      return str;
    }

    code_point_t utf8_to_cp(const unsigned char* s, unsigned int &l)
    {
      // TODO: use ICU to implement this function.
      // This was previously done in https://github.com/OpenNMT/Tokenizer/commit/3d255ce3
      // but it was causing memory issues with ICU 57.1. Was the implementation incorrect
      // or was there a bug in this specific ICU version?
      if (*s == 0 || *s >= 0xfe)
        return 0;
      if (*s <= 0x7f)
      {
        l = 1;
        return *s;
      }
      if (!s[1])
        return 0;
      if (*s < 0xe0)
      {
        l = 2;
        return ((s[0] & 0x1f) << 6) + ((s[1] & 0x3f));
      }
      if (!s[2])
        return 0;
      if (*s < 0xf0)
      {
        l = 3;
        return ((s[0] & 0x0f) << 12) + ((s[1] & 0x3f) << 6) + ((s[2] & 0x3f));
      }
      if (!s[3])
        return 0;
      if (*s < 0xf8)
      {
        l = 4;
        return (((s[0] & 0x07) << 18) + ((s[1] & 0x3f) << 12) + ((s[2] & 0x3f) << 6)
                + ((s[3] & 0x3f)));
      }

      return 0; // Incorrect unicode
    }

    std::vector<std::string> split_utf8(const std::string& str, const std::string& sep)
    {
      return split_string(str, sep);
    }

    void explode_utf8(const std::string& str,
                      std::vector<std::string>& chars,
                      std::vector<code_point_t>& code_points)
    {
      const char* c_str = str.c_str();

      chars.reserve(str.length());
      code_points.reserve(str.length());

      while (*c_str)
      {
        unsigned int char_size = 0;
        code_point_t code_point = utf8_to_cp(
          reinterpret_cast<const unsigned char*>(c_str), char_size);
        code_points.push_back(code_point);
        chars.emplace_back(c_str, char_size);
        c_str += char_size;
      }
    }

    void explode_utf8_with_marks(const std::string& str,
                                 std::vector<std::string>& chars,
                                 std::vector<code_point_t>* code_points_main,
                                 std::vector<std::vector<code_point_t>>* code_points_combining,
                                 const std::vector<code_point_t>* protected_chars)
    {
      std::vector<Char> char_info = get_characters(str, protected_chars);

      const size_t num_chars = char_info.size();
      chars.reserve(num_chars);
      if (code_points_main)
        code_points_main->reserve(num_chars);
      if (code_points_combining)
        code_points_combining->reserve(num_chars);

      for (Char& character : char_info)
      {
        chars.emplace_back(std::move(character.surface));
        if (code_points_main)
          code_points_main->emplace_back(character.value);
        if (code_points_combining)
          code_points_combining->emplace_back(std::move(character.marks));
      }
    }

    size_t utf8len(const std::string& str)
    {
      const auto* c_str = reinterpret_cast<const unsigned char*>(str.c_str());
      size_t length = 0;
      for (unsigned int char_size = 0; *c_str; ++length, c_str += char_size)
        utf8_to_cp(c_str, char_size);
      return length;
    }

    static inline CaseType get_case_type(const int8_t category)
    {
      switch (category)
      {
      case U_LOWERCASE_LETTER:
        return CaseType::Lower;
      case U_UPPERCASE_LETTER:
        return CaseType::Upper;
      default:
        return CaseType::None;
      }
    }

    static inline CharType get_char_type(const int8_t category)
    {
      switch (category)
      {
      case U_SPACE_SEPARATOR:
      case U_LINE_SEPARATOR:
      case U_PARAGRAPH_SEPARATOR:
        return CharType::Separator;

      case U_DECIMAL_DIGIT_NUMBER:
      case U_LETTER_NUMBER:
      case U_OTHER_NUMBER:
        return CharType::Number;

      case U_UPPERCASE_LETTER:
      case U_LOWERCASE_LETTER:
      case U_TITLECASE_LETTER:
      case U_MODIFIER_LETTER:
      case U_OTHER_LETTER:
        return CharType::Letter;

      case U_NON_SPACING_MARK:
      case U_ENCLOSING_MARK:
      case U_COMBINING_SPACING_MARK:
        return CharType::Mark;

      default:
        return CharType::Other;
      }
    }

    CharType get_char_type(code_point_t u)
    {
      return get_char_type(u_charType(u));
    }

    bool is_separator(code_point_t u)
    {
      return get_char_type(u) == CharType::Separator;
    }

    bool is_letter(code_point_t u)
    {
      return get_char_type(u) == CharType::Letter;
    }

    bool is_number(code_point_t u)
    {
      return get_char_type(u) == CharType::Number;
    }

    bool is_mark(code_point_t u)
    {
      return get_char_type(u) == CharType::Mark;
    }

    CaseType get_case_v2(code_point_t u)
    {
      return get_case_type(u_charType(u));
    }

    code_point_t get_lower(code_point_t u)
    {
      return u_tolower(u);
    }

    code_point_t get_upper(code_point_t u)
    {
      return u_toupper(u);
    }

    std::vector<Char>
    get_characters(const std::string& str,
                   const std::vector<code_point_t>* protected_chars)
    {
      std::vector<Char> chars;
      chars.reserve(str.size());

      const char* c_str = str.c_str();
      for (unsigned int char_size = 0; *c_str; c_str += char_size)
      {
        const auto code_point = utf8_to_cp(reinterpret_cast<const unsigned char*>(c_str),
                                           char_size);
        const auto category = u_charType(code_point);
        const auto char_type = get_char_type(category);

        if (chars.empty()
            || char_type != CharType::Mark
            || (protected_chars
                && std::find(protected_chars->begin(),
                             protected_chars->end(),
                             chars.back().value) != protected_chars->end()))
        {
          chars.emplace_back(std::string(c_str, char_size),
                             code_point,
                             char_type,
                             get_case_type(category));
        }
        else
        {
          chars.back().surface.append(c_str, char_size);
          chars.back().marks.emplace_back(code_point);
        }
      }

      return chars;
    }

    // The functions below are made backward compatible with the Kangxi and Kanbun script names
    // that were previously declared in Alphabet.h but are not Unicode script aliases.
    static const std::vector<std::pair<std::pair<const char*, int>,
                                       std::pair<code_point_t, code_point_t>>>
    compat_scripts = {
      {{"Kangxi", USCRIPT_CODE_LIMIT + 0}, {0x2F00, 0x2FD5}},
      {{"Kanbun", USCRIPT_CODE_LIMIT + 1}, {0x3190, 0x319F}},
    };

    int get_script_code(const char* script_name)
    {
      for (const auto& pair : compat_scripts)
      {
        const auto& script_info = pair.first;
        if (strcmp(script_name, script_info.first) == 0)
          return script_info.second;
      }

      return u_getPropertyValueEnum(UCHAR_SCRIPT, script_name);
    }

    const char* get_script_name(int script_code)
    {
      for (const auto& pair : compat_scripts)
      {
        const auto& script_info = pair.first;
        if (script_info.second == script_code)
          return script_info.first;
      }

      return uscript_getName(static_cast<UScriptCode>(script_code));
    }

    int get_script(code_point_t c)
    {
      for (const auto& pair : compat_scripts)
      {
        const auto& range = pair.second;
        if (c >= range.first && c <= range.second)
          return pair.first.second;
      }

      UErrorCode error = U_ZERO_ERROR;
      return static_cast<int>(uscript_getScript(c, &error));
    }

  }
}
