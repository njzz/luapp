#pragma once
#include <list>
#include <mutex>

namespace base{

	// 不能被多个线程同时使用，但又能被多线程使用的对象
	//T 对象类型  V 版本类型   只有一个版本存在，不会存在多个版本
	template<typename T,typename V=size_t>
	class single_use {
	public:
		using creator = std::function<T(const V&)>;
		using clearor = std::function<void(T)>;
		using my_list = std::list<T>;
		using my_lock = std::recursive_mutex;
		using my_lock_gd = std::lock_guard < std::recursive_mutex >;

		single_use(const V &vInit):m_version(vInit){}
		//获取一个对象，参数为需要的版本
		//如果版本不相等，设置为新版本 v 并清理所有未使用对象，已使用对象在release时清理
		//如果设置cf，
		T get(const creator &cf,const clearor &cl,const V& v) {
			T rv;
			bool bUnGeted = true;
			m_lock.lock();			
			if(!m_lstObjs.empty()){//存在
				if (v != m_version) {//不同版本，释放所有
					clear_all(cl,v);//清理并设置新版本，可以再次被锁定
				}
				else {//相同版本，获取
					rv = std::move(m_lstObjs.front());
					m_lstObjs.pop_front();//弹出
					bUnGeted = false;
				}
			}
			else if(m_version != v){//不存在且版本不一样直接更新版本
				m_version = v;
			}

			m_lock.unlock();//解锁

			if (bUnGeted &&  cf ) {//未获取且可以构造
				rv=cf(v);				
			}

			return rv;
		}
		//释放一个对象，如果版本和当前版本不匹配，返回false，外部需要清理对象
		bool release(T& value,const V &ver) {
			my_lock_gd lg(m_lock);
			bool matched = (ver == m_version);
			if (matched) {//同版本，缓存
				m_lstObjs.emplace_back(value);
			}
			return matched;
		}
		//清理一个失效对象[已在外部清理]
		void clear(const T &value) {
			my_lock_gd lg(m_lock);
			for (auto i = m_lstObjs.begin(); i != m_lstObjs.end(); ++i) {
				if (*i == value) {
					m_lstObjs.erase(i);
					break;
				}
			}
		}
		//清理所有对象，并设定一个新版本，新版本可以和老版本一致
		void clear_all(const clearor &cl, const V &newV) {
			my_lock_gd lg(m_lock);
			if ( cl) {
				for (auto &i : m_lstObjs) {
					cl(i);
				}
			}
			m_lstObjs.clear();
			m_version = newV;
		}

		//获取当前版本
		const V &get_version() const{
			my_lock_gd lg(m_lock);
			return m_version;
		}
	protected:
		V m_version;
		mutable my_lock m_lock;
		my_list  m_lstObjs;
	};

}