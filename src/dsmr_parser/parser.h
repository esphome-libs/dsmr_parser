#pragma once

#include "util.h"

namespace dsmr_parser {

// uses polynomial x^16+x^15+x^2+1
inline uint16_t crc16_update(uint16_t crc, uint8_t data) {
  crc ^= data;
  for (size_t i = 0; i < 8; ++i) {
    if (crc & 1) {
      crc = (crc >> 1) ^ 0xA001;
    } else {
      crc = (crc >> 1);
    }
  }
  return crc;
}

// ParsedData is a template for the result of parsing a Dsmr P1 message.
// You pass the fields you want to add to it as template arguments.
//
// This template will then generate a class that extends all the fields
// passed (the fields really are classes themselves). Since each field
// class has a single member variable, with the same name as the field
// class, all of these fields will be available on the generated class.
//
// In other words, if I have:
//
// using MyData = ParsedData<
//  identification,
//  equipment_id
// >;
//
// MyData data;
//
// then I can refer to the fields like data.identification and
// data.equipment_id normally.
//
// Furthermore, this class offers some helper methods that can be used
// to loop over all the fields inside it.
template <typename... Ts>
struct ParsedData : Ts... {
  ParseResult<void> parse_line(const ObisId& obisId, const char* str, const char* end) {
    ParseResult<void> res;
    const auto& try_one = [&](auto& field) -> bool {
      using FieldType = std::decay_t<decltype(field)>;
      if (obisId != FieldType::id) {
        return false;
      }

      if (field.present())
        res = ParseResult<void>().fail("Duplicate field", str);
      else {
        field.present() = true;
        res = field.parse(str, end);
      }
      return true;
    };

    const bool found = (try_one(static_cast<Ts&>(*this)) || ...);
    return found ? res : ParseResult<void>().until(str);
  }

  template <typename F>
  void applyEach(F&& f) {
    (Ts::apply(f), ...);
  }

  bool all_present() { return (Ts::present() && ...); }
};

struct StringParser {
  static ParseResult<std::string> parse_string(size_t min, size_t max, const char* str, const char* end) {
    ParseResult<std::string> res;
    if (str >= end || *str != '(')
      return res.fail("Missing (", str);

    const char* str_start = str + 1; // Skip (
    const char* str_end = str_start;

    while (str_end < end && *str_end != ')')
      ++str_end;

    if (str_end == end)
      return res.fail("Missing )", str_end);

    const auto& len = static_cast<size_t>(str_end - str_start);
    if (len < min || len > max)
      return res.fail("Invalid string length", str_start);

    res.result.append(str_start, len);

    return res.until(str_end + 1); // Skip )
  }
};

static constexpr char INVALID_NUMBER[] = "Invalid number";
static constexpr char INVALID_UNIT[] = "Invalid unit";

struct NumParser {
  static ParseResult<uint32_t> parse(size_t max_decimals, const char* unit, const char* str, const char* end) {
    ParseResult<uint32_t> res;
    if (str >= end || *str != '(')
      return res.fail("Missing (", str);

    const char* num_start = str + 1; // Skip (
    const char* num_end = num_start;

    uint32_t value = 0;

    // Parse integer part
    while (num_end < end && !strchr("*.)", *num_end)) {
      if (*num_end < '0' || *num_end > '9')
        return res.fail(INVALID_NUMBER, num_end);
      value *= 10;
      value += static_cast<uint32_t>(*num_end - '0');
      ++num_end;
    }

    // Parse decimal part, if any
    if (max_decimals && num_end < end && *num_end == '.') {
      ++num_end;

      while (num_end < end && !strchr("*)", *num_end) && max_decimals) {
        max_decimals--;
        if (*num_end < '0' || *num_end > '9')
          return res.fail(INVALID_NUMBER, num_end);
        value *= 10;
        value += static_cast<uint32_t>(*num_end - '0');
        ++num_end;
      }
    }

    // Fill in missing decimals with zeroes
    while (max_decimals--)
      value *= 10;

    // Workaround for https://github.com/matthijskooijman/arduino-dsmr/issues/50
    // If value is 0, then we allow missing unit.
    if (unit && *unit && (num_end >= end || (*num_end != '*' && *num_end != '.')) && value == 0) {
      num_end = std::find(num_end, end, ')');
    }

    // If a unit was passed, check that the unit in the messages
    // messages the unit passed.
    else if (unit && *unit) {
      if (num_end >= end || *num_end != '*')
        return res.fail("Missing unit", num_end);
      const char* unit_start = ++num_end; // skip *
      while (num_end < end && *num_end != ')' && *unit) {
        // Next character in units do not match?
        if (std::tolower(static_cast<unsigned char>(*num_end++)) != std::tolower(static_cast<unsigned char>(*unit++)))
          return res.fail(INVALID_UNIT, unit_start);
      }
      // At the end of the message unit, but not the passed unit?
      if (*unit)
        return res.fail(INVALID_UNIT, unit_start);
    }

    if (num_end >= end || *num_end != ')')
      return res.fail("Extra data", num_end);

    return res.succeed(value).until(num_end + 1); // Skip )
  }
};

struct ObisIdParser {
  static ParseResult<ObisId> parse(const char* str, const char* end) {
    // Parse a Obis ID of the form 1-2:3.4.5.6
    // Stops parsing on the first unrecognized character. Any unparsed
    // parts are set to 255.
    ParseResult<ObisId> res;
    ObisId& id = res.result;
    res.next = str;
    uint8_t part = 0;
    while (res.next < end) {
      char c = *res.next;

      if (c >= '0' && c <= '9') {
        const auto& digit = c - '0';
        if (id.v[part] > 25 || (id.v[part] == 25 && digit > 5))
          return res.fail("Obis ID has number over 255", res.next);
        id.v[part] = static_cast<uint8_t>(id.v[part] * 10 + digit);
      } else if (part == 0 && c == '-') {
        part++;
      } else if (part == 1 && c == ':') {
        part++;
      } else if (part > 1 && part < 5 && c == '.') {
        part++;
      } else {
        break;
      }
      ++res.next;
    }

    if (res.next == str)
      return res.fail("OBIS id Empty", str);

    for (++part; part < 6; ++part)
      id.v[part] = 255;

    return res;
  }
};

struct CrcParser {
private:
  static const size_t CRC_LEN = 4;

