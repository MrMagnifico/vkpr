#include "resource_management.h"

void vkUtils::DeletionQueue::pushFunction(std::function<void()>&& function) {deletors.push_back(function);}

void vkUtils::DeletionQueue::flush() {
    // Reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {(*it)();}
    deletors.clear();
}
