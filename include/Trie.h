/* This component is a implement for trie for data manager */

#ifndef TRIE_H
#define TRIE_H

#include <vector>
#include <string.h>
#include <unordered_map>

template <typename T, bool lowercase>
class TrieNode
{
public:
    char c_;
    std::unordered_map<char, TrieNode*> char_map_;
    std::vector<T> value_list_;

    TrieNode() : c_(0) {}
    TrieNode(char c): c_(c) {}

    ~TrieNode(){
        for(auto &pair : char_map_){
            delete pair.second;
        }
    }

    void Insert(const char* word, T value, int &size){
        // printf("Insert at %c, value %d\n", c_, value);
        if(*word == '\0'){
            // printf("end insert at %c\n", c_);
            value_list_.push_back(value);
        }else{
            char tmp_char = *word;
            if(lowercase){
                if('A' <= tmp_char && tmp_char <= 'Z'){
                    tmp_char += 'a' - 'A';
                }
            }

            TrieNode* trie_node;
            auto iter = char_map_.find(tmp_char);
            if(iter == char_map_.end()){
                trie_node = new TrieNode(tmp_char);
                size++;
                char_map_.insert(std::make_pair(tmp_char, trie_node));
            }else{
                trie_node = iter->second;
            }
            trie_node->Insert(word+1, value, size);
        }
    }

    TrieNode* Search(const char* word){
        // printf("Search at %c\n", c_);
        if(*word == '\0'){
            return this;
        }else{
            auto iter = char_map_.find(*word);
            if(iter == char_map_.end()){
                return nullptr;
            }else{
                return iter->second->Search(word + 1);
            }
        }

        return nullptr;        
    }
};


template <typename T, bool lowercase>
class Trie
{
public:
    TrieNode<T, lowercase> *root_ = nullptr;
    int size_ = 1;
    bool valid_ = false;

    bool store_in_array = false;
    bool read_only = false;

    bool SetReadOnly() {read_only = true;}
    
    bool Valid() {return valid_;}

    void Set_valid(){
        if(root_ == nullptr){
            root_ = new TrieNode<T, lowercase>('@');
        }
        valid_ = true;
    }

    void Set_unvalid(){
        valid_ = false;
    }

    Trie(){
        Set_valid();
    }

    void Insert(const char* word, T value){
        // printf("word == 0, %d\n", *word == '\0');
        if(Valid() && !read_only){
            root_->Insert(word, value, size_);
        }
    }

    std::vector<T>* Search(const char* word){
        if(!Valid()){
            return nullptr;
        }
        TrieNode<T, lowercase>* node = root_->Search(word);
        if(node == nullptr){
            return nullptr;
        }
        return &node->value_list_;
    }

    void Delete_tree()
    {
        Set_unvalid();
        if(!store_in_array){
            delete root_;
        }else{
            free(root_);
        }
        root_ = nullptr;
        store_in_array = false;
        size_ = 1;
    }


};


#endif