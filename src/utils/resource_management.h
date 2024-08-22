#pragma once

#include <deque>
#include <functional>


namespace vkUtils {
struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void pushFunction(std::function<void()>&& function);
	void flush();
};
}
