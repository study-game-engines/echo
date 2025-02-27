#pragma once

#include "signal.h"
#include "variant.h"
#include "class_method_bind.h"
#include "property_info.h"
#include "engine/core/editor/object_editor.h"
#include "engine/core/util/StringUtil.h"
#include "engine/core/script/lua/lua_binder.h"

namespace Echo
{
	struct ClassInfo
	{
		bool			m_singleton = false;		// singleton class
		bool			m_virtual = false;			// virtual class can't be instanced
		String			m_parent;
		String			m_module;
		PropertyInfos	m_propertyInfos;
		ClassMethodMap	m_methods;
        ClassMethodMap  m_signals;
	};

	class object;
	struct ObjectFactory
	{
		String		m_name;
		ClassInfo	m_classInfo;

        // free
        void destroy();
        
        // create object
		virtual Object* create() = 0;

		// get default object
		virtual Object* getDefaultObject() = 0;

		// register property
		void registerProperty(PropertyInfo* property);

		// register method
		void registerMethod(const String& methodName, ClassMethodBind* method)
		{
			m_classInfo.m_methods[methodName] = method;
		}
        
        // register Signal
        void registerSignalGetMethod(const String& signalName, ClassMethodBind* getSignalMethod)
        {
            m_classInfo.m_signals[signalName] = getSignalMethod;
        }

		// return method bind
		ClassMethodBind* getMethodBind(const String& methodName)
		{
			auto it = m_classInfo.m_methods.find(methodName);
			if (it != m_classInfo.m_methods.end())
			{
				return it->second;
			}

			return nullptr;
		}
        
        // get signal
        ClassMethodBind* getSignalGetMethod( const String& signalName)
        {
            auto it = m_classInfo.m_signals.find(signalName);
            if (it != m_classInfo.m_signals.end())
            {
                return it->second;
            }
            
            return nullptr;
        }

		// get propertys
		const PropertyInfos& getPropertys()
		{
			return m_classInfo.m_propertyInfos;
		}

		// get property
		PropertyInfo* getProperty(const String& propertyName)
		{
			for (PropertyInfo* pi : m_classInfo.m_propertyInfos)
			{
				if (pi->m_name == propertyName)
				{
					return pi;
				}
			}

			return nullptr;
		}

        // get current module name
        const char* getCurrentRegisterModuleName();
	};

	class Class
	{
	public:
		template<typename T>
		static void registerType()
		{
			T::initClassInfo();
		}

		template<typename T>
		static T create(const String& className)
		{
			Object* obj = create(className);
			if (obj)
			{
				return ECHO_DOWN_CAST<T>(obj);
			}

			return nullptr;
		}

		// get all class names
		static size_t getAllClasses(StringArray& classes);

		// create
		static Object* create(const String& className);

		// get default object
		static Object* getDefaultObject(const String& className);

		// add class
		static void addClass(const String& className, ObjectFactory* objFactory);

		// get class info
		static ClassInfo* getClassInfo(const String& className);

		// is derived from
		static bool isDerivedFrom(const String& className, const String& parentClassName);

		// is virtual
		static bool isVirtual(const String& className);

		// is singleton
		static bool isSingleton(const String& className);

		// get parent class name
		static bool getParentClass(String& parentClassName, const String& className);

		// get all child class
		static bool getChildClasses(StringArray& childClasses, const String& className, bool recursive);

		// register method
		static bool registerMethodBind(const String& className, const String& methodName, ClassMethodBind* method);

		// get method
		static ClassMethodBind* getMethodBind(const String& className, const String& methodName);
        
        // register signal
        static bool registerSignal(const String& className, const String& signalName, ClassMethodBind* getSignalMethod);
        
        // get signal by class name
        static Signal* getSignal(const String& className, Object* classPtr, const String& signalName);
        
        // get signal
        static Signal* getSignal(Object* classPtr, const String& signalName);

		// add property
		static bool registerProperty(const String& className, const String& propertyName, const Variant::Type type, const String& getter, const String& setter);

		// get propertys
		static ui32 getPropertys(const String& className, Object* classPtr, PropertyInfos& propertys, i32 flag=PropertyInfo::Static | PropertyInfo::Dynamic, bool withParent=false);

		// get property
        static PropertyInfo* getProperty(Object* classPtr, const String& propertyName);
		static PropertyInfo* getProperty(const String& className, Object* classPtr, const String& propertyName);

		// get property value
		static bool getPropertyValue(Object* classPtr, const String& propertyName, Variant& oVar);
		static bool getPropertyValueDefault(Object* classPtr, const String& propertyName, Variant& oVar);

		// get property flag
		static i32 getPropertyFlag(Object* classPtr, const String& propertyName);

		// get property type
		static Variant::Type getPropertyType(Object* classPtr, const String& propertyName);

