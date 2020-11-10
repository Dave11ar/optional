#include <type_traits>
#include <functional>


struct nullopt_t {};
inline constexpr nullopt_t nullopt;

struct in_place_t {};
inline constexpr in_place_t in_place;

template<typename T, bool is_trivial>
struct optional_destructor_base {
  constexpr optional_destructor_base() noexcept :
    has_value(false), dummy(0) {}

  constexpr optional_destructor_base(T value_) :
    has_value(true), value(std::move(value_)) {}

  template<typename... Args>
  explicit constexpr optional_destructor_base(in_place_t, Args &&... args) :
      has_value(true), value(std::forward<Args>(args)...) {}


  ~optional_destructor_base() {
    reset();
  }

  void reset() {
    if (this->has_value) {
      this->value.~T();
      this->has_value = false;
    }
  }

  bool has_value;
  union {
    T value;
    char dummy;
  };
};

template<typename T>
struct optional_destructor_base<T, true> {
  constexpr optional_destructor_base() noexcept :
    has_value(false), dummy(0) {}

  constexpr optional_destructor_base(T value) :
    has_value(true), value(std::move(value)) {}

  template<typename... Args>
  explicit constexpr optional_destructor_base(in_place_t, Args &&... args) :
      has_value(true), value(std::forward<Args>(args)...) {}

  ~optional_destructor_base() = default;

  void reset() {
    this->has_value = false;
  }

  bool has_value;
  union {
    T value;
    char dummy;
  };
};

template <typename T, bool is_trivial>
struct optional_constructor_base : optional_destructor_base<T, std::is_trivially_destructible_v<T>> {
  using base = optional_destructor_base<T, std::is_trivially_destructible_v<T>>;
  using base::base;
  using base::reset;

  constexpr optional_constructor_base(optional_constructor_base const &other) {
    if ((this->has_value = other.has_value)) {
      new (&this->value) T(other.value);
    }
  }

  constexpr optional_constructor_base(optional_constructor_base &&other)
    noexcept(std::is_nothrow_move_constructible_v<T>) {
    if ((this->has_value = other.has_value)) {
      new(&this->value) T(std::move(other.value));
    }
  }

  optional_constructor_base &operator=(optional_constructor_base const &other) {
    if (this == &other) {
      return *this;
    }

    if (other.has_value) {
      if (this->has_value) {
        this->value = other.value;
      } else {
        new(&this->value) T(other.value);
        this->has_value = true;
      }
    } else {
      reset();
    }

    return *this;
  }

  optional_constructor_base& operator=(optional_constructor_base &&other)
    noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {
    if (this == &other) {
      return *this;
    }

    if (other.has_value) {
      if (this->has_value) {
        this->value = std::move(other.value);
      } else {
        new(&this->value) T(std::move(other.value));
        this->has_value = true;
      }
    } else {
      reset();
    }

    return *this;
  }
};

template <typename T>
struct optional_constructor_base<T, true> : optional_destructor_base<T, std::is_trivially_destructible_v<T>> {
  using base = optional_destructor_base<T, std::is_trivially_destructible_v<T>>;
  using base::base;
  using base::reset;

  constexpr optional_constructor_base(optional_constructor_base const &other) = default;
  constexpr optional_constructor_base(optional_constructor_base &&other) noexcept = default;

  optional_constructor_base &operator=(optional_constructor_base const &other) = default;
  optional_constructor_base& operator=(optional_constructor_base &&other) noexcept = default;
};

template<typename T>
struct optional : private optional_constructor_base<T, std::is_trivially_copyable_v<T>> {
  using base = optional_constructor_base<T, std::is_trivially_copyable_v<T>>;
  using base::base;
  using base::reset;

  constexpr optional(nullopt_t) noexcept : optional() {}

  constexpr optional(optional const &) = default;
  constexpr optional(optional &&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

  optional &operator=(optional const &other) = default;
  optional &operator=(optional &&other) noexcept(std::is_nothrow_move_assignable_v<T> &&
      std::is_nothrow_move_constructible_v<T>) = default;

  optional &operator=(nullopt_t) noexcept {
    reset();
    return *this;
  }

  constexpr explicit operator bool() const noexcept {
    return this->has_value;
  }

  constexpr T &operator*() noexcept {
    return this->value;
  }
  constexpr T const &operator*() const noexcept {
    return this->value;
  }

  constexpr T *operator->() noexcept {
    return &this->value;
  }
  constexpr T const *operator->() const noexcept {
    return &this->value;
  }

  template<typename... Args>
  void emplace(Args &&... args) {
    reset();

    new (&this->value) T(std::forward<Args>(args)...);
    this->has_value = true;
  }

  constexpr bool operator==(optional const &b) {
    if (this->has_value != b.has_value) {
      return false;
    }

    if (!this->has_value) {
      return true;
    } else {
      return this->value == b.value;
    }
  }

  constexpr bool operator!=(optional const &b) {
    return !(*this == b);
  }

  constexpr bool operator<(optional const &b) {
    if (!b.has_value) {
      return false;
    }

    if (!this->has_value) {
      return true;
    } else {
      return this->value < b.value;
    }
  }

  constexpr bool operator>(optional const &b) {
    if (!this->has_value) {
      return false;
    }

    if (!b.has_value) {
      return true;
    } else {
      return this->value > b.value;
    }
  }

  constexpr bool operator<=(optional const &b) {
    return !(*this > b);
  }

  constexpr bool operator>=( optional<T> const &b) {
    return !(*this < b);
  }
};
