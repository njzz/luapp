#pragma once

#include <mutex>

namespace base{

//locked type.use type with lock
template <typename T,typename L = std::recursive_mutex>
class lock_t : public T {
 public:
	 void lock() const {
		 m_lock.lock();
	 }
	 void unlock() const {
		 m_lock.unlock();
	 }

 private:
  mutable L m_lock;
};

}
