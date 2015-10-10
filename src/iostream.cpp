//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#define NOWIDE_SOURCE
#include <nowide/iostream.hpp>
#include <nowide/convert.hpp>
#include <stdio.h>
#include <vector>

#ifdef NOWIDE_WINDOWS
#include <util.h>

#ifndef NOMINMAX
# define NOMINMAX
#endif

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <windows.h>


namespace nowide {
namespace details {
    class console_output_buffer : public std::streambuf {
    public:
        console_output_buffer(HANDLE h) :
            handle_(h),
            isatty_(false),
            consuming_escape_seq_(false)
        {
            if(handle_) {
                DWORD dummy;
                isatty_ = GetConsoleMode(handle_,&dummy) == TRUE;
            }


        }
    protected:
        int sync()
        {
            return overflow(EOF);
        }
        int overflow(int c)
        {
            if(!handle_)
                return -1;

            int n = pptr() - pbase();
            int r = 0;
            
            if(n > 0 && (r=write(pbase(),n)) < 0)
                    return -1;
            if(r < n) {
                memmove(pbase(),pbase() + r,n-r);
            }
            setp(buffer_, buffer_ + buffer_size);
            pbump(n-r);
            if(c!=EOF)
                sputc(c);
            return 0;
        }
    private:
        const DWORD FOREGROUND_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN;
        const DWORD FOREGROUND_MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE;
        const DWORD FOREGROUND_CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE;
        const DWORD FOREGROUND_WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        const DWORD FOREGROUND_BLACK = 0;
        const DWORD FOREGROUND_RESET = ~FOREGROUND_WHITE;
        const DWORD BACKGROUND_YELLOW = BACKGROUND_RED | BACKGROUND_GREEN;
        const DWORD BACKGROUND_MAGENTA = BACKGROUND_RED | BACKGROUND_BLUE;
        const DWORD BACKGROUND_CYAN = BACKGROUND_GREEN | BACKGROUND_BLUE;
        const DWORD BACKGROUND_WHITE = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
        const DWORD BACKGROUND_BLACK = 0;
        const DWORD BACKGROUND_RESET = ~(BACKGROUND_WHITE | BACKGROUND_INTENSITY);
        const DWORD CON_NORMAL = FOREGROUND_WHITE;
        const DWORD CON_ALL = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
                              BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE |
                              FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
        const utf::code_point ESC = 0x1b;

        bool hasFlag(WORD attribs, WORD flag) {
            return (attribs & flag) == flag;
        }

        WORD inverse(WORD attrib) {
            switch (attrib) {
                case FOREGROUND_RED:
                    return BACKGROUND_RED;
                case FOREGROUND_GREEN:
                    return BACKGROUND_GREEN;
                case FOREGROUND_BLUE:
                    return BACKGROUND_BLUE;
                case FOREGROUND_INTENSITY:
                    return BACKGROUND_INTENSITY;
                case BACKGROUND_RED:
                    return FOREGROUND_RED;
                case BACKGROUND_GREEN:
                    return FOREGROUND_GREEN;
                case BACKGROUND_BLUE:
                    return FOREGROUND_BLUE;
                case BACKGROUND_INTENSITY:
                    return FOREGROUND_INTENSITY;
                default:
                    return 0;
            }
        }

        WORD invertFlag(WORD attribs, WORD attrib) {
            if (hasFlag(attribs, attrib)) {
                return inverse(attrib);
            }

            return 0;
        }

        WORD invertFlags(WORD attribs) {
            WORD result = 0;
            result |= invertFlag(attribs, FOREGROUND_RED);
            result |= invertFlag(attribs, FOREGROUND_GREEN);
            result |= invertFlag(attribs, FOREGROUND_BLUE);
            result |= invertFlag(attribs, FOREGROUND_INTENSITY);
            result |= invertFlag(attribs, BACKGROUND_RED);
            result |= invertFlag(attribs, BACKGROUND_GREEN);
            result |= invertFlag(attribs, BACKGROUND_BLUE);
            result |= invertFlag(attribs, BACKGROUND_INTENSITY);

            return result;
        }

