#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include <Magick++.h>

using namespace Magick;

using discrete_pixel_type = std::array<std::uint16_t, 3>;

template <typename T_>
class Grid {
private:
  using Self = Grid;

public:
  inline Grid(const std::size_t rows, const std::size_t columns) : _rows(rows), _columns(columns), _values(rows * columns) {}
  Grid(const Self&) = default;
  Grid(Self&&)      = default;

  constexpr inline auto rows(void) const { return _rows; }
  constexpr inline auto columns(void) const { return _columns; }
  constexpr inline auto begin(void) const { return _values.begin(); }
  constexpr inline auto begin(void) { return _values.begin(); }
  constexpr inline auto end(void) const { return _values.end(); }
  constexpr inline auto end(void) { return _values.end(); }
  constexpr inline auto size(void) const { return _values.size(); }

  constexpr inline void set(const std::size_t index, const T_ value) { _values[index] = value; }
  constexpr inline void reset(void) {
    for (auto value : _values) {
      value = false;
    }
  }

  constexpr inline decltype(auto) operator[](const std::size_t index) const { return _values[index]; }
  constexpr inline decltype(auto) operator[](const std::size_t index) { return _values[index]; }

  constexpr Self& operator=(const Self&) = default;
  constexpr Self& operator=(Self&&)      = default;

private:
  std::size_t     _rows;
  std::size_t     _columns;
  std::vector<T_> _values;
};

using BooleanGrid = Grid<bool>;

constexpr inline BooleanGrid pad(const BooleanGrid& bits, const std::size_t pad = 1) {
  BooleanGrid out(bits.rows() + pad * 2, bits.columns() + pad * 2);

  out.reset();
  for (std::size_t row = 0; row < bits.rows(); row++) {
    for (std::size_t column = 0; column < bits.columns(); column++) {
      if (bits[row * bits.columns() + column]) out.set((row + pad) * (bits.columns() + pad * 2) + (column + pad), true);
    }
  }

  return out;
}

static inline Grid<discrete_pixel_type> black_and_white(const BooleanGrid& bits) {
  Grid<discrete_pixel_type> out(bits.rows(), bits.columns());

  for (std::size_t i = 0; i < bits.size(); i++) {
    out[i] = bits[i] ? discrete_pixel_type{ 0, 0, 0 } : discrete_pixel_type{ MaxRGB, MaxRGB, MaxRGB };
  }

  return out;
}

static inline void make_image_from_grid(const Grid<discrete_pixel_type>& grid, const std::string& file_name) {
  Image image(Geometry(grid.columns(), grid.rows()), Color(0, 0, 0, 0));

  image.type(TrueColorType);
  image.modifyImage();

  // Allocate pixel view
  Pixels view(image);

  auto pixels = view.get(0, 0, grid.columns(), grid.rows());

  for (std::size_t row = 0; row < grid.rows(); row++) {
    for (std::size_t column = 0; column < grid.columns(); column++) {
      const auto pixel = grid[row * grid.columns() + column];
      *pixels++        = Color(pixel[0], pixel[1], pixel[2]);
    }
  }

  view.sync();
  image.write(file_name);
}

class Hilbert {
private:
  using Self = Hilbert;

public:
  static constexpr const std::size_t NUM_CHILDREN = 4;
  using children_type                             = std::array<std::unique_ptr<Hilbert>, NUM_CHILDREN>;

  constexpr inline Hilbert(std::size_t order = 0) : _order(order), _children{ NULL, NULL, NULL, NULL } {}
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

  constexpr inline std::size_t get_order(void) const { return _order; }
  constexpr inline std::size_t get_side_length(void) const { return get_side_length(get_order()); }
  constexpr inline std::size_t get_child_side_length(void) const { return get_side_length(get_order() - 1); }

