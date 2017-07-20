// ---------------------------------------------------------------------------

#ifndef SingletonH
#define SingletonH
// ---------------------------------------------------------------------------

template <typename T>
class TSingleton {
public:
	static T& Instance();

protected:
	virtual ~TSingleton();
	inline explicit TSingleton();

private:
	static T* _instance;

	static T* CreateInstance();
};

template<typename T>
T* TSingleton<T>::_instance = 0;

// ---------------------------------------------------------------------------
#endif
