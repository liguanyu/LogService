/* This component is for text to url encode and decode */

#ifndef URLENCODE_H
#define URLENCODE_H

#include <string>

char *url_encode(const char *s);

void url_encode(std::string &s);

char* url_decode(const char *str);

void url_decode(std::string &str);


#endif
