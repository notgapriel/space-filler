#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <Magick++.h>

using namespace Magick;

using discrete_pixel_type = std::array<std::uint16_t, 3>;

class BooleanMatrix {
private:
  using Self = BooleanMatrix;

public:
  inline BooleanMatrix(const std::size_t rows, const std::size_t columns) : _rows(rows), _columns(columns), _values(rows * columns) {}
  BooleanMatrix(const Self&) = default;
  BooleanMatrix(Self&&)      = default;

  constexpr inline auto rows(void) const { return _rows; }
  constexpr inline auto columns(void) const { return _columns; }
  constexpr inline auto begin(void) const { return _values.begin(); }
  constexpr inline auto begin(void) { return _values.begin(); }
  constexpr inline auto end(void) const { return _values.end(); }
  constexpr inline auto end(void) { return _values.end(); }
  constexpr inline auto size(void) const { return _values.size(); }

  constexpr inline void set(const std::size_t index, bool value = true) { _values[index] = value; }
  constexpr inline void reset(void) {
    for (auto value : _values) {
      value = false;
    }
  }

  constexpr inline bool operator[](const std::size_t index) const { return _values[index]; }

  constexpr Self& operator=(const Self&) = default;
  constexpr Self& operator=(Self&&)      = default;

private:
  std::size_t       _rows;
  std::size_t       _columns;
  std::vector<bool> _values;
};

class Hilbert {
private:
  using Self = Hilbert;

public:
  using children_type = std::array<std::unique_ptr<Hilbert>, 4>;

  constexpr inline Hilbert(void) : _order(0), _children{ NULL, NULL, NULL, NULL } {}
  constexpr inline Hilbert(const std::size_t order, auto&& children) : _order(order), _children(std::forward<decltype(children)>(children)) {}
  constexpr Hilbert(const Self&) = default;
  constexpr Hilbert(Self&&)      = default;

  static constexpr inline std::size_t get_side_length(const std::size_t order) {
    if (order == 0) {
      return 3;
    } else {
      // keeps all line lengths the same
      return ((get_side_length(order - 1) * 2) + 1);
    }
  }
  constexpr inline std::size_t get_side_length(void) const { return get_side_length(_order); }
  constexpr inline std::size_t get_child_side_length(void) const { return get_side_length(_order - 1); }

  using draw_bits_type = BooleanMatrix;

private:
  template <std::size_t Quadrant_>
  constexpr inline const void populate_child(auto& bits) const {
    const std::size_t side_length       = get_side_length();
    const std::size_t child_side_length = get_child_side_length();

    BooleanMatrix child_bits(child_side_length, child_side_length);

    child_bits.reset();
    if (_children[Quadrant_]) {
      const auto& child = _children[Quadrant_];

      child_bits = child->draw();
    } else {
      for (std::size_t x = 0; x < child_side_length; x++) {
        child_bits.set(x);
      }

      for (std::size_t y = 1; y < child_side_length; y++) {
        const std::size_t X_OFFSETS[2] = { 0, child_side_length - 1 };

        child_bits.set(X_OFFSETS[0] + y * child_side_length);
        child_bits.set(X_OFFSETS[1] + y * child_side_length);
      }
    }
    for (std::size_t x = 0; x < child_side_length; x++) {
      for (std::size_t y = 0; y < child_side_length; y++) {
        const std::size_t X_OFFSET = Quadrant_ == 0 || Quadrant_ == 3 ? side_length - child_side_length : 0;
        const std::size_t Y_OFFSET = Quadrant_ == 2 || Quadrant_ == 3 ? side_length - child_side_length : 0;

        if (!child_bits[x + child_side_length * y]) continue;

        if constexpr (Quadrant_ == 0 || Quadrant_ == 1) {
          bits.set((x + X_OFFSET) + side_length * (y + Y_OFFSET));
        } else if constexpr (Quadrant_ == 2) {
          bits.set(((child_side_length - 1 - y) + X_OFFSET) + side_length * ((child_side_length - 1 - x) + Y_OFFSET));
        } else if constexpr (Quadrant_ == 3) {
          bits.set((y + X_OFFSET) + side_length * (x + Y_OFFSET));
        } else {
          static_assert(false, "Invalid quadrant supplied to function, must be [0,3]");
        }
      }
    }
  }

public:
  inline void draw(draw_bits_type& bits) const {
    bits.reset();

    if (_order == 0) {
      for (std::size_t x = 0; x < bits.columns(); x++) {
        for (std::size_t y = 0; y < bits.rows(); y++) {
          if ((x == 0 || (x == bits.columns() - 1)) || (y == 0)) {
            bits.set(x + bits.columns() * y);
          }
        }
      }
    } else {
      const std::size_t side_length       = get_side_length();
      const std::size_t child_side_length = get_child_side_length();

      populate_child<0>(bits);
      populate_child<1>(bits);
      populate_child<2>(bits);
      populate_child<3>(bits);

      bits.set(0 + child_side_length * side_length, true);                       // left join
      bits.set(side_length - 1 + child_side_length * side_length, true);         // right join
      bits.set(child_side_length + (child_side_length - 1) * side_length, true); // top-middle join
    }
  }

