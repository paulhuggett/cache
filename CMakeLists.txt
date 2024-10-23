cmake_minimum_required(VERSION 3.28)
project (hasht)

add_executable(hasht main.cpp cache.hpp)
target_compile_features(hasht PUBLIC cxx_std_20)


#add_executable(lru lru.cpp)
#target_compile_features(lru PUBLIC cxx_std_20)
