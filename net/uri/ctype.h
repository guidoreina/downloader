#ifndef NET_URI_CTYPE_H
#define NET_URI_CTYPE_H

#include "util/ctype.h"

namespace net {
  namespace uri {
    static inline bool is_reserved(uint8_t c)
    {
      // reserved    = gen-delims / sub-delims
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, true,  false, true,  true,  false, true,  true,  true,  true,  true,  true,  true,  false, false, true,
        /* 0x30 */ false, false, false, false, false, false, false, false, false, false, true,  true,  false, true,  false, true,
        /* 0x40 */ true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x50 */ false, false, false, false, false, false, false, false, false, false, false, true,  false, true,  false, false,
        /* 0x60 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x70 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return characters[c];
    }

    static inline bool is_gen_delim(uint8_t c)
    {
      // gen-delims  = ":" / "/" / "?" / "#" / "[" / "]" / "@"
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, false, false, true,  false, false, false, false, false, false, false, false, false, false, false, true,
        /* 0x30 */ false, false, false, false, false, false, false, false, false, false, true,  false, false, false, false, true,
        /* 0x40 */ true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x50 */ false, false, false, false, false, false, false, false, false, false, false, true,  false, true,  false, false,
        /* 0x60 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x70 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return characters[c];
    }

    static inline bool is_sub_delim(uint8_t c)
    {
      // sub-delims  = "!" / "$" / "&" / "'" / "(" / ")"
      //             / "*" / "+" / "," / ";" / "="
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, true,  false, false, true,  false, true,  true,  true,  true,  true,  true,  true,  false, false, false,
        /* 0x30 */ false, false, false, false, false, false, false, false, false, false, false, true,  false, true,  false, false,
        /* 0x40 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x50 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x60 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x70 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return characters[c];
    }

    static inline bool is_unreserved(uint8_t c)
    {
      // unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, false, false, false, false, false, false, false, false, false, false, false, false, true,  true,  false,
        /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false, false,
        /* 0x40 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, true,
        /* 0x60 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return characters[c];
    }

    static inline bool is_pct_encoded(const uint8_t* start, const uint8_t* end)
    {
      return ((start + 2 < end) &&
              (start[0] == '%') &&
              (util::is_xdigit(start[1])) &&
              (util::is_xdigit(start[2])));
    }

    static inline bool is_pchar(const uint8_t* start, const uint8_t* end)
    {
      // pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, true,  false, false, true,  false, true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
        /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, true,  false, false,
        /* 0x40 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, true,
        /* 0x60 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return ((characters[*start]) || (is_pct_encoded(start, end)));
    }

    static inline bool is_valid_scheme_char(uint8_t c)
    {
      // scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, false, false, false, false, false, false, false, false, false, false, true,  false, true,  true,  false,
        /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false, false,
        /* 0x40 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false,
        /* 0x60 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return characters[c];
    }

    static inline bool is_valid_userinfo_char(const uint8_t* start,
                                              const uint8_t* end)
    {
      // userinfo    = *( unreserved / pct-encoded / sub-delims / ":" )
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, true,  false, false, true,  false, true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
        /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, true,  false, false,
        /* 0x40 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, true,
        /* 0x60 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return ((characters[*start]) || (is_pct_encoded(start, end)));
    }

    static inline bool is_valid_ip_literal_char(uint8_t c)
    {
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, true,  false,
        /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false,
        /* 0x40 */ false, true,  true,  true,  true,  true,  true,  false, false, false, false, false, false, false, false, false,
        /* 0x50 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x60 */ false, true,  true,  true,  true,  true,  true,  false, false, false, false, false, false, false, false, false,
        /* 0x70 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return characters[c];
    }

    static inline bool is_valid_reg_name_char(const uint8_t* start,
                                              const uint8_t* end)
    {
      // reg-name    = *( unreserved / pct-encoded / sub-delims )
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, true,  false, false, true,  false, true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
        /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, true,  false, true,  false, false,
        /* 0x40 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, true,
        /* 0x60 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return ((characters[*start]) || (is_pct_encoded(start, end)));
    }

    static inline bool is_valid_path_char(const uint8_t* start,
                                          const uint8_t* end)
    {
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, true,  false, false, true,  false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, true,  false, false,
        /* 0x40 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, true,
        /* 0x60 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return ((characters[*start]) || (is_pct_encoded(start, end)));
    }

    static inline bool is_valid_query_or_fragment_char(const uint8_t* start,
                                                       const uint8_t* end)
    {
      static const bool characters[] = {
        //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
        /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x20 */ false, true,  false, false, true,  false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, true,  false, true,
        /* 0x40 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, true,
        /* 0x60 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
        /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  false,
        /* 0x80 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0x90 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xa0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xb0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xc0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xd0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xe0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        /* 0xf0 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
      };

      return ((characters[*start]) || (is_pct_encoded(start, end)));
    }
  }
}

#endif // NET_URI_CTYPE_H