  using draw_bits_type = BooleanGrid;

private:
  template <std::size_t Quadrant_>
  constexpr inline const void populate_child(auto& bits) const {
    const std::size_t side_length       = get_side_length();
    const std::size_t child_side_length = get_child_side_length();

    BooleanGrid child_bits(child_side_length, child_side_length);

    child_bits.reset();
    if (_children[Quadrant_]) {
      const auto& child = _children[Quadrant_];

      child_bits = child->draw();
    } else {
      for (std::size_t x = 0; x < child_side_length; x++) {
        child_bits.set(x, true);
      }

      for (std::size_t y = 1; y < child_side_length; y++) {
        const std::size_t X_OFFSETS[2] = { 0, child_side_length - 1 };

        child_bits.set(X_OFFSETS[0] + y * child_side_length, true);
        child_bits.set(X_OFFSETS[1] + y * child_side_length, true);
      }
    }
    for (std::size_t x = 0; x < child_side_length; x++) {
      for (std::size_t y = 0; y < child_side_length; y++) {
        const std::size_t X_OFFSET = Quadrant_ == 0 || Quadrant_ == 3 ? side_length - child_side_length : 0;
        const std::size_t Y_OFFSET = Quadrant_ == 2 || Quadrant_ == 3 ? side_length - child_side_length : 0;

        if (!child_bits[x + child_side_length * y]) continue;

        std::size_t x_pos, y_pos;

        if constexpr (Quadrant_ == 0 || Quadrant_ == 1) {
          x_pos = x;
          y_pos = y;
        } else if constexpr (Quadrant_ == 2) {
          // pi/2 rotation
          x_pos = (child_side_length - 1 - y);
          y_pos = x;
        } else if constexpr (Quadrant_ == 3) {
          // 3pi/2 rotation
          x_pos = y;
          y_pos = (child_side_length - 1 - x);
        } else {
          static_assert(false, "Invalid quadrant supplied to function, must be [0,3]");
        }

        bits.set((x_pos + X_OFFSET) + side_length * (y_pos + Y_OFFSET), true);
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
            bits.set(x + bits.columns() * y, true);
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

  constexpr inline bool place_matrix(
    const std::uint8_t rotation, const Grid<std::size_t>& orders, const std::size_t x_offset, const std::size_t y_offset
  ) & {
    if (get_order() == 0) return false;

    const std::size_t submatrix_size = 1 << get_order();

    for (std::size_t y = 0; y < submatrix_size; y++) {
      for (std::size_t x = 0; x < submatrix_size; x++) {
        const std::size_t index = (x + x_offset) + orders.columns() * (y + y_offset);

        if (orders[index] <= get_order()) {
          goto exit_loop;
        }
      }
    }

    // found nothing, leave.
    return false;

  exit_loop:
    static constexpr const std::size_t ROTATION_OFFSETS[] = { 0, 0, 3, 1 };

    const std::size_t child_submatrix_size = submatrix_size >> 1;

    const std::pair<std::size_t, std::size_t> child_offsets[] = {
      { x_offset + child_submatrix_size, y_offset },
      { x_offset, y_offset },
      { x_offset, y_offset + child_submatrix_size },
      { x_offset + child_submatrix_size, y_offset + child_submatrix_size },
    };

    for (std::size_t i = 0; i < _children.size(); i++) {
      const auto child_offset = child_offsets[(i + rotation) % NUM_CHILDREN];

      auto& child = (_children[i] = std::make_unique<Hilbert>(get_order() - 1));
      if (child->place_matrix(rotation + ROTATION_OFFSETS[i], orders, child_offset.first, child_offset.second)) continue; // succeeded, try next

      // failed, delete
      child.release();
    }

    return true;
  }

private:
  std::size_t _order;
  // 0 - top right
  // 1 - top left
  // 2 - bottom left
  // 3 - bottom right
  children_type _children;
};

inline double calculate_density(const std::size_t order) {
  const auto bits = Hilbert(order).draw();

  std::size_t sum = 0;
  for (const bool value : bits) {
    if (value) sum++;
  }

  return ((double) sum) / ((double) (bits.rows() * bits.columns()));
}

constexpr inline double calculate_perceived_brightness(const double r, const double g, const double b) {
  return std::sqrt(0.299 * r * r + 0.587 * g * g + 0.114 * b * b);
}

int main(const int argc, const char *const *const argv) {
  InitializeMagick(nullptr);

  const std::string in_file_path  = argc > 1 ? argv[1] : "in.png";
  const std::string out_file_path = argc > 2 ? argv[2] : "out.png";

  Image image;
  image.read(in_file_path);
  image.type(TrueColorType);

  const auto columns = image.columns();
  const auto rows    = image.rows();

  if (columns != rows) {
    std::cerr << "Error: Input image must be square" << std::endl;
    return 1;
  }

  const double      log_order = std::max(0.0, std::log2(columns));
  const std::size_t order     = (int) log_order;

  const size_t output_side_length = static_cast<std::size_t>(1) << order;

  // ordered from high to low
  std::vector<double> densities;
  densities.reserve(order + 1);

  for (std::size_t o = 0; o <= order; o++) {
    densities.emplace_back(calculate_density(o));
  }

  const double min_density = densities.back();
  const double max_density = densities.front();

  // adjust densities to be [0,1]
  for (auto& density : densities) {
    density = (density - min_density) / (max_density - min_density);
  }

  Grid<std::size_t> orders(output_side_length, output_side_length);

  // 'pixels' iterator goes row by column
  auto pixels = image.getPixels(0, 0, output_side_length, output_side_length);

  for (std::size_t y = 0; y < output_side_length; y++) {
    for (std::size_t x = 0; x < output_side_length; x++) {
      auto pixel = *pixels++;

      const double brightness = calculate_perceived_brightness(pixel.red / MaxRGBDouble, pixel.green / MaxRGBDouble, pixel.blue / MaxRGBDouble);

      std::size_t i = 0;
      for (; i < densities.size() - 1; i++) {
        if ((1 - brightness) >= 0.5 * (densities[i] + densities[i + 1])) {
          break;
        }
      }

      orders.set(x + output_side_length * y, i);
    }
  }

  Hilbert hilbert(order);
  hilbert.place_matrix(0, orders, 0, 0);

  const auto draw_bits = hilbert.draw();

  const auto padded_bits = pad(draw_bits);

  const auto image_grid = black_and_white(padded_bits);

  make_image_from_grid(image_grid, out_file_path);

  return 0;
}
