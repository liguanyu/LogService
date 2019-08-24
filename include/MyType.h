#ifndef MYTYPE_H
#define MYTYPE_H

#include "Trie.h"

typedef Trie<int, false> KeyTrie;
typedef Trie<int, true> KeyWordTrie;
typedef std::vector<int> OffsetList;
typedef std::vector<uint64_t> TimeStampList;


#endif