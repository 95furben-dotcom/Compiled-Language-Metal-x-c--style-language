#pragma once
#include <stdexcept>
#include <string>
#include <fstream>

inline void WriteToFile(const std::string& contents, const std::string& path){
    std::ofstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Could not open file at " + path);
    file << contents;
    file.close();
}

inline std::string ReadFile(const std::string& path){
    std::ifstream file(path);
    if(!file.is_open()) throw std::runtime_error("Could not open file: at "+path);
    std::string fileconts;
    
    std::string line;
    while (getline(file, line)) {
        fileconts.push_back('\n');
        for(const char& c : line){
            fileconts.push_back(c);
        }
    }
    return fileconts;
}