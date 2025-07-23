// src/syscall/io_manager.cpp
#include "io_manager.hpp"
#include <iostream>

bool IOManager::registerDevice(uint8_t fd, std::shared_ptr<IODevice> device) {
    if (!device) {
        std::cerr << "[IOManager] Cannot register null device for fd=" << static_cast<int>(fd) << std::endl;
        return false;
    }

    devices_[fd] = device;
    std::cout << "[IOManager] Registered device '" << device->get_device_type() 
              << "' for fd=" << static_cast<int>(fd) << std::endl;
    return true;
}

bool IOManager::unregisterDevice(uint8_t fd) {
    auto it = devices_.find(fd);  // 🔧 수정: devices_find → devices_.find
    if (it == devices_.end()) {
        std::cerr << "[IOManager] No device found for fd=" << static_cast<int>(fd) << std::endl;  // 🔧 수정: std:cerr >> → std::cerr <<
        return false;
    }

    std::cout << "[IOManager] Unregistered device for fd=" << static_cast<int>(fd) << std::endl;
    devices_.erase(it);
    return true;
}

size_t IOManager::read(uint8_t fd, char* buffer, size_t size) {
    auto it = devices_.find(fd);
    if (it == devices_.end()) {
        std::cerr << "[IOManager] No device found for fd=" << static_cast<int>(fd) << " (READ)" << std::endl;
        return 0;
    }

    if (!it->second->is_readable()) {
        std::cerr << "[IOManager] Device fd=" << static_cast<int>(fd) << " is not readable" << std::endl;
        return 0;
    }

    size_t bytes_read = it->second->read(buffer, size);
    std::cout << "[IOManager] Read " << bytes_read << " bytes from fd=" << static_cast<int>(fd) << std::endl;
    return bytes_read;
}

size_t IOManager::write(uint8_t fd, const char* buffer, size_t size) {
    auto it = devices_.find(fd);
    if (it == devices_.end()) {
        std::cerr << "[IOManager] No device found for fd=" << static_cast<int>(fd) << " (WRITE)" << std::endl;
        return 0;
    }

    if (!it->second->is_writable()) {
        std::cerr << "[IOManager] Device fd=" << static_cast<int>(fd) << " is not writable" << std::endl;
        return 0;
    }

    size_t bytes_written = it->second->write(buffer, size);
    std::cout << "[IOManager] Wrote " << bytes_written << " bytes to fd=" << static_cast<int>(fd) << std::endl;
    return bytes_written;
}

bool IOManager::hasDevice(uint8_t fd) const {
    return devices_.find(fd) != devices_.end();
}

void IOManager::printDevices() const {
    std::cout << "[IOManager] Registered devices:" << std::endl;
    for (const auto& pair : devices_) {
        std::cout << "  fd=" << static_cast<int>(pair.first) 
                  << " -> " << pair.second->get_device_type() << std::endl;
    }
}

void IOManager::clear() {
    std::cout << "[IOManager] Clearing all devices (" << devices_.size() << " total)" << std::endl;
    devices_.clear();
}