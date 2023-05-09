#pragma once
#include <list>
#include <mutex>

namespace base{

	// ���ܱ�����߳�ͬʱʹ�ã������ܱ����߳�ʹ�õĶ���
	// Ҫ����Ժ�nullptr������һ��Ϊָ��������mgr�ڶ��߳���ʹ��
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

		//��ȡһ������
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
		//�ͷ�һ������
		void release(T ptr) {
			std::lock_guard<std::mutex> lg(m_lock);
			for (auto &i : m_lstObjs) {				
				if (i.ptr==ptr) {
					i.used = false;
					break;
				}
			}
		}
		//����һ��ʧЧ����[�����ⲿ�ͷ��ڴ�]
		void clean(const T &ptr) {
			std::lock_guard<std::mutex> lg(m_lock);
			for (auto i = m_lstObjs.begin(); i != m_lstObjs.end(); ++i) {
				if (i->ptr == ptr) {
					m_lstObjs.erase(i);
					break;
				}
			}
		}
		//�������ж���[δ�ͷ�]
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