  static bool hex_nibble(char c, uint8_t& out) {
    if (c >= '0' && c <= '9') {
      out = static_cast<uint8_t>(c - '0');
      return true;
    }
    if (c >= 'A' && c <= 'F') {
      out = static_cast<uint8_t>(c - 'A' + 10);
      return true;
    }
    if (c >= 'a' && c <= 'f') {
      out = static_cast<uint8_t>(c - 'a' + 10);
      return true;
    }
    return false;
  }

public:
  // Parse a crc value. str must point to the first of the four hex
  // bytes in the CRC.
  static ParseResult<uint16_t> parse(const char* str, const char* end) {
    ParseResult<uint16_t> res;

    if (str + CRC_LEN > end)
      return res.fail("No checksum found", str);

    uint16_t value = 0;
    for (size_t i = 0; i < CRC_LEN; ++i) {
      uint8_t nibble;
      if (!hex_nibble(str[i], nibble))
        return res.fail("Incomplete or malformed checksum", str + i);
      value = static_cast<uint16_t>((value << 4) | nibble);
    }

    res.next = str + CRC_LEN;
    return res.succeed(value);
  }
};

struct P1Parser {

  // Parse a complete P1 telegram. The string passed should start
  // with '/' and run up to and including the ! and the following
  // four byte checksum. It's ok if the string is longer, the .next
  // pointer in the result will indicate the next unprocessed byte.
  template <typename... Ts>
  static ParseResult<void> parse(ParsedData<Ts...>* data, const char* str, size_t n, bool unknown_error = false, bool check_crc = true) {
    ParseResult<void> res;

    const char* const buf_begin = str;
    const char* const buf_end = str + n;

    if (!n || *buf_begin != '/')
      return res.fail("Data should start with /", buf_begin);

    // The payload starts after '/', and runs up to (but not including) '!'
    const char* const data_begin = buf_begin + 1;

    // Find the terminating '!' (or the end of buffer if not present)
    const char* term = std::find(data_begin, buf_end, '!');
    if (term == buf_end)
      return res.fail("Data should end with !");

    if (check_crc) {
      // With CRC enabled, '!' must exist and be followed by 4 hex chars.
      if (term >= buf_end)
        return res.fail("No checksum found", term);

      // Compute CRC over '/' .. '!' (inclusive).
      uint16_t crc = 0;
      for (const char* p = buf_begin; p <= term; ++p)
        crc = crc16_update(crc, static_cast<uint8_t>(*p));

      // Parse and verify the 4-hex checksum after '!'
      ParseResult<uint16_t> check = CrcParser::parse(term + 1, buf_end);
      if (check.err)
        return check;
      if (check.result != crc)
        return res.fail("Checksum mismatch", term + 1);

      // Parse payload (between '/' and '!')
      res = parse_data(data, data_begin, term, unknown_error);
      res.next = check.next; // Advance past checksum
      return res;
    }

    // No CRC checking: parse up to '!' if present, otherwise up to buf_end.
    res = parse_data(data, data_begin, term, unknown_error);
    res.next = (term < buf_end) ? term : buf_end;
    return res;
  }

