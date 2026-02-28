/**
 * @file UncText.cpp
 * A simple class that handles the chunk text.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "unc_text.h"

#include "unc_ctype.h"
#include "unicode.h" // encode_utf8()
#include <string>    // Added for string

#include <algorithm>
#include <cstdint>
#include <stdexcept>


using namespace std;


static constexpr const int_fast8_t UTF8_BLOCKS = 6;  // 6 -> max utf8 blocks per char


static size_t fix_len_idx(size_t size, size_t idx, size_t len);

//! converts \n and \r chars are into NL and CR UTF8 symbols before encode_utf8 is called
static void toLogTextUtf8(int c, UncText::log_type &container);

/**
 * calculates the size a 'log_type' container needs to have in order to take
 * in values of a 'UncText::value_type' up to idx
 * (without \0, with symbols for the converted \n and \r chars)
 *
 * throws if char is greater than 0x7fffffff
 */
static int getLogTextUtf8Len(const UncText::value_type &c0, size_t end);

static int getLogTextUtf8Len(const UncText::value_type &c0, size_t start, size_t end);


static int getLogTextUtf8Len(const UncText::value_type &c0, size_t start, size_t end)
{
   size_t c1_idx = 0;

   for (size_t i = start; i < end; ++i)
   {
      auto ch = c0[i];

      if (ch == '\n')
      {
         ch = 0x2424;  // NL symbol
      }
      else if (ch == '\r')
      {
         ch = 0x240d;  // CR symbol
      }

      if (ch < 0x80)              // 1-byte sequence
      {
         c1_idx += 1;
      }
      else if (ch < 0x0800)       // 2-byte sequence
      {
         c1_idx += 2;
      }
      else if (ch < 0x10000)      // 3-byte sequence
      {
         c1_idx += 3;
      }
      else if (ch < 0x200000)     // 4-byte sequence
      {
         c1_idx += 4;
      }
      else if (ch < 0x4000000)    // 5-byte sequence
      {
         c1_idx += 5;
      }
      else if (ch <= 0x7fffffff)  // 6-byte sequence
      {
         c1_idx += 6;
      }
      else
      {
         throw out_of_range(string(__func__) + ":" + to_string(__LINE__)
                            + " - ch value too big, can't convert to utf8");
      }
   }

   return(c1_idx);
} // getLogTextUTF8Len


static int getLogTextUtf8Len(const UncText::value_type &c0, size_t end)
{
   return(getLogTextUtf8Len(c0, 0, end));
}


static void toLogTextUtf8(int c, UncText::log_type &container)
{
   if (c == '\n')
   {
      c = 0x2424; // NL symbol
   }
   else if (c == '\r')
   {
      c = 0x240d; // CR symbol
   }
   encode_utf8(c, container);
}


static size_t fix_len_idx(size_t size, size_t idx, size_t len)
{
   if (idx >= size)
   {
      return(0);
   }
   const size_t left = size - idx;

   return((len > left) ? left : len);
}


UncText::UncText()
{
   m_logText = log_type{ '\0' };
}


UncText::UncText(const UncText &ref)
{
   set(ref);
}


UncText::UncText(const UncText &ref, size_t idx, size_t len)
{
   set(ref, idx, len);
}


UncText::UncText(const char *ascii_text)
{
   set(ascii_text);
}


UncText::UncText(const std::string &ascii_text)
{
   set(ascii_text);
}


UncText::UncText(const value_type &data, size_t idx, size_t len)
{
   set(data, idx, len);
}


size_t UncText::size() const
{
   return(m_unicodeText.size());
}


UncText &UncText::operator=(int ch)
{
   set(ch);
   return(*this);
}


UncText &UncText::operator=(const UncText &ref)
{
   set(ref);
   return(*this);
}


UncText &UncText::operator=(const std::string &ascii_text)
{
   set(ascii_text);
   return(*this);
}


UncText &UncText::operator=(const char *ascii_text)
{
   set(ascii_text);
   return(*this);
}


UncText &UncText::operator+=(int ch)
{
   append(ch);
   return(*this);
}


UncText &UncText::operator+=(const UncText &ref)
{
   append(ref);
   return(*this);
}


