#ifndef TABLE_HPP_
#define TABLE_HPP_

#include <cassert>
#include <memory>
#include <span>
#include <utility>

namespace satisfactory {

template <typename T>
class Table {
 public:
  constexpr Table() noexcept = default;
  Table(int width, int height) noexcept
      : width_(width),
        height_(height),
        data_(std::make_unique<T[]>(width * height)) {}

  Table(const Table& other) noexcept : Table(other.width_, other.height_) {
    const int size = width_ * height_;
    std::copy(other.data_.get(), other.data_.get() + size, data_.get());
  }

  Table& operator=(const Table& other) noexcept {
    const int current_size = width_ * height_;
    const int new_size = other.width_ * other.height_;
    if (current_size != new_size) data_ = std::make_unique<T[]>(new_size);
    width_ = other.width_;
    height_ = other.height_;
    std::copy(other.data_.get(), other.data_.get() + new_size, data_.get());
    return *this;
  }

  Table(Table&& other) noexcept
      : width_(std::exchange(other.width_, 0)),
        height_(std::exchange(other.height_, 0)),
        data_(std::exchange(other.data_, nullptr)) {}

  Table& operator=(Table&& other) noexcept {
    data_ = std::exchange(other.data_, nullptr);
    width_ = std::exchange(other.width_, 0);
    height_ = std::exchange(other.height_, 0);
    return *this;
  }

  // Accessors

  constexpr int width() const noexcept { return width_; }
  constexpr int height() const noexcept { return height_; }

  constexpr std::span<T> Row(int y) noexcept {
    assert(0 <= y && y < height_);
    return std::span(data_.get() + y * width_, width_);
  }

  constexpr std::span<const T> Row(int y) const noexcept {
    assert(0 <= y && y < height_);
    return std::span(data_.get() + y * width_, width_);
  }

  constexpr std::span<T> operator[](int y) noexcept { return Row(y); }
  constexpr std::span<const T> operator[](int y) const noexcept {
    return Row(y);
  }

 private:
  int width_ = 0, height_ = 0;
  std::unique_ptr<T[]> data_;
};

}  // namespace satisfactory

#endif  // TABLE_HPP_
