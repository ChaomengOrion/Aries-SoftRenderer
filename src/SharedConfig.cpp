/// FileName: SharedConfig.cpp
/// Date: 2025/05/11
/// Author: ChaomengOrion

#include "SharedConfig.hpp"

std::shared_ptr<SharedConfig> SharedConfigManager::s_instance = nullptr;

void SharedConfigManager::BindConfig(std::shared_ptr<SharedConfig> config) {
    s_instance = config;
}

std::shared_ptr<const SharedConfig> SharedConfigManager::GetConfig() {
    return s_instance;
}