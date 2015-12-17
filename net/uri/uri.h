#ifndef NET_URI_URI_H
#define NET_URI_URI_H

#include <stdint.h>
#include <netinet/in.h>
#include "string/buffer.h"
#include "string/slice.h"
#include "util/move.h"

namespace net {
  namespace uri {
    class uri {
      public:
        // Constructor.
        uri();
        uri(uri&& other);

        // Destructor.
        ~uri();

        // Move assignment operator.
        uri& operator=(uri&& other);

        // Swap content.
        void swap(uri& other);

        // Clear.
        void clear();

        // Initialize from string.
        bool init(const void* s, size_t n);

        // Initialize from another URI.
        bool init(const uri& other);

        // Normalize.
        bool normalize(uri& other) const;

        // Get original URI.
        string::slice string() const;

        // Get scheme.
        const string::slice& scheme() const;

        // Get user information.
        const string::slice& userinfo() const;

        // Is IP literal?
        bool ip_literal() const;

        // Get host.
        const string::slice& host() const;

        // Get port.
        in_port_t port() const;

        // Get path.
        const string::slice& path() const;

        // Get query.
        const string::slice& query() const;

        // Get fragment.
        const string::slice& fragment() const;

      private:
        // The original URI.
        string::buffer _M_uri;

        string::slice _M_scheme;

        struct hierarchical_part {
          string::slice userinfo;
          bool ip_literal;
          string::slice host;
          in_port_t port;
          string::slice path;
        } _M_hier_part;

        string::slice _M_query;
        string::slice _M_fragment;

        // Parser.
        struct parser {
          const uint8_t* begin;
          const uint8_t* end;
          const uint8_t* p;
          size_t schemelen;
          const uint8_t* userinfo;
          size_t userinfolen;
          bool ip_literal;
          const uint8_t* host;
          size_t hostlen;
          in_port_t port;
          const uint8_t* path;
          size_t pathlen;
          const uint8_t* query;
          size_t querylen;
          const uint8_t* fragment;
          size_t fragmentlen;

          // Constructor.
          parser();

          // Initialize.
          bool init(const void* s, size_t n);

          // Advance.
          bool advance();

          // Parse scheme.
          bool parse_scheme();

          // Parse authority.
          bool parse_authority();

          // Parse IP literal.
          bool parse_ip_literal();

          // Parse path.
          bool parse_path();

          // Parse query.
          void parse_query();

          // Parse fragment.
          void parse_fragment();
        } _M_parser;

        // Initialize from parser.
        bool init_from_parser();

        // Parse scheme.
        bool parse_scheme();

        // Disable copy constructor and assignment operator.
        uri(const uri&) = delete;
        uri& operator=(const uri&) = delete;
    };

    inline uri::uri()
    {
      _M_hier_part.ip_literal = false;
      _M_hier_part.port = 0;
    }

    inline uri::uri(uri&& other)
      : _M_uri(util::move(other._M_uri)),
        _M_scheme(util::move(other._M_scheme)),
        _M_query(util::move(other._M_query)),
        _M_fragment(util::move(other._M_fragment))
    {
      _M_hier_part.userinfo         = util::move(other._M_hier_part.userinfo);

      _M_hier_part.ip_literal       = other._M_hier_part.ip_literal;
      other._M_hier_part.ip_literal = false;

      _M_hier_part.host             = util::move(other._M_hier_part.host);

      _M_hier_part.port             = other._M_hier_part.port;
      other._M_hier_part.port       = 0;

      _M_hier_part.path             = util::move(other._M_hier_part.path);
    }

    inline uri::~uri()
    {
    }

    inline uri& uri::operator=(uri&& other)
    {
      _M_uri                        = util::move(other._M_uri);

      _M_scheme                     = util::move(other._M_scheme);

      _M_hier_part.userinfo         = util::move(other._M_hier_part.userinfo);

      _M_hier_part.ip_literal       = other._M_hier_part.ip_literal;
      other._M_hier_part.ip_literal = false;

      _M_hier_part.host             = util::move(other._M_hier_part.host);

      _M_hier_part.port             = other._M_hier_part.port;
      other._M_hier_part.port       = 0;

      _M_hier_part.path             = util::move(other._M_hier_part.path);

      _M_query                      = util::move(other._M_query);
      _M_fragment                   = util::move(other._M_fragment);

      return *this;
    }

    inline void uri::swap(uri& other)
    {
      _M_uri.swap(other._M_uri);
      _M_scheme.swap(other._M_scheme);
      _M_hier_part.userinfo.swap(other._M_hier_part.userinfo);
      util::swap(_M_hier_part.ip_literal, other._M_hier_part.ip_literal);
      _M_hier_part.host.swap(other._M_hier_part.host);
      util::swap(_M_hier_part.port, other._M_hier_part.port);
      _M_hier_part.path.swap(other._M_hier_part.path);
      _M_query.swap(other._M_query);
      _M_fragment.swap(other._M_fragment);
    }

    inline void uri::clear()
    {
      _M_uri.clear();
      _M_scheme.clear();
      _M_hier_part.userinfo.clear();
      _M_hier_part.ip_literal = false;
      _M_hier_part.host.clear();
      _M_hier_part.port = 0;
      _M_hier_part.path.clear();
      _M_query.clear();
      _M_fragment.clear();
    }

    inline string::slice uri::string() const
    {
      return string::slice(_M_uri);
    }

    inline const string::slice& uri::scheme() const
    {
      return _M_scheme;
    }

    inline const string::slice& uri::userinfo() const
    {
      return _M_hier_part.userinfo;
    }

    inline bool uri::ip_literal() const
    {
      return _M_hier_part.ip_literal;
    }

    inline const string::slice& uri::host() const
    {
      return _M_hier_part.host;
    }

    inline in_port_t uri::port() const
    {
      return _M_hier_part.port;
    }

    inline const string::slice& uri::path() const
    {
      return _M_hier_part.path;
    }

    inline const string::slice& uri::query() const
    {
      return _M_query;
    }

    inline const string::slice& uri::fragment() const
    {
      return _M_fragment;
    }

    inline uri::parser::parser()
    {
      schemelen = 0;
      userinfolen = 0;
      ip_literal = false;
      hostlen = 0;
      port = 0;
      pathlen = 0;
      querylen = 0;
      fragmentlen = 0;
    }

    inline bool uri::parser::init(const void* s, size_t n)
    {
      if (n == 0) {
        return false;
      }

      begin = reinterpret_cast<const uint8_t*>(s);
      end = begin + n;

      p = begin;

      return true;
    }

    inline bool uri::parser::advance()
    {
      return (++p < end);
    }
  }
}

#endif // NET_URI_URI_H