UncText &UncText::operator+=(const std::string &ascii_text)
{
   append(ascii_text);
   return(*this);
}


UncText &UncText::operator+=(const char *ascii_text)
{
   append(ascii_text);
   return(*this);
}


const UncText::value_type &UncText::get() const
{
   return(m_unicodeText);
}


int UncText::operator[](size_t idx) const
{
   return((idx < m_unicodeText.size()) ? m_unicodeText[idx] : 0);
}


const int &UncText::at(size_t idx) const
{
   return(m_unicodeText.at(idx));
}


int &UncText::at(size_t idx)
{
   return(m_unicodeText.at(idx));
}


const int &UncText::back() const
{
   return(m_unicodeText.back());
}


void UncText::push_back(int ch)
{
   append(ch);
}


void UncText::pop_back()
{
   if (size() == 0)
   {
      return;
   }
   m_unicodeText.pop_back();
   update_logtext();
}


void UncText::pop_front()
{
   if (size() == 0)
   {
      return;
   }
   m_unicodeText.pop_front();
   update_logtext();
}


void UncText::update_logtext()
{
   // make a pessimistic guess at the size
   m_logText.clear();
   m_logText.reserve(m_unicodeText.size() * 3);

   for (int m_char : m_unicodeText)
   {
      toLogTextUtf8(m_char, m_logText);
   }

   m_logText.push_back(0);
}


int UncText::compare(const UncText &ref1, const UncText &ref2, size_t len, bool tcare)
{
   const size_t len1    = ref1.size();
   const size_t len2    = ref2.size();
   const auto   max_idx = std::min({ len, len1, len2 });
   size_t       idx     = 0;

   for ( ; idx < max_idx; idx++)
   {
      // exactly the same character ?
      if (ref1.m_unicodeText[idx] == ref2.m_unicodeText[idx])
      {
         continue;
      }
      int diff;                                             // Issue #2091

      if (tcare)
      {
         diff = ref1.m_unicodeText[idx] - ref2.m_unicodeText[idx];
      }
      else
      {
         diff = unc_tolower(ref1.m_unicodeText[idx]) - unc_tolower(ref2.m_unicodeText[idx]);
      }

      if (diff == 0)
      {
         /*
          * if we're comparing the same character but in different case
          * we want to favor lower case before upper case (e.g. a before A)
          * so the order is the reverse of ASCII order (we negate).
          */
         return(-(ref1.m_unicodeText[idx] - ref2.m_unicodeText[idx]));
      }
      // return the case-insensitive diff to sort alphabetically
      return(diff);
   }

   if (idx == len)
   {
      return(0);
   }
   // underflow save: return(len1 - len2);
   return((len1 > len2) ? (len1 - len2) : -static_cast<int>(len2 - len1));
} // UncText::compare


bool UncText::equals(const UncText &ref) const
{
   const size_t len = size();

   if (ref.size() != len)
   {
      return(false);
   }

   for (size_t idx = 0; idx < len; idx++)
   {
      if (m_unicodeText[idx] != ref.m_unicodeText[idx])
      {
         return(false);
      }
   }

   return(true);
}


const char *UncText::GetLogText() const
{
   return(reinterpret_cast<const char *>(&m_logText[0]));
}


void UncText::set(int ch)
{
   m_logText.clear();
   toLogTextUtf8(ch, m_logText);
   m_logText.push_back('\0');


   m_unicodeText.clear();
   m_unicodeText.push_back(ch);
}


void UncText::set(const UncText &ref)
{
   m_unicodeText = ref.m_unicodeText;
   m_logText     = ref.m_logText;
}


void UncText::set(const UncText &ref, size_t idx, size_t len)
{
   const auto ref_size = ref.size();

   if (len == ref_size)
   {
      m_unicodeText = ref.m_unicodeText;
      update_logtext();
      return;
   }
   m_unicodeText.resize(len);

   len = fix_len_idx(ref_size, idx, len);

   for (size_t di = 0;
        len > 0;
        di++, idx++, len--)
   {
      m_unicodeText[di] = ref.m_unicodeText[idx];
   }

   update_logtext();
}


