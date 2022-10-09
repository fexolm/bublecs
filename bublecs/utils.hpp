#pragma once

#include <exception>
#include <iostream>

static inline void catcher() noexcept {
    if (std::current_exception()) {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& error) {
            std::cerr << "ERROR: " << error.what() << '\n';
        } catch (...) {

            std::cerr << "Non-exception object thrown" << '\n';
        }
        std::exit(1);
    }
    std::abort();
}