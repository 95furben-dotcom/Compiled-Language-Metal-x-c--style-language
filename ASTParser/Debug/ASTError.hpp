#pragma once
#include "../AST.hpp"
#include "ParseError.hpp"
#include <expected>
#include <iostream>
#include <source_location>
#include <string>
#include <type_traits>
#include <vector>


#define ADVANCE()\
    do{\
    index++;\
    if(index > tokens.size())\
        return Fail("Index bigger than tokensize", index-1); \
    std::cout << "Indexing at " << std::string(__func__) + "():" + std::to_string(index-1)+"\t\t" + __FILE__ + ":" + std::to_string(__LINE__) << "\n"; \
    } while(0)

template<typename T>
T* Ensure(Result<T*> node){
    static_assert(std::is_base_of<Node, T>(),"T must be a node");

    if(node.has_value())
        return node.value();

    T* errorNode = new T();
    errorNode->error = new ParseError(node.error());


    return errorNode;
}