		// set property value
		static bool setPropertyValue(Object* classPtr, const String& propertyName, const Variant& propertyValue);

		// bind method
		template<typename N, typename M>
		static ClassMethodBind* bindMethod(const String& className, M method, N methodName)
		{
			ClassMethodBind* bind = createMethodBind(method);

			registerMethodBind(className, methodName, bind);

			return bind;
		}
        
        // register signal
        template<typename M>
        static ClassMethodBind* registerSignal(const String& className, const String& signalName, M getSignalMethod)
        {
            ClassMethodBind* bind = createMethodBind(getSignalMethod);
            
            registerSignal(className, signalName, bind);
            
            return bind;
        }

		template<typename ... T>
		static bool registerPropertyHint(const String& className, const String& propertyName, T... args)
		{
			PropertyInfo* pi = getProperty(className, nullptr, propertyName);
			if (pi)
			{
				pi->addHint(args...);
				return true;
			}

			return false;
		}
        
        // clear all classinfos
        static void clear();
	};
    
    template<typename T>
    struct ObjectFactoryT : public ObjectFactory
    {
        ObjectFactoryT(const String& name, const String& parent, bool isVirtual=false)
        {
            // register class to lua
			LuaBinder::instance()->registerClass( name.c_str(), parent.c_str());
            
            m_name = name;
            m_classInfo.m_singleton = false;
            m_classInfo.m_virtual = isVirtual;
			m_classInfo.m_module = getCurrentRegisterModuleName();
            m_classInfo.m_parent = parent;
            
            Class::addClass(name, this);
            T::bindMethods();
        }
        
        virtual Object* create()
        {
			Object* obj = EchoNew(T);

			return obj;
        }

		virtual Object* getDefaultObject()
		{
			static T obj;
			return &obj;
		}
    };
    
    template<typename T>
    struct ObjectFactorySingletonT : public ObjectFactory
    {
        ObjectFactorySingletonT(const String& name, const String& parent, bool isVirtual = false)
        {
            // register class to lua
            LuaBinder::instance()->registerClass(name.c_str(), parent.c_str());
			LuaBinder::instance()->registerObject(name, name, T::instance());
            
            m_name = name;
            m_classInfo.m_singleton = true;
            m_classInfo.m_virtual = isVirtual;
			m_classInfo.m_module = getCurrentRegisterModuleName();
            m_classInfo.m_parent = parent;
            
            Class::addClass(name, this);
            T::bindMethods();
        }
        
        virtual Object* create()
        {
            return T::instance();
        }

		virtual Object* getDefaultObject()
		{
			return nullptr;
		}
    };
}

#define ECHO_CLASS_NAME(m_class) #m_class

#define ECHO_CLASS(m_class, m_parent)														\
public:																						\
	typedef m_parent Super;																	\
																							\
	virtual const String& getClassName() const override									    \
	{																						\
		static String className=#m_class;													\
		return className;																	\
	}																						\
																							\
	static void initClassInfo()																\
	{																						\
		static Echo::ObjectFactoryT<m_class> G_OBJECT_FACTORY(#m_class, #m_parent);			\
	}																						\
																							\
	static void bindMethods();																\
																							\
																							\
private:															

#define ECHO_VIRTUAL_CLASS(m_class, m_parent)												\
public:																						\
	virtual const String& getClassName() const override									    \
	{																						\
		static String className=#m_class;													\
		return className;																	\
	}																						\
																							\
	static void initClassInfo()																\
	{																						\
		static Echo::ObjectFactoryT<m_class> G_OBJECT_FACTORY(#m_class, #m_parent, true);	\
	}																						\
																							\
	static void bindMethods();																\
																							\
private:				


#define ECHO_SINGLETON_CLASS(m_class, m_parent)												\
public:																						\
	virtual const String& getClassName() const override										\
	{																						\
		static String className=#m_class;													\
		return className;																	\
	}																						\
																							\
	static void initClassInfo()																\
	{																						\
		static Echo::ObjectFactorySingletonT<m_class>										\
		G_OBJECT_FACTORY(#m_class, #m_parent, true);										\
	}																						\
																							\
	static void bindMethods();																\
																							\
private:		

#define CLASS_BIND_METHOD(m_class, method) \
	Echo::Class::bindMethod(#m_class, &m_class::method, #method)

#define CLASS_REGISTER_PROPERTY(m_class, name, type, getter, setter) \
	Echo::Class::registerProperty(#m_class, name, type, #getter, #setter)

#define CLASS_REGISTER_PROPERTY_HINT(m_class, name, hintType, hintStr) \
	Echo::Class::registerPropertyHint(#m_class, name, PropertyHint(hintType, hintStr))

#define CLASS_REGISTER_SIGNAL(class, signal) \
    Echo::Class::registerSignal(#class, #signal, &class::getSignal##signal)