  // Parse the data part of a message. Str should point to the first
  // character after the leading /, end should point to the ! before the
  // checksum. Does not verify the checksum.
  template <typename... Ts>
  static ParseResult<void> parse_data(ParsedData<Ts...>* data, const char* str, const char* end, bool unknown_error = false) {
    // Split into lines and parse those
    const char* line_end = str;
    const char* line_start = str;

    // Parse ID line
    while (line_end < end) {
      if (*line_end == '\r' || *line_end == '\n') {
        // The first identification line looks like:
        // XXX5<id string>
        // The DSMR spec is vague on details, but in 62056-21, the X's
        // are a three-letter (registerd) manufacturer ID, the id
        // string is up to 16 chars of arbitrary characters and the
        // '5' is a baud rate indication. 5 apparently means 9600,
        // which DSMR 3.x and below used. It seems that DSMR 2.x
        // passed '3' here (which is mandatory for "mode D"
        // communication according to 62956-21), so we also allow
        // that. Apparently swedish meters use '9' for 115200. This code
        // used to check the format of the line somewhat, but for
        // flexibility (and since we do not actually parse the contents
        // of the line anyway), just allow anything now.
        //
        // Offer it for processing using the all-ones Obis ID, which
        // is not otherwise valid.
        ParseResult<void> tmp = data->parse_line(ObisId(255, 255, 255, 255, 255, 255), line_start, line_end);
        if (tmp.err)
          return tmp;
        line_start = ++line_end;
        break;
      }
      ++line_end;
    }

    // Parse data lines
    // We need to track brackets to handle cases like:
    //   0-0:96.13.0(303132333435
    //   30313233343)
    bool open_bracket_found = false;
    while (line_end < end) {
      char c = *line_end;

      if (c == '(') {
        if (open_bracket_found) {
          return ParseResult<void>().fail("Unexpected '(' symbol", line_end);
        }
        open_bracket_found = true;
      } else if (c == ')') {
        if (!open_bracket_found) {
          return ParseResult<void>().fail("Unexpected ')' symbol", line_end);
        }
        open_bracket_found = false;
      } else if (c == '\r' || c == '\n') {

        // handles case like:
        //  0-1:24.3.0(120517020000)(08)(60)(1)(0-1:24.2.1)(m3)
        //  (00124.477)
        const auto& next_part_of_the_data_line_on_next_line = (end - line_end > 2) && (line_end[1] == '(' || line_end[2] == '(');

        const auto& break_in_the_middle_of_the_data_line = open_bracket_found || next_part_of_the_data_line_on_next_line;

        if (!break_in_the_middle_of_the_data_line) {
          // End of logical line -> parse it
          ParseResult<void> tmp = parse_line(data, line_start, line_end, unknown_error);
          if (tmp.err)
            return tmp;

          line_start = line_end + 1;
        }
      }

      ++line_end;
    }

    if (line_end != line_start)
      return ParseResult<void>().fail("Last dataline not CRLF terminated", line_end);

    return ParseResult<void>();
  }

  template <typename Data>
  static ParseResult<void> parse_line(Data* data, const char* line, const char* end, bool unknown_error) {
    ParseResult<void> res;
    if (line == end)
      return res;

    ParseResult<ObisId> idres = ObisIdParser::parse(line, end);
    if (idres.err)
      return idres;

    ParseResult<void> datares = data->parse_line(idres.result, idres.next, end);
    if (datares.err)
      return datares;

    // If datares.next didn't move at all, there was no parser for
    // this field, that's ok. But if it did move, but not all the way
    // to the end, that's an error.
    if (datares.next != idres.next && datares.next != end)
      return res.fail("Trailing characters on data line", datares.next);
    else if (datares.next == idres.next && unknown_error)
      return res.fail("Unknown field", line);

    return res.until(end);
  }
};

}
