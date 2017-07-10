// ---------------------------------------------------------------------------

#pragma hdrstop

#include "Singleton.h"
// #include <cstdlib>
// ---------------------------------------------------------------------------
#pragma package(smart_init)

// ---------------------------------------------------------------------------
template <typename T>
TSingleton<T>::TSingleton() {
	assert(TSingleton::_instance == 0);
	TSingleton::_instance = static_cast<T*>(this);
}

// ---------------------------------------------------------------------------
template<typename T>
T& TSingleton<T>::Instance() {
	if (TSingleton::_instance == 0) {
		TSingleton::_instance = CreateInstance();
	}
	return *(TSingleton::_instance);
}

// ---------------------------------------------------------------------------
template<typename T>
inline T* TSingleton<T>::CreateInstance() {
	return new T();
}

// ---------------------------------------------------------------------------
template<typename T>
TSingleton<T>::~TSingleton() {
	if (Singleton::_instance != 0) {
		delete TSingleton::_instance;
	}
	TSingleton::_instance = 0;
}
// ---------------------------------------------------------------------------
