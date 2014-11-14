//
//  debug.cpp
//  SOM
//
//  Created by Stefan Marr on 14/11/14.
//
//

#include "debug.h"

#include <mutex>
#include <iostream>
#include <sstream>
#include <string>

static std::mutex output_mtx;

void sync_out(std::ostringstream msg) {
              //std::string msg) {
    std::lock_guard<std::mutex> lock(output_mtx);
    std::cout << msg.str() << std::endl;
}
