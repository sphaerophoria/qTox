/*
    Copyright © 2019 by The qTox Project Contributors

    This file is part of qTox, a Qt-based graphical interface for Tox.

    qTox is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    qTox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with qTox.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STORNGTYPE_H
#define STORNGTYPE_H

#include <QHash>
#include <type_traits>

template <typename T>
struct Addable
{
    T operator+(T const& other) const { return static_cast<T const&>(*this).get() + other.get(); };
};

template <typename T, typename Underlying>
struct UnderlyingAddable
{
    T operator+(Underlying const& other) const
    {
        return T(static_cast<T const&>(*this).get() + other);
    };
};

template <typename T, typename Underlying>
struct UnitlessDifferencable
{
    T operator-(Underlying const& other) const
    {
        return T(static_cast<T const&>(*this).get() - other);
    };

    Underlying operator-(T const& other) const
    {
        return static_cast<T const&>(*this).get() - other.get();
    }
};

template <typename T, typename>
struct Incrementable
{
    T& operator++()
    {
        auto& underlying = static_cast<T&>(*this).get();
        ++underlying;
        return static_cast<T&>(*this);
    }

    T operator++(int)
    {
        auto ret = T(static_cast<T const&>(*this));
        ++(*this);
        return ret;
    }
};


template <typename T, typename>
struct EqualityComparible
{
    bool operator==(const T& other) const { return static_cast<T const&>(*this).get() == other.get(); };
    bool operator!=(const T& other) const
    {
        return static_cast<T const&>(*this).get() != other.get();
    };
};


template <typename T, typename Underlying>
struct Hashable
{
    friend uint qHash(const Hashable<T, Underlying>& id)
    {
        return qHash(static_cast<T const&>(id).get());
    }
};

template <typename T, typename Underlying>
struct Orderable : public EqualityComparible<T, Underlying>
{
    bool operator<(const T& rhs) const { return static_cast<T const&>(*this).get() < rhs.get(); }
    bool operator>(const T& rhs) const { return static_cast<T const&>(*this).get() > rhs.get(); }
    bool operator>=(const T& rhs) const { return static_cast<T const&>(*this).get() >= rhs.get(); }
    bool operator<=(const T& rhs) const { return static_cast<T const&>(*this).get() <= rhs.get(); }
};



/* This class facilitates creating a named class which wraps underlying POD,
 * avoiding implict casts and arithmetic of the underlying data.
 * Usage: Declare named type with arbitrary tag, then hook up Qt metatype for use
 * in signals/slots. For queued connections, registering the metatype is also
 * required before the type is used.
 *   using ReceiptNum = NamedType<uint32_t, struct ReceiptNumTag>;
 *   Q_DECLARE_METATYPE(ReceiptNum);
 *   qRegisterMetaType<ReceiptNum>();
 */

template <typename T, typename Tag, template <typename, typename> class... Properties>
class NamedType : public Properties<NamedType<T, Tag, Properties...>, T>...
{
public:
    using UnderlyingType = T;

    NamedType() {}
    constexpr explicit NamedType(T const& value) : value_(value) {}
    T& get() { return value_; }
    T const& get() const {return value_; }
private:
    T value_;
};


namespace HashableDetail
{
    template <typename...>
    struct IsOneOf {
        static constexpr bool value = false;
    };

    template <typename F, typename S, typename... T>
    struct IsOneOf<F, S, T...> {
        static constexpr bool value =
            std::is_same<F, S>::value || IsOneOf<F, T...>::value;
    };

    template <typename T>
    struct IsHashable : std::false_type
    {};

    template <typename T, typename Tag, template <typename, typename> class... Properties>
    struct IsHashable<NamedType<T, Tag, Properties...>> :
        IsOneOf<Hashable<NamedType<T, Tag, Properties...>, T>, Properties<NamedType<T, Tag, Properties...>, T>...>
    {};


    template <typename T, typename Tag, template <typename, typename> class... Properties>
    struct NamedTypeHashTrueImpl
    {
        size_t operator()(NamedType<T, Tag, Properties...> const& item)
        {
            return std::hash<T>()(item.get());
        }
    };

    struct EmptyStruct
    {};

    template <typename T>
    struct NamedTypeHashImpl
    {};

    template <typename T, typename Tag, template <typename, typename> class... Properties>
    struct NamedTypeHashImpl<NamedType<T, Tag, Properties...>>
        : std::conditional<HashableDetail::IsHashable<NamedType<T, Tag, Properties...>>::value,
                    NamedTypeHashTrueImpl<T, Tag, Properties...>,
                    EmptyStruct>::type
    {};
}

namespace std
{
    template <typename T, typename Tag, template <typename, typename> class... Properties>
    struct hash<NamedType<T, Tag, Properties...>> : HashableDetail::NamedTypeHashImpl<NamedType<T, Tag, Properties...>>
    {};
}

#endif // STORNGTYPE_H
