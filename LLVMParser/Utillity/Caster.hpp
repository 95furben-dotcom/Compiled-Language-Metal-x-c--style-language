#pragma once
#include <type_traits>
#include "../../ASTParser/AST.hpp"
template<typename B, typename A>
static const B* Cast(const A* node){
    static_cast<std::is_base_of<Node, A>>("A must be node");
    static_cast<std::is_base_of<B, A>>("A bust be base of b");

    return dynamic_cast<const B*>(node);
}

template<typename B, typename A>
static const B* TryCast(const A* node){
    static_cast<std::is_base_of<Node, A>>("A must be node");
    static_cast<std::is_base_of<Node, B>>("B must be node");
    
    if(auto t = dynamic_cast<const B*>(node)){
        return t; 
    }
    return nullptr;
}