        enum ColorCodes {
            Reset = 0,
            Intense = 1,
            Inverse = 7,
            NoIntense = 21,
            FgBlack = 30,
            FgRed = 31,
            FgGreen = 32,
            FgYellow = 33,
            FgBlue = 34,
            FgMagenta = 35,
            FgCyan = 36,
            FgWhite = 37,
            FgDefault = 39,
            BgBlack = 40,
            BgRed = 41,
            BgGreen = 42,
            BgYellow = 43,
            BgBlue = 44,
            BgMagenta = 45,
            BgCyan = 46,
            BgWhite = 47,
            BgDefault = 49,
            BgBlackIntense = 100,
            BgRedIntense = 101,
            BgGreenIntense = 102,
            BgYellowIntense = 103,
            BgBlueIntense = 104,
            BgMagentaIntense = 105,
            BgCyanIntense = 106,
            BgWhiteIntense = 107,
        };

        void fgSet(WORD *attribs, WORD attrib) {
            *attribs &= FOREGROUND_RESET;
            *attribs |= attrib;
        }

        void bgSet(WORD *attribs, WORD attrib) {
            *attribs &= BACKGROUND_RESET;
            *attribs |= attrib;
        }

        void processEscapeCode(const std::string& modifiers, char command) {
            // m is for color commands
            if (command == 'm') {
                std::vector<std::string> parts;
                if (modifiers.empty()) {
                    parts.push_back("0");
                }
                else {
                    boost::split(parts, modifiers, boost::is_any_of(";"));
                }

                CONSOLE_SCREEN_BUFFER_INFO bufInfo = {0};
                GetConsoleScreenBufferInfo(handle_, &bufInfo);

                WORD attribs = bufInfo.wAttributes;

                for (auto& part : parts) {
                    int val = 0;
                    try {
                        val = boost::lexical_cast<int>(part);
                    }
                    catch (boost::bad_lexical_cast&) {
                        continue;
                    }

                    switch (val) {
                        case ColorCodes::Reset:
                            attribs = CON_NORMAL;
                            break;
                        case ColorCodes::Intense:
                            attribs |= FOREGROUND_INTENSITY;
                            break;
                        case ColorCodes::Inverse: {
                            WORD inverted = invertFlags(attribs);
                            attribs &= ~CON_ALL;
                            attribs |= inverted;
                            break;
                        }
                        case ColorCodes::NoIntense:
                            attribs &= ~FOREGROUND_INTENSITY;
                            break;
                        case ColorCodes::FgBlack:
                            fgSet(&attribs, FOREGROUND_BLACK);
                            break;
                        case ColorCodes::FgRed:
                            fgSet(&attribs, FOREGROUND_RED);
                            break;
                        case ColorCodes::FgGreen:
                            fgSet(&attribs, FOREGROUND_GREEN);
                            break;
                        case ColorCodes::FgYellow:
                            fgSet(&attribs, FOREGROUND_YELLOW);
                            break;
                        case ColorCodes::FgBlue:
                            fgSet(&attribs, FOREGROUND_BLUE);
                            break;
                        case ColorCodes::FgMagenta:
                            fgSet(&attribs, FOREGROUND_MAGENTA);
                            break;
                        case ColorCodes::FgCyan:
                            fgSet(&attribs, FOREGROUND_CYAN);
                            break;
                        case ColorCodes::FgWhite:
                        case ColorCodes::FgDefault:
                            fgSet(&attribs, FOREGROUND_WHITE);
                            break;
                        case ColorCodes::BgBlack:
                        case ColorCodes::BgDefault:
                            bgSet(&attribs, BACKGROUND_BLACK);
                            break;
                        case ColorCodes::BgRed:
                            bgSet(&attribs, BACKGROUND_RED);
                            break;
                        case ColorCodes::BgGreen:
                            bgSet(&attribs, BACKGROUND_GREEN);
                            break;
                        case ColorCodes::BgYellow:
                            bgSet(&attribs, BACKGROUND_YELLOW);
                            break;
                        case ColorCodes::BgBlue:
                            bgSet(&attribs, BACKGROUND_BLUE);
                            break;
                        case ColorCodes::BgMagenta:
                            bgSet(&attribs, BACKGROUND_MAGENTA);
                            break;
                        case ColorCodes::BgCyan:
                            bgSet(&attribs, BACKGROUND_CYAN);
                            break;
                        case ColorCodes::BgWhite:
                            bgSet(&attribs, BACKGROUND_WHITE);
                            break;
                        case ColorCodes::BgBlackIntense:
                            bgSet(&attribs, BACKGROUND_BLACK | BACKGROUND_INTENSITY);
                            break;
                        case ColorCodes::BgRedIntense:
                            bgSet(&attribs, BACKGROUND_RED | BACKGROUND_INTENSITY);
                            break;
                        case ColorCodes::BgGreenIntense:
                            bgSet(&attribs, BACKGROUND_GREEN | BACKGROUND_INTENSITY);
                            break;
                        case ColorCodes::BgYellowIntense:
                            bgSet(&attribs, BACKGROUND_YELLOW | BACKGROUND_INTENSITY);
                            break;
                        case ColorCodes::BgBlueIntense:
                            bgSet(&attribs, BACKGROUND_BLUE | BACKGROUND_INTENSITY);
                            break;
                        case ColorCodes::BgMagentaIntense:
                            bgSet(&attribs, BACKGROUND_MAGENTA | BACKGROUND_INTENSITY);
                            break;
                        case ColorCodes::BgCyanIntense:
                            bgSet(&attribs, BACKGROUND_CYAN | BACKGROUND_INTENSITY);
                            break;
                        case ColorCodes::BgWhiteIntense:
                            bgSet(&attribs, BACKGROUND_WHITE | BACKGROUND_INTENSITY);
                            break;
                    }
                }

                SetConsoleTextAttribute(handle_, attribs);
            }
        }

