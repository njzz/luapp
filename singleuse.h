#pragma once
#include <list>
#include <mutex>

namespace base{

	// 不能被多个线程同时使用，但又能被多线程使用的对象
	// 要求可以和nullptr互动，一般为指针对象，配合mgr在多线程中使用
	template<typename T>
	struct single_use_obj {
		single_use_obj(T t):ptr(t),used(true){}
		T ptr;
		bool used;
	};

	template<typename T>
	class single_use_mgr {
	public:
		using innerobj = single_use_obj<T>;
		using creator = std::function<T()>;
		using cleanor = std::function<void(T)>;
		using my_list = std::list<innerobj>;
		using my_lock = std::mutex;

		//获取一个对象
		T get(const creator &f) {
			T ptr = nullptr;
			{
				std::lock_guard<my_lock> lg(m_lock);
				for (auto &i : m_lstObjs) {
					if (!i.used) {
						ptr = i.ptr;
						i.used = true;
						break;
					}
				}
			}

			if (ptr == nullptr && f) {
				ptr = f();//new 
				if(ptr){
					std::lock_guard<my_lock> lg(m_lock);
					m_lstObjs.emplace_back(innerobj{ ptr });
				}
			}

			return ptr;
		}
		//释放一个对象
		void release(T ptr) {
			std::lock_guard<std::mutex> lg(m_lock);
			for (auto &i : m_lstObjs) {				
				if (i.ptr==ptr) {
					i.used = false;
					break;
				}
			}
		}
		//清理一个失效对象[已在外部释放内存]
		void clean(const T &ptr) {
			std::lock_guard<std::mutex> lg(m_lock);
			for (auto i = m_lstObjs.begin(); i != m_lstObjs.end(); ++i) {
				if (i->ptr == ptr) {
					m_lstObjs.erase(i);
					break;
				}
			}
		}
		//清理所有对象[未释放]
		void clean_all(const cleanor &f) {
			std::lock_guard<std::mutex> lg(m_lock);
			if (f) {
				for (auto &i : m_lstObjs) {
					f(i.ptr);
				}
			}
			m_lstObjs.clear();
		}
	protected:
		my_lock m_lock;
		my_list  m_lstObjs;
	};

}