void UncText::set(const string &ascii_text)
{
   const size_t len = ascii_text.size();

   m_unicodeText.resize(len);

   for (size_t idx = 0; idx < len; idx++)
   {
      m_unicodeText[idx] = ascii_text[idx];
   }

   update_logtext();
}


void UncText::set(const char *ascii_text)
{
   const size_t len = strlen(ascii_text);

   m_unicodeText.resize(len);

   for (size_t idx = 0; idx < len; idx++)
   {
      m_unicodeText[idx] = *ascii_text++;
   }

   update_logtext();
}


void UncText::set(const value_type &data, size_t idx, size_t len)
{
   m_unicodeText.resize(len);

   len = fix_len_idx(data.size(), idx, len);

   for (size_t di = 0;
        len > 0;
        di++, idx++, len--)
   {
      m_unicodeText[di] = data[idx];
   }

   update_logtext();
}


void UncText::resize(size_t new_size)
{
   if (size() == new_size)
   {
      return;
   }
   const auto log_new_size = getLogTextUtf8Len(m_unicodeText, new_size);

   m_logText.resize(log_new_size + 1); // one extra for \0
   m_logText[log_new_size] = '\0';


   m_unicodeText.resize(new_size);
}


void UncText::clear()
{
   m_logText.clear();
   m_logText.push_back('\0');


   m_unicodeText.clear();
}


void UncText::insert(size_t idx, int ch)
{
   if (idx >= m_unicodeText.size())
   {
      throw out_of_range(string(__func__) + ":" + to_string(__LINE__)
                         + " - idx >= m_chars.size()");
   }
   log_type utf8converted;

   utf8converted.reserve(UTF8_BLOCKS);
   toLogTextUtf8(ch, utf8converted);

   const auto utf8_idx = getLogTextUtf8Len(m_unicodeText, idx);

   m_logText.pop_back(); // remove '\0'
   m_logText.insert(std::next(std::begin(m_logText), utf8_idx),
                    std::begin(utf8converted), std::end(utf8converted));
   m_logText.push_back('\0');


   m_unicodeText.insert(std::next(std::begin(m_unicodeText), idx), ch);
}


void UncText::insert(size_t idx, const UncText &ref)
{
   if (ref.size() == 0)
   {
      return;
   }

   if (idx >= m_unicodeText.size())
   {
      throw out_of_range(string(__func__) + ":" + to_string(__LINE__)
                         + " - idx >= m_chars.size()");
   }
   const auto utf8_idx = getLogTextUtf8Len(m_unicodeText, idx);

   // (A+B) remove \0 from both containers, add back a single at the end
   m_logText.pop_back(); // A
   m_logText.insert(std::next(std::begin(m_logText), utf8_idx),
                    std::begin(ref.m_logText),
                    std::prev(std::end(ref.m_logText))); // B
   m_logText.push_back('\0');


   m_unicodeText.insert(std::next(std::begin(m_unicodeText), idx),
                        std::begin(ref.m_unicodeText), std::end(ref.m_unicodeText));
}


void UncText::append(int ch)
{
   m_logText.pop_back();

   if (  ch < 0x80
      && ch != '\n'
      && ch != '\r')
   {
      m_logText.push_back(ch);
   }
   else
   {
      log_type utf8converted;
      utf8converted.reserve(UTF8_BLOCKS);
      toLogTextUtf8(ch, utf8converted);

      m_logText.insert(std::end(m_logText),
                       std::begin(utf8converted), std::end(utf8converted));
   }
   m_logText.push_back('\0');


   m_unicodeText.push_back(ch);
}


void UncText::append(const UncText &ref)
{
   if (ref.size() == 0)
   {
      return;
   }
   m_logText.pop_back();
   m_logText.insert(std::end(m_logText),
                    std::begin(ref.m_logText), std::end(ref.m_logText));

   m_unicodeText.insert(m_unicodeText.end(), ref.m_unicodeText.begin(), ref.m_unicodeText.end());
}


void UncText::append(const string &ascii_text)
{
   UncText tmp(ascii_text);

   append(tmp);
}


void UncText::append(const char *ascii_text)
{
   UncText tmp(ascii_text);

   append(tmp);
}