        int write(char const *p,int n)
        {
            namespace uf = nowide::utf;
            char const *b = p;
            char const *e = p+n;
            if(!isatty_) {
                DWORD size=0;
                if(!WriteFile(handle_,p,n,&size,0) || static_cast<int>(size) != n)
                    return -1;
                return n;
            }
            if(n > buffer_size)
                return -1;
            wchar_t *out = wbuffer_;
            uf::code_point c;
            size_t decoded = 0;
            while(p < e && (c = uf::utf_traits<char>::decode(p,e))!=uf::illegal && c!=uf::incomplete) {
                // If our console doesn't support ANSI escape codes, process them into
                // Windows console API calls
                if (!supports_ansi_codes()) {
                    if (!consuming_escape_seq_ && c == ESC) {
                        // Flush the buffer if it's not empty before processing the next escape code
                        // otherwise we will change formatting before formatting the current text
                        if (out != wbuffer_) {
                            if(!WriteConsoleW(handle_,wbuffer_,out - wbuffer_,0,0)) {
                                return -1;
                            }
                            out = wbuffer_;
                        }
                        consuming_escape_seq_ = true;
                        modifiers_.clear();
                        continue;
                    }
                    else if (consuming_escape_seq_) {
                        // For the sake of simplicity, I'm doing this after the code point has
                        // been processed, but a valid escape sequence is going to be entirely ASCII
                        if (c > 0x7F) return -1;

                        char ch = static_cast<char>(c);

                        // Should really validate that this comes directly after ESC, but whatever...
                        if (ch == '[') continue;

                        if (ch > 0x40 && ch < 0x7E) {
                            processEscapeCode(modifiers_, ch);
                            consuming_escape_seq_ = false;
                        }
                        else {
                            modifiers_.push_back(ch);
                        }

                        continue;
                    }
                }

                out = uf::utf_traits<wchar_t>::encode(c,out);
                decoded = p-b;
            }
            if(c==uf::illegal)
                return -1;
            if(!WriteConsoleW(handle_,wbuffer_,out - wbuffer_,0,0))
                return -1;
            return decoded;
        }
        
        static const int buffer_size = 1024;
        char buffer_[buffer_size];
        wchar_t wbuffer_[buffer_size]; // for null
        HANDLE handle_;
        bool isatty_;
        bool consuming_escape_seq_;
        std::string modifiers_;
    };
    
    class console_input_buffer: public std::streambuf {
    public:
        console_input_buffer(HANDLE h) :
            handle_(h),
            isatty_(false),
            wsize_(0)
        {
            if(handle_) {
                DWORD dummy;
                isatty_ = GetConsoleMode(handle_,&dummy) == TRUE;
            }
        } 
        
