/* This component is for text to url encode and decode */

#include "UrlEncode.h"
#include <cstring>
#include <string>

static unsigned char hexchars[] = "0123456789ABCDEF";

char *url_encode(const char *s)
{
    int len = strlen(s);

    register int x, y;

    char* str = new char[3 * len + 1];
    memset(str, 3 * len + 1, 0);
    for(x = 0, y = 0; len--; x++, y++)
    {
        str[y] = (unsigned char) s[x];
        if ((str[y] < '0' && str[y] != '-' && str[y] != '.')
                || (str[y] < 'A' && str[y] > '9')
                || (str[y] > 'Z' && str[y] < 'a' && str[y] != '_')
                || (str[y] > 'z'))
        {
            str[y++] = '%';
            str[y++] = hexchars[(unsigned char) s[x] >> 4];
            str[y] = hexchars[(unsigned char) s[x] & 15];
        }
    }
    str[y] = '\0';

    return ((char *) str);
}

void url_encode(std::string &s)
{
    char *buf = url_encode(s.c_str() );
    if (buf)
    {
        s = buf;
        delete[] buf;
    }
}

// string to int
static inline int htoi(const char *s)
{
    int value;
    int c;

    c = ((unsigned char *)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char *)s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}


char* url_decode(const char *str)
{
    int len = strlen(str);

    char* new_str = new char[len+1];
    memset(new_str, len+1, 0);
    char *dest = new_str;
    const char *data = str;

    while (len--) {
        if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) {
            *dest = (char) htoi(data + 1);
            data += 2;
            len -= 2;
        }
        else{
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    return new_str;
}


void url_decode(std::string &str)
{
    char * new_str = url_decode(str.c_str());
    str = new_str;
    delete[] new_str;
}