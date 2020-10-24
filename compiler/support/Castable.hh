#pragma once

#include <support/Assert.hh>

template <typename T, typename Kind>
concept HasKind = requires(T *t) {
    static_cast<Kind>(T::KIND);
};

template <typename, typename Kind, bool Mutable>
struct Castable {
    template <typename T>
    T *as() requires HasKind<T, Kind> && Mutable;
    template <typename T>
    T *as_or_null() requires HasKind<T, Kind> && Mutable;

    template <typename T>
    const T *as() const requires HasKind<T, Kind>;
    template <typename T>
    const T *as_or_null() const requires HasKind<T, Kind>;

    template <typename T>
    bool is() const requires HasKind<T, Kind>;
};

template <typename Derived, typename Kind, bool Mutable>
template <typename T>
T *Castable<Derived, Kind, Mutable>::as() requires HasKind<T, Kind> && Mutable {
    return const_cast<T *>(static_cast<const Castable *>(this)->as<T>());
}

template <typename Derived, typename Kind, bool Mutable>
template <typename T>
T *Castable<Derived, Kind, Mutable>::as_or_null() requires HasKind<T, Kind> && Mutable {
    return const_cast<T *>(static_cast<const Castable *>(this)->as_or_null<T>());
}

template <typename Derived, typename Kind, bool Mutable>
template <typename T>
const T *Castable<Derived, Kind, Mutable>::as() const requires HasKind<T, Kind> {
    ASSERT(is<T>());
    ASSERT_PEDANTIC(dynamic_cast<const T *>(static_cast<const Derived *>(this)) != nullptr);
    return static_cast<const T *>(this);
}

template <typename Derived, typename Kind, bool Mutable>
template <typename T>
const T *Castable<Derived, Kind, Mutable>::as_or_null() const requires HasKind<T, Kind> {
    return is<T>() ? as<T>() : nullptr;
}

template <typename Derived, typename Kind, bool Mutable>
template <typename T>
bool Castable<Derived, Kind, Mutable>::is() const requires HasKind<T, Kind> {
    return static_cast<const Derived *>(this)->kind() == T::KIND;
}