    protected:
        int pbackfail(int c)
        {
            if(c==EOF)
                return EOF;
            
            if(gptr()!=eback()) {
                gbump(-1);
                *gptr() = c;
                return 0;
            }
            
            if(pback_buffer_.empty()) {
                pback_buffer_.resize(4);
                char *b = &pback_buffer_[0];
                char *e = b + pback_buffer_.size();
                setg(b,e-1,e);
                *gptr() = c;
            }
            else {
                size_t n = pback_buffer_.size();
                std::vector<char> tmp;
                tmp.resize(n*2);
                memcpy(&tmp[n],&pback_buffer_[0],n);
                tmp.swap(pback_buffer_);
                char *b = &pback_buffer_[0];
                char *e = b + n * 2;
                char *p = b+n-1;
                *p = c;
                setg(b,p,e);
            }
          
            return 0;
        }

        int underflow()
        {
            if(!handle_)
                return -1;
            if(!pback_buffer_.empty())
                pback_buffer_.clear();
            
            size_t n = read();
            setg(buffer_,buffer_,buffer_+n);
            if(n == 0)
                return EOF;
            return std::char_traits<char>::to_int_type(*gptr());
        }
        
    private:
        
        size_t read()
        {
            namespace uf = nowide::utf;
            if(!isatty_) {
                DWORD read_bytes = 0;
                if(!ReadFile(handle_,buffer_,buffer_size,&read_bytes,0))
                    return 0;
                return read_bytes;
            }
            DWORD read_wchars = 0;
            size_t n = wbuffer_size - wsize_;
            if(!ReadConsoleW(handle_,wbuffer_,n,&read_wchars,0))
                return 0;
            wsize_ += read_wchars;
            char *out = buffer_;
            wchar_t *b = wbuffer_;
            wchar_t *e = b + wsize_;
            wchar_t *p = b;
            uf::code_point c;
            wsize_ = e-p;
            while(p < e && (c = uf::utf_traits<wchar_t>::decode(p,e))!=uf::illegal && c!=uf::incomplete) {
                // Extremely naive line ending conversion
                if (c == '\r') {
                    continue;
                }

                out = uf::utf_traits<char>::encode(c,out);
                wsize_ = e-p;
            }
            
            if(c==uf::illegal)
                return -1;
            
            
            if(c==uf::incomplete) {
                memmove(b,e-wsize_,sizeof(wchar_t)*wsize_);
            }
            
            return out - buffer_;
        }
        
        static const size_t buffer_size = 1024 * 3;
        static const size_t wbuffer_size = 1024;
        char buffer_[buffer_size];
        wchar_t wbuffer_[buffer_size]; // for null
        HANDLE handle_;
        bool isatty_;
        int wsize_;
        std::vector<char> pback_buffer_;
    };

    winconsole_ostream::winconsole_ostream(int fd) : std::ostream(0)
    {
        HANDLE h = 0;
        switch(fd) {
        case 1:
            h = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case 2:
            h = GetStdHandle(STD_ERROR_HANDLE);
            break;
        }
        d.reset(new console_output_buffer(h));
        std::ostream::rdbuf(d.get());
    }
    
    winconsole_ostream::~winconsole_ostream()
    {
    }

    winconsole_istream::winconsole_istream() : std::istream(0)
    {
        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
        d.reset(new console_input_buffer(h));
        std::istream::rdbuf(d.get());
    }
    
    winconsole_istream::~winconsole_istream()
    {
    }
    
} // details
    
NOWIDE_DECL details::winconsole_istream cin;
NOWIDE_DECL details::winconsole_ostream cout(1);
NOWIDE_DECL details::winconsole_ostream cerr(2);
NOWIDE_DECL details::winconsole_ostream clog(2);
    
namespace {
    struct initialize {
        initialize()
        {
            nowide::cin.tie(&nowide::cout);
            nowide::cerr.tie(&nowide::cout);
            nowide::cerr.setf(std::ios_base::unitbuf);
            nowide::clog.tie(&nowide::cout);
        }
    } inst;
}


    
} // nowide


#endif
///
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
