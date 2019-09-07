#ifndef PLISTPP2_PLIST_H
#define PLISTPP2_PLIST_H

#include <plist/plist.h>

namespace plist {

    class c_string_key {
    private:
        const char *value;
    public:
        c_string_key(const char *value) : value(value) { }
        operator const char *() const { return value; }
    };

    class object {

    private:
        plist_t wrapped;

    public:
        object() : wrapped(nullptr) {}
        object(object const &o) : wrapped(plist_copy(o.wrapped)) {}
        object(object &&o) : wrapped(o.wrapped) {
            if (!plist_get_parent(wrapped))
                o.wrapped = nullptr;
        }

        ~object() { if (!plist_get_parent(wrapped)) plist_free(wrapped); }

        object(plist_t value) : wrapped(value) {}
        object(bool value) : object(plist_new_bool(value)) {}
        object(uint64_t value) : object(plist_new_uint(value)) {}
        object(double value) : object(plist_new_real(value)) {}
        object(const char *value) : object(plist_new_string(value)) {}
        object(std::string const &value) : object(plist_new_string(value.c_str())) {}

        object &assign(plist_t value, bool owned = true) {
            if (!plist_get_parent(value))
                plist_free(release());
            wrapped = value;
            return *this;
        }

        object &operator =(plist_t value) { return assign(value); }
        object &operator =(object const &value) { return *this = object(value).release(); }
        object &operator =(object &&value) { return *this = value.release(); }
        object &operator =(bool value) { return *this = plist_new_bool(value); }
        object &operator =(uint64_t value) { return *this = plist_new_uint(value); }
        object &operator =(double value) { return *this = plist_new_real(value); }
        object &operator =(const char *value) { return *this = plist_new_string(value); }
        object &operator =(std::string const &value) { return *this = value.c_str(); }

        plist_t plist_ptr() const {
            return wrapped;
        }

        plist_t release() {
            plist_t ret = wrapped;
            wrapped = nullptr;
            return ret;
        }

        object copy() const {
            return object(plist_copy(wrapped));
        }

        plist_type type() const {
            return plist_get_node_type(wrapped);
        }

        template <typename T>
        T get(T def = T()) const;


        size_t size() const {
            switch (type()) {
                case PLIST_ARRAY:
                    return plist_array_get_size(wrapped);
                case PLIST_DICT:
                    return plist_dict_get_size(wrapped);
                default:
                    throw std::runtime_error("size() not allowed on this type");
            }
        }

        void append(object const &o) {
            if (type() != PLIST_ARRAY)
                throw std::runtime_error("append() not allowed on non-arrays");
            plist_array_append_item(wrapped, object(o).release());
        }
        void append(object &&o) {
            if (type() != PLIST_ARRAY)
                throw std::runtime_error("append() not allowed on non-arrays");
            if (plist_get_parent(o.wrapped))
                plist_array_append_item(wrapped, o.copy().release());
            else
                plist_array_append_item(wrapped, o.release());
        }

        void set(size_t index, object const &o) {
            if (type() != PLIST_ARRAY)
                throw std::runtime_error("set(index) not allowed on non-arrays");
            plist_array_set_item(wrapped, object(o).release(), index);
        }
        void set(size_t index, object &&o) {
            if (type() != PLIST_ARRAY)
                throw std::runtime_error("set(index) not allowed on non-arrays");
            if (plist_get_parent(o.wrapped))
                plist_array_set_item(wrapped, o.copy().release(), index);
            else
                plist_array_set_item(wrapped, o.release(), index);
        }

        void set(c_string_key key, object const &o) {
            if (type() != PLIST_ARRAY)
                throw std::runtime_error("set(key) not allowed on non-arrays");
            plist_dict_set_item(wrapped, key, object(o).release());
        }
        void set(c_string_key key, object &&o) {
            if (type() != PLIST_ARRAY)
                throw std::runtime_error("set(key) not allowed on non-arrays");
            if (plist_get_parent(o.wrapped))
                plist_dict_set_item(wrapped, key, o.copy().release());
            else
                plist_dict_set_item(wrapped, key, o.release());
        }

        object at(size_t index) const {
            if (type() != PLIST_ARRAY)
                throw std::runtime_error("at(index) not allowed on non-arrays");
            plist_t ret = plist_array_get_item(wrapped, index);
            return object(ret);
        }
        object at(c_string_key key) const {
            if (type() != PLIST_DICT)
                throw std::runtime_error("at(key) not allowed on non-dicts");
            plist_t ret = plist_dict_get_item(wrapped, key);
            return object(ret);
        }

    };

    static object boolean(bool value) { return object(value); }
    static object integer(uint64_t value) { return object(value); }
    static object real(double value) { return object(value); }
    static object string(const char *value) { return object(value); }
    static object string(std::string const &value) { return object(value); }
    static object uid(uint64_t value) { return object(plist_new_uid(value)); }
    static object array() { return object(plist_new_array()); }
    static object dictionary() { return object(plist_new_dict()); }

    template <>
    inline uint64_t object::get<uint64_t>(uint64_t def) const {
        uint64_t res = 0;
        switch (type()) {
            case PLIST_UINT:
                plist_get_uint_val(wrapped, &res);
                break;
            case PLIST_UID:
                plist_get_uid_val(wrapped, &res);
                break;
            default:
                break;
        }
        return res;
    }

    template <>
    inline bool object::get<bool>(bool def) const {
        uint8_t res = def;
        switch (type()) {
            case PLIST_BOOLEAN:
                plist_get_bool_val(wrapped, &res);
                break;
            case PLIST_UINT: {
                uint64_t v = 0;
                plist_get_uint_val(wrapped, &v);
                res = (v != 0);
                break;
            }
            default:
                break;
        }
        return (res != 0);
    }

    template <>
    inline double object::get<double>(double def) const {
        double res = 0;
        plist_get_real_val(wrapped, &res);
        return res;
    }

    template <>
    inline const char *object::get<const char *>(const char *def) const {
        char *res = nullptr;
        plist_get_string_val(wrapped, &res);
        return res ? res : def;
    }

    template <>
    inline std::string object::get<std::string>(std::string def) const {
        const char *res = get<const char *>(nullptr);
        return res ? std::string(res) : def;
    }

}

#endif //PLISTPP2_PLIST_H
