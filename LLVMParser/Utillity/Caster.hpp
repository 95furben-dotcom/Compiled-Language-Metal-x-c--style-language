#pragma once
#include <type_traits>
#include "../../ASTParser/AST.hpp"
template<typename B, typename A>
static const B* Cast(const A* node){
    static_assert(std::is_base_of_v<Node, A>,"A must be node");
    static_assert(std::is_base_of_v<B, A> || std::is_base_of_v<A, B>,"A bust be base of b or the other way");

    return dynamic_cast<const B*>(node);
}

template<typename B, typename A>
static const B* TryCast(const A* node){
    static_assert(std::is_base_of_v<Node, A>, "A must be derived from Node");
    static_assert(std::is_base_of_v<A, B>, "A must be base of B");
    
    if(auto t = dynamic_cast<const B*>(node)){
        return t; 
    }
    return nullptr;
}