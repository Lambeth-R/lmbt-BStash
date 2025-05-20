#pragma once

// A bit sloppy and chubby way to transfer memory objects into buffer.

#include <map>
#include <string>
#include <vector>

#include <typeindex>

#define ADD_IMPL(C, T) \
    inline const std::vector<uint8_t> Marshallable<T>::Marshall() const \
    { return C<T>::MarshallImpl(m_Object); }; \
    inline void Marshallable<T>::Unmarshall(const std::vector<uint8_t> &marshalledData, T &result, std::size_t &processedData) \
    { result = C<T>::UnmarshallImpl(marshalledData, processedData); };

#define ADD_IMPL_SUB_ONE(C, T, T1) \
    inline const std::vector<uint8_t> Marshallable<T<T1>>::Marshall() const \
    { return C<T1>::MarshallImpl(m_Object); }; \
    inline void Marshallable<T<T1>>::Unmarshall(const std::vector<uint8_t> &marshalledData, T<T1> &result, std::size_t &processedData) \
    { result = C<T1>::UnmarshallImpl(marshalledData, processedData); };

#define ADD_IMPL_SUB_TWO(C, T, T1, T2) \
    inline const std::vector<uint8_t> Marshallable<T<T1,T2>>::Marshall() const \
    { return C<T1,T2>::MarshallImpl(m_Object); }; \
    inline void Marshallable<T<T1, T2>>::Unmarshall(const std::vector<uint8_t> &marshalledData, T<T1, T2> &result, std::size_t &processedData) \
    { result = C<T1,T2>::UnmarshallImpl(marshalledData, processedData); };

struct Marshall;

template <class T>
struct Marshallable
{
    friend struct Marshall;
    using   MarshallType = T;
    const   MarshallType &m_Object;
    static void Unmarshall(const std::vector<uint8_t> &marshalledData, MarshallType &result, std::size_t &processedData);
    virtual const std::vector<uint8_t> Marshall() const;

public:
    Marshallable(const MarshallType &obj) : m_Object(obj) {};
};

struct Marshall
{
    template <typename T>
    static void MarshallObject(const Marshallable<T> &obj, std::vector<uint8_t> &result)
    {
        result = obj.Marshall();
    }

    template <typename T>
    static T UnmarshallObject(const std::vector<uint8_t> &data, std::size_t &processedData)
    {
        T result = {};
        Marshallable<T>::Unmarshall(data, result, processedData);
        return result;
    }

    template <typename T>
    static T UnmarshallObject(const std::vector<uint8_t> &data)
    {
        T result = {};
        std::size_t processed_data = 0;
        Marshallable<T>::Unmarshall(data, result, processed_data);
        return result;
    }
};

template <class T, std::enable_if_t<std::is_trivial_v<T>, bool> = true>
struct MarshallableTrivialImpl : public Marshallable<T>
{
    inline static const std::vector<uint8_t> MarshallImpl(const T &obj)
    {
        const std::type_index type_info = typeid(T);
        std::vector<uint8_t> result(sizeof(std::size_t) + sizeof(T), 0x00);
        *(reinterpret_cast<std::size_t *>(result.data()))      = type_info.hash_code();
        *(reinterpret_cast<T *>(&result[sizeof(std::size_t)])) = obj;
        return result;
    }

    inline static T UnmarshallImpl(const std::vector<uint8_t> &marshalledData, std::size_t &processedData)
    {
        T result;
        if (std::type_index(typeid(T)).hash_code() == *reinterpret_cast<const std::size_t *>(marshalledData.data()))
        {
            result = *reinterpret_cast<const T*>(&marshalledData[sizeof(std::size_t)]);
        }
        processedData = sizeof(std::size_t) + sizeof(T);
        return result;
    }
};

ADD_IMPL(MarshallableTrivialImpl, int8_t);
ADD_IMPL(MarshallableTrivialImpl, uint8_t);

template <class Tchar>
struct MarshallableStringImpl : public Marshallable<std::basic_string<Tchar>>
{
    inline static const std::vector<uint8_t> MarshallImpl(const std::basic_string<Tchar> &obj)
    {
        const size_t copy_size = obj.size() * sizeof(Tchar);
        std::vector<uint8_t> result(sizeof(std::size_t) + copy_size, 0x0);
        *(reinterpret_cast<std::size_t *>(result.data())) = obj.size();
        memcpy(&result[sizeof(std::size_t)], obj.data(), copy_size);
        return result;
    }

    inline static std::basic_string<Tchar> UnmarshallImpl(const std::vector<uint8_t> &marshalledData, size_t &processedData)
    {
        const std::size_t unmarshall_size = *(reinterpret_cast<const std::size_t *>(marshalledData.data()));
        std::basic_string<Tchar> result(unmarshall_size, 0x00);
        memcpy(result.data(), &marshalledData[sizeof(std::size_t)], result.size() * sizeof(Tchar));
        processedData = sizeof(std::size_t) + result.size() * sizeof(Tchar);
        return result;
    }
};

ADD_IMPL_SUB_ONE(MarshallableStringImpl, std::basic_string, char);
ADD_IMPL_SUB_ONE(MarshallableStringImpl, std::basic_string, wchar_t);

template <class Tkey, class Tval>
struct MarshallableMapImpl : public Marshallable<std::map<Tkey, Tval>>
{
    inline static const std::vector<uint8_t> MarshallImpl(const std::map<Tkey, Tval> &obj)
    {
        std::vector<uint8_t> result(sizeof(std::size_t), 0x00);
        for (const auto &[key, val] : obj)
        {
            std::vector<uint8_t> temp_vector;
            Marshall::MarshallObject(Marshallable<Tkey>(key), temp_vector);
            result.insert(result.end(), std::make_move_iterator(temp_vector.begin()), std::make_move_iterator(temp_vector.end()));
            Marshall::MarshallObject(Marshallable<Tval>(val), temp_vector);
            result.insert(result.end(), std::make_move_iterator(temp_vector.begin()), std::make_move_iterator(temp_vector.end()));
        }
        *(reinterpret_cast<std::size_t *>(result.data())) = result.size() - sizeof(std::size_t);
        return result;
    }

    inline static std::map<Tkey, Tval> UnmarshallImpl(const std::vector<uint8_t> &marshalledData, size_t &processedData)
    {
        std::map<Tkey, Tval> result;
        std::size_t map_size = *reinterpret_cast<const size_t *>(marshalledData.data());
        auto it_block_st  = std::next(marshalledData.begin(),sizeof(std::size_t)),
             it_block_end = it_block_st;
        std::advance(it_block_end, sizeof(std::size_t) + sizeof(Tkey));
        while (processedData != map_size)
        {
            size_t processed_size = 0;
            Tkey key = Marshall::UnmarshallObject<Tkey>({ it_block_st, it_block_end }, processed_size);
            processedData += processed_size;
            if (processedData >= map_size)
            {
                break;
            }
            std::advance(it_block_st, processed_size);
            std::advance(it_block_end, processed_size);
            Tval value = Marshall::UnmarshallObject<Tval>({ it_block_st, marshalledData.end()}, processed_size);
            result[key] = value;
            processedData += processed_size;
            if (processedData >= map_size)
            {
                break;
            }
            std::advance(it_block_st,  processed_size);
            std::advance(it_block_end, processed_size);
        }
        return result;
    }
};

ADD_IMPL_SUB_TWO(MarshallableMapImpl, std::map, uint8_t, std::string);
ADD_IMPL_SUB_TWO(MarshallableMapImpl, std::map, uint8_t, std::wstring);