  inline draw_bits_type draw(void) const {
    const std::size_t side_length = get_side_length();
    draw_bits_type    out(side_length, side_length);
    draw(out);
    return out;
  }

  static constexpr inline Self make_full(const std::size_t order) {
    if (order == 0) {
      return Self();
    } else {
      return Self(
        order,
        children_type{
          std::make_unique<Hilbert>(make_full(order - 1)),
          std::make_unique<Hilbert>(make_full(order - 1)),
          std::make_unique<Hilbert>(make_full(order - 1)),
          std::make_unique<Hilbert>(make_full(order - 1)),
        }
      );
    }
  }

private:
  // 0 - top right
  // 1 - top left
  // 2 - bottom left
  // 3 - bottom right
  std::size_t   _order;
  children_type _children;
};

constexpr inline BooleanMatrix pad(const BooleanMatrix& bits, const std::size_t pad = 1) {
  BooleanMatrix out(bits.rows() + pad * 2, bits.columns() + pad * 2);

  out.reset();
  for (std::size_t row = 0; row < bits.rows(); row++) {
    for (std::size_t column = 0; column < bits.columns(); column++) {
      if (bits[row * bits.columns() + column]) out.set((row + pad) * (bits.columns() + pad * 2) + (column + pad));
    }
  }

  return out;
}

static inline std::vector<std::vector<discrete_pixel_type>> black_and_white(const BooleanMatrix& bits) {
  std::vector<std::vector<discrete_pixel_type>> out(bits.rows());

  for (std::size_t row = 0; row < bits.rows(); row++) {
    out[row] = std::vector<discrete_pixel_type>(bits.columns());
    for (std::size_t column = 0; column < bits.columns(); column++) {
      out[row][column] = bits[row * bits.columns() + column] ? discrete_pixel_type{ 0, 0, 0 } : discrete_pixel_type{ 65535, 65535, 65535 };
    }
  }

  return out;
}

static inline void make_image_from_array(const std::vector<std::vector<discrete_pixel_type>>& matrix, const std::string& file_name) {
  // matrix assumed square
  Image image(Geometry(matrix.size(), matrix.size()), Color(0, 0, 0, 0));

  image.type(TrueColorType);
  image.modifyImage();

  // Allocate pixel view
  Pixels view(image);

  // auto pixels = image.setPixels(2, 2, 1, 3);
  auto pixels = view.get(0, 0, matrix.size(), matrix.size());

  for (std::size_t row = 0; row < matrix.size(); row++) {
    for (std::size_t column = 0; column < matrix.size(); column++) {
      *pixels++ = Color(matrix[row][column][0], matrix[row][column][1], matrix[row][column][2]);
    }
  }

  view.sync();
  image.write(file_name);
}

int main(const int argc, const char *const *const argv) {
  InitializeMagick(nullptr);

  Hilbert hilbert = Hilbert::make_full(3);

  auto draw_bits = hilbert.draw();

  auto padded_bits = pad(draw_bits);

  auto image_matrix = black_and_white(padded_bits);

  make_image_from_array(image_matrix, "matrix.png");

  return 0;
}
