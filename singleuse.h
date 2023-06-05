#pragma once
#include <list>
#include <mutex>

namespace base{

	// ���ܱ�����߳�ͬʱʹ�ã������ܱ����߳�ʹ�õĶ���
	//T ��������  V �汾����   ֻ��һ���汾���ڣ�������ڶ���汾
	template<typename T,typename V=size_t>
	class single_use {
	public:
		using creator = std::function<T(const V&)>;
		using clearor = std::function<void(T)>;
		using my_list = std::list<T>;
		using my_lock = std::recursive_mutex;
		using my_lock_gd = std::lock_guard < std::recursive_mutex >;

		single_use(const V &vInit):m_version(vInit){}
		//��ȡһ�����󣬲���Ϊ��Ҫ�İ汾
		//����汾����ȣ�����Ϊ�°汾 v ����������δʹ�ö�����ʹ�ö�����releaseʱ����
		//�������cf��
		T get(const creator &cf,const clearor &cl,const V& v) {
			T rv;
			bool bUnGeted = true;
			m_lock.lock();			
			if(!m_lstObjs.empty()){//����
				if (v != m_version) {//��ͬ�汾���ͷ�����
					clear_all(cl,v);//���������°汾�������ٴα�����
				}
				else {//��ͬ�汾����ȡ
					rv = std::move(m_lstObjs.front());
					m_lstObjs.pop_front();//����
					bUnGeted = false;
				}
			}
			else if(m_version != v){//�������Ұ汾��һ��ֱ�Ӹ��°汾
				m_version = v;
			}

			m_lock.unlock();//����

			if (bUnGeted &&  cf ) {//δ��ȡ�ҿ��Թ���
				rv=cf(v);				
			}

			return rv;
		}
		//�ͷ�һ����������汾�͵�ǰ�汾��ƥ�䣬����false���ⲿ��Ҫ�������
		bool release(T& value,const V &ver) {
			my_lock_gd lg(m_lock);
			bool matched = (ver == m_version);
			if (matched) {//ͬ�汾������
				m_lstObjs.emplace_back(value);
			}
			return matched;
		}
		//����һ��ʧЧ����[�����ⲿ����]
		void clear(const T &value) {
			my_lock_gd lg(m_lock);
			for (auto i = m_lstObjs.begin(); i != m_lstObjs.end(); ++i) {
				if (*i == value) {
					m_lstObjs.erase(i);
					break;
				}
			}
		}
		//�������ж��󣬲��趨һ���°汾���°汾���Ժ��ϰ汾һ��
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

		//��ȡ��ǰ�汾
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