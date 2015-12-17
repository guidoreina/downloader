#ifndef NET_HTTP_HEADER_CTYPE_H
#define NET_HTTP_HEADER_CTYPE_H

namespace net {
  namespace http {
    namespace header {
      static inline bool header_name_valid_character(uint8_t c)
      {
        static const bool characters[] = {
          //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
          /* 0x00 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
          /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
          /* 0x20 */ false, true,  false, true,  true,  true,  true,  true,  false, false, true,  true,  false, true,  true,  false,
          /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false, false,
          /* 0x40 */ false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, false, false, true,  true,
          /* 0x60 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false, true,  false, true,  false,
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

      static inline bool header_value_valid_character(uint8_t c)
      {
        static const bool characters[] = {
          //          0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
          /* 0x00 */ false, false, false, false, false, false, false, false, false, true,  true,  false, false, true,  false, false,
          /* 0x10 */ false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
          /* 0x20 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0x30 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0x40 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0x50 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0x60 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0x70 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
          /* 0x80 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0x90 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0xa0 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0xb0 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0xc0 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0xd0 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0xe0 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
          /* 0xf0 */ true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true
        };

        return characters[c];
      }
    }
  }
}

#endif // NET_HTTP_HEADER_CTYPE_H
