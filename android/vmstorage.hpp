
#ifndef COMPONENTS_ANDROID_VMSTORAGE_HPP_
#define COMPONENTS_ANDROID_VMSTORAGE_HPP_

class VMStorage {
public:
	static JavaVM* getVM();
	static jclass getCursorClass();
	static jmethodID getUpdateControlsMethod();
};

#endif /* COMPONENTS_ANDROID_VMSTORAGE_HPP_ */
