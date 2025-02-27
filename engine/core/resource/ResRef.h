#pragma once

#include "engine/core/memory/MemAllocDef.h"

namespace Echo
{
	class Refable
	{
	public:
		virtual ~Refable() {}

		// add ref count
		void addRefCount() { m_refCount++; }

		// release
		virtual void subRefCount()
		{
			m_refCount--;
			if (m_refCount <= 0)
			{
				ECHO_DELETE_T(this, Refable);
			}
		}

	protected:
		int	m_refCount = 0;
	};

	template<typename T>
	class ResRef
	{
	public:
		ResRef()
			: m_ptr(nullptr)
		{}

		ResRef(T* ptr)
		{
			if (ptr)
			{
				ptr->addRefCount();
				m_ptr = ptr;
			}
			else
			{
				m_ptr = nullptr;
			}
		}

		ResRef(const ResRef<T>& orig)
		{
			if (orig.m_ptr)
			{
				m_ptr = orig.m_ptr;
				m_ptr->addRefCount();
			}
			else
			{
				m_ptr = nullptr;
			}
		}

		~ResRef()
		{
			reset();
		}

		void reset()
		{
			if (m_ptr)
			{
				m_ptr->subRefCount();
				m_ptr = nullptr;
			}
		}

		void operator = (ResRef<T>& orig)
		{
			if (m_ptr)
			{
				m_ptr->subRefCount();
				m_ptr = nullptr;
			}

			if (orig.m_ptr)
			{
				m_ptr = orig.m_ptr;
                m_ptr->addRefCount();
			}
		}
        
        void operator = (const ResRef<T>& orig)
        {
            if (m_ptr)
            {
                m_ptr->subRefCount();
                m_ptr = nullptr;
            }
            
            if (orig.m_ptr)
            {
                m_ptr = orig.m_ptr;
                m_ptr->addRefCount();
            }
        }

		void operator = (T* orig)
		{
			if (m_ptr != orig)
			{
				if (m_ptr)
				{
					m_ptr->subRefCount();
					m_ptr = nullptr;
				}

				if (orig)
				{
					orig->addRefCount();
					m_ptr = orig;
				}
			}
		}

		T* operator -> ()
		{
			return m_ptr;
		}

		const T* operator -> () const
		{
			return m_ptr;
		}

		T* ptr()
		{
			return m_ptr;
		}

		operator T*() const
		{
			return m_ptr;
		}

	private:
		T*		m_ptr;
	};
}
