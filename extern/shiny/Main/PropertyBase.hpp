#ifndef SH_PROPERTYBASE_H
#define SH_PROPERTYBASE_H

#include <string>
#include <map>

#include <boost/shared_ptr.hpp>

namespace sh
{
	class StringValue;
	class PropertySetGet;
	class LinkedValue;

	enum ValueType
	{
		VT_String,
		VT_Int,
		VT_Float,
		VT_Vector2,
		VT_Vector3,
		VT_Vector4
	};

	class PropertyValue
	{
	public:
		PropertyValue() {}

		virtual ~PropertyValue() {}

		std::string _getStringValue() { return mStringValue; }

		virtual std::string serialize() = 0;

	protected:
		std::string mStringValue; ///< this will possibly not contain anything in the specialised classes
	};
	typedef boost::shared_ptr<PropertyValue> PropertyValuePtr;

	class StringValue : public PropertyValue
	{
	public:
		StringValue (const std::string& in);
		std::string get() const { return mStringValue; }

		virtual std::string serialize();
	};

	/**
	 * @brief Used for retrieving a named property from a context
	 */
	class LinkedValue : public PropertyValue
	{
	public:
		LinkedValue (const std::string& in);

		std::string get(PropertySetGet* context) const;

		virtual std::string serialize();
	};

	class FloatValue : public PropertyValue
	{
	public:
		FloatValue (float in);
		FloatValue (const std::string& in);
		float get() const { return mValue; }

		virtual std::string serialize();
	private:
		float mValue;
	};

	class IntValue : public PropertyValue
	{
	public:
		IntValue (int in);
		IntValue (const std::string& in);
		int get() const { return mValue; }

		virtual std::string serialize();
	private:
		int mValue;
	};

	class BooleanValue : public PropertyValue
	{
	public:
		BooleanValue (bool in);
		BooleanValue (const std::string& in);
		bool get() const { return mValue; }

		virtual std::string serialize();
	private:
		bool mValue;
	};

	class Vector2 : public PropertyValue
	{
	public:
		Vector2 (float x, float y);
		Vector2 (const std::string& in);

		float mX, mY;

		virtual std::string serialize();
	};

	class Vector3 : public PropertyValue
	{
	public:
		Vector3 (float x, float y, float z);
		Vector3 (const std::string& in);

		float mX, mY, mZ;

		virtual std::string serialize();
	};

	class Vector4 : public PropertyValue
	{
	public:
		Vector4 (float x, float y, float z, float w);
		Vector4 (const std::string& in);

		float mX, mY, mZ, mW;

		virtual std::string serialize();
	};

	/// \brief base class that allows setting properties with any kind of value-type
	class PropertySet
	{
	public:
        virtual ~PropertySet() {}
		void setProperty (const std::string& name, PropertyValuePtr& value, PropertySetGet* context);

	protected:
		virtual bool setPropertyOverride (const std::string& name, PropertyValuePtr& value, PropertySetGet* context);
		///< @return \a true if the specified property was found, or false otherwise
	};

	typedef std::map<std::string, PropertyValuePtr> PropertyMap;

	/// \brief base class that allows setting properties with any kind of value-type and retrieving them
	class PropertySetGet
	{
	public:
		PropertySetGet (PropertySetGet* parent);
		PropertySetGet ();

		virtual ~PropertySetGet() {}

		void save (std::ofstream& stream, const std::string& indentation);

		void copyAll (PropertySet* target, PropertySetGet* context, bool copyParent=true);
		///< call setProperty for each property/value pair stored in \a this
		void copyAll (PropertySetGet* target, PropertySetGet* context, bool copyParent=true);
		///< call setProperty for each property/value pair stored in \a this

		void setParent (PropertySetGet* parent);
		PropertySetGet* getParent () { return mParent; }
		void setContext (PropertySetGet* context);
		PropertySetGet* getContext();

		virtual void setProperty (const std::string& name, PropertyValuePtr value);
		PropertyValuePtr& getProperty (const std::string& name);

		void deleteProperty (const std::string& name);

		const PropertyMap& listProperties() { return mProperties; }

		bool hasProperty (const std::string& name) const;

	private:
		PropertyMap mProperties;

	protected:
		PropertySetGet* mParent;
		///< the parent can provide properties as well (when they are retrieved via getProperty) \n
		/// multiple levels of inheritance are also supported \n
		/// children can override properties of their parents

		PropertySetGet* mContext;
		///< used to retrieve linked property values
	};

	template <typename T>
	static T retrieveValue (boost::shared_ptr<PropertyValue>& value, PropertySetGet* context)
	{
		if (typeid(*value).name() == typeid(LinkedValue).name())
		{
			std::string v = static_cast<LinkedValue*>(value.get())->get(context);
			PropertyValuePtr newVal = PropertyValuePtr (new StringValue(v));
			return retrieveValue<T>(newVal, NULL);
		}
		if (typeid(T).name() == typeid(*value).name())
		{
			// requested type is the same as source type, only have to cast it
			return *static_cast<T*>(value.get());
		}

		if ((typeid(T).name() == typeid(StringValue).name())
			&& typeid(*value).name() != typeid(StringValue).name())
		{
			// if string type is requested and value is not string, use serialize method to convert to string
			T* ptr = new T (value->serialize()); // note that T is always StringValue here, but we can't use it here
			value = boost::shared_ptr<PropertyValue> (static_cast<PropertyValue*>(ptr));
			return *ptr;
		}

		{
			// remaining case: deserialization from string by passing the string to constructor of class T
			T* ptr = new T(value->_getStringValue());
			PropertyValuePtr newVal (static_cast<PropertyValue*>(ptr));
			value = newVal;
			return *ptr;
		}
	}
	///<
	/// @brief alternate version that supports linked values (use of $variables in parent material)
	/// @note \a value is changed in-place to the converted object
	/// @return converted object \n

	/// Create a property from a string
	inline PropertyValuePtr makeProperty (const std::string& prop)
	{
		if (prop.size() > 1 && prop[0] == '$')
			return PropertyValuePtr (static_cast<PropertyValue*>(new LinkedValue(prop)));
		else
			return PropertyValuePtr (static_cast<PropertyValue*> (new StringValue(prop)));
	}

	template <typename T>
	/// Create a property of any type
	/// Example: sh::makeProperty (new sh::Vector4(1, 1, 1, 1))
	inline PropertyValuePtr makeProperty (T* p)
	{
		return PropertyValuePtr ( static_cast<PropertyValue*>(p) );
	}
}

#endif