void UncText::append(const value_type &data, size_t idx, size_t len)
{
   UncText tmp(data, idx, len);

   append(tmp);
}


bool UncText::startswith(const char *text, size_t idx) const
{
   const auto orig_idx = idx;

   for ( ;
         (  idx < size()
         && *text);
         idx++, text++)
   {
      if (*text != m_unicodeText[idx])
      {
         return(false);
      }
   }

   return(  idx != orig_idx
         && (*text == 0));
}


bool UncText::startswith(const UncText &text, size_t idx) const
{
   size_t     si       = 0;
   const auto orig_idx = idx;

   for ( ;
         (  idx < size()
         && si < text.size());
         idx++, si++)
   {
      if (text.m_unicodeText[si] != m_unicodeText[idx])
      {
         return(false);
      }
   }

   return(  idx != orig_idx
         && (si == text.size()));
}


int UncText::find(const char *search_txt, size_t start_idx) const
{
   const size_t t_len = strlen(search_txt); // the length of 'text' we are looking for
   const size_t s_len = size();             // the length of the string we are looking in

   if (  s_len < t_len                      // search_txt longer than the string we are looking in
      || start_idx + t_len - 1 >= s_len)    // starting position to high to find search_txt
   {
      return(-1);
   }
   const size_t end_idx = s_len - t_len;

   for (size_t idx = start_idx; idx <= end_idx; idx++)
   {
      bool match = true;

      for (size_t ii = 0; ii < t_len; ii++)
      {
         if (m_unicodeText[idx + ii] != search_txt[ii])
         {
            match = false;
            break;
         }
      }

      if (match) // 'text' found at position 'idx'
      {
         return(idx);
      }
   }

   return(-1);  //  'text' not found
}


int UncText::rfind(const char *search_txt, size_t start_idx) const
{
   const size_t t_len = strlen(search_txt); // the length of 'text' we are looking for
   const size_t s_len = size();             // the length of the string we are looking in

   if (  s_len < t_len                      // search_txt longer than the string we are looking in
      || start_idx < t_len - 1)             // starting position to low to find search_txt
   {
      return(-1);
   }
   const size_t end_idx = s_len - t_len;

   if (start_idx > end_idx)
   {
      start_idx = end_idx;
   }

   for (auto idx = static_cast<int>(start_idx); idx >= 0; idx--)
   {
      bool match = true;

      for (size_t ii = 0; ii < t_len; ii++)
      {
         if (m_unicodeText[idx + ii] != search_txt[ii])
         {
            match = false;
            break;
         }
      }

      if (match)
      {
         return(idx);
      }
   }

   return(-1);
}


void UncText::erase(size_t start_idx, size_t len)
{
   if (len == 0)
   {
      return;
   }
   const size_t end_idx = start_idx + len - 1;

   if (end_idx >= m_unicodeText.size())
   {
      throw out_of_range(string(__func__) + ":" + to_string(__LINE__)
                         + " - idx + len >= m_chars.size()");
   }
   const auto pos_s = getLogTextUtf8Len(m_unicodeText, start_idx);
   const auto pos_e = pos_s + getLogTextUtf8Len(m_unicodeText, start_idx, end_idx);

   m_logText.pop_back();
   m_logText.erase(std::next(std::begin(m_logText), pos_s),
                   std::next(std::begin(m_logText), pos_e + 1));
   m_logText.push_back('\0');


   m_unicodeText.erase(std::next(std::begin(m_unicodeText), start_idx),
                       std::next(std::begin(m_unicodeText), end_idx + 1));
}


int UncText::replace(const char *search_text, const UncText &replace_text)
{
   const size_t s_len = strlen(search_text);
   const size_t r_len = replace_text.size();

   int          rcnt = 0;
   int          fidx = find(search_text);

   while (fidx >= 0)
   {
      rcnt++;
      erase(static_cast<size_t>(fidx), s_len);

      (static_cast<size_t>(fidx) >= m_unicodeText.size())
      ? append(replace_text)
      : insert(static_cast<size_t>(fidx), replace_text);

      fidx = find(search_text, static_cast<size_t>(fidx) + r_len);
   }
   return(rcnt);
}
