#pragma once
#include <string>
#include <unordered_map>



namespace Oprands{
    static std::unordered_map<std::string, std::string> opToAssembly = {{"+","add"},{"-","sub"},{"*","mul"},{"/","sdiv"}};
}