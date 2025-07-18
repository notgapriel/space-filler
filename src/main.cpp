#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>

#include <Magick++.h>

using namespace Magick;

using discrete_pixel_type = std::array<std::uint16_t, 3>;

template <std::size_t N_ = 0>
class Hilbert;

template <std::size_t N_>
struct hilbert_children {
  using type = std::array<std::unique_ptr<Hilbert<N_ - 1>>, 4>;
};

template <>
struct hilbert_children<0> {
  using type = std::nullptr_t;
};

template <std::size_t N_>
using hilbert_children_t = typename hilbert_children<N_>::type;

template <std::size_t N_>
class Hilbert {
private:
  using Self = Hilbert;

public:
  using children_type = hilbert_children_t<N_>;

  constexpr inline Hilbert(void) {}
  constexpr inline Hilbert(auto&& children) : _children(std::forward<decltype(children)>(children)) {}
  constexpr Hilbert(const Self&) = default;
  constexpr Hilbert(Self&&)      = default;

  template <std::size_t N2_ = N_>
  static constexpr inline std::size_t get_side_length(void) {
    if constexpr (N2_ == 0) {
      return 3;
    } else {
      // keeps all line lengths the same
      return ((get_side_length<N2_ - 1>() * 2) + 1);
    }
  }

  static constexpr inline const std::size_t SIDE_LENGTH = Self::get_side_length();

  using draw_bits_type = std::bitset<SIDE_LENGTH * SIDE_LENGTH>;

private:
  template <std::size_t Quadrant_>
  constexpr inline const void populate_child(auto& bits) const {
    using child_type      = Hilbert<N_ - 1>;
    using child_bits_type = typename child_type::draw_bits_type;

    child_bits_type child_bits;

    child_bits.reset();
    if (_children[Quadrant_]) {
      child_bits = _children[Quadrant_]->draw();
    } else {
      for (std::size_t x = 0; x < child_type::SIDE_LENGTH; x++) {
        child_bits.set(x);
      }

      for (std::size_t y = 1; y < child_type::SIDE_LENGTH; y++) {
        static constexpr const std::size_t X_OFFSETS[2] = { 0, child_type::SIDE_LENGTH - 1 };

        child_bits.set(X_OFFSETS[0] + y * (child_type::SIDE_LENGTH));
        child_bits.set(X_OFFSETS[1] + y * (child_type::SIDE_LENGTH));
      }
    }
    for (std::size_t x = 0; x < child_type::SIDE_LENGTH; x++) {
      for (std::size_t y = 0; y < child_type::SIDE_LENGTH; y++) {
        static constexpr const std::size_t X_OFFSET = Quadrant_ == 0 || Quadrant_ == 3 ? SIDE_LENGTH - child_type::SIDE_LENGTH : 0;
        static constexpr const std::size_t Y_OFFSET = Quadrant_ == 2 || Quadrant_ == 3 ? SIDE_LENGTH - child_type::SIDE_LENGTH : 0;

        if (!child_bits[x + child_type::SIDE_LENGTH * y]) continue;

        if constexpr (Quadrant_ == 0 || Quadrant_ == 1) {
          bits.set((x + X_OFFSET) + SIDE_LENGTH * (y + Y_OFFSET));
        } else if constexpr (Quadrant_ == 2) {
          bits.set(((child_type::SIDE_LENGTH - 1 - y) + X_OFFSET) + SIDE_LENGTH * (x + Y_OFFSET));
        } else if constexpr (Quadrant_ == 3) {
          bits.set((y + X_OFFSET) + SIDE_LENGTH * (x + Y_OFFSET));
        } else {
          static_assert(false, "Invalid quadrant supplied to function, must be [0,3]");
        }
      }
    }
  }

public:
  inline void draw(draw_bits_type& bits) const {
    bits.reset();

    if constexpr (N_ == 0) {
      for (std::size_t x = 0; x < SIDE_LENGTH; x++) {
        for (std::size_t y = 0; y < SIDE_LENGTH; y++) {
          if ((x == 0 || (x == SIDE_LENGTH - 1)) || (y == 0)) {
            bits.set(x + SIDE_LENGTH * y);
          }
        }
      }
    } else {
      using child_type           = Hilbert<N_ - 1>;
      using child_draw_bits_type = typename child_type::draw_bits_type;

      populate_child<0>(bits);
      populate_child<1>(bits);
      populate_child<2>(bits);
      populate_child<3>(bits);

      bits.set(0 + child_type::SIDE_LENGTH * SIDE_LENGTH, true);                             // left join
      bits.set(SIDE_LENGTH - 1 + child_type::SIDE_LENGTH * SIDE_LENGTH, true);               // right join
      bits.set(child_type::SIDE_LENGTH + (child_type::SIDE_LENGTH - 1) * SIDE_LENGTH, true); // top-middle join
    }
  }

  inline draw_bits_type draw(void) const {
    draw_bits_type out;
    draw(out);
    return out;
  }

  static constexpr inline Self make_full(void) {
    if constexpr (N_ == 0) {
      return Self();
    } else {
      using child_type = Hilbert<N_ - 1>;

      return Self(children_type{
        std::make_unique<child_type>(child_type::make_full()),
        std::make_unique<child_type>(child_type::make_full()),
        std::make_unique<child_type>(child_type::make_full()),
        std::make_unique<child_type>(child_type::make_full()),
      });
    }
  }

private:
  // 0 - top right
  // 1 - top left
  // 2 - bottom left
  // 3 - bottom right
  children_type _children;
};

template <std::size_t Rows_, std::size_t Columns_, std::size_t Pad_ = 1>
constexpr inline std::bitset<(Rows_ + Pad_ * 2) * (Columns_ + Pad_ * 2)> pad(const std::bitset<Rows_ * Columns_>& bits) {
  std::bitset<(Rows_ + Pad_ * 2) * (Columns_ + Pad_ * 2)> out;

  out.reset();
  for (std::size_t row = 0; row < Rows_; row++) {
    for (std::size_t column = 0; column < Columns_; column++) {
      if (bits[row * Columns_ + column]) out.set((row + Pad_) * (Columns_ + Pad_ * 2) + (column + Pad_));
    }
  }

  return out;
}

template <std::size_t Rows_, std::size_t Columns_>
static inline std::array<std::array<discrete_pixel_type, Rows_>, Columns_> black_and_white(const std::bitset<Rows_ * Columns_>& bits) {
  std::array<std::array<discrete_pixel_type, Rows_>, Columns_> out;

  for (std::size_t row = 0; row < Rows_; row++) {
    for (std::size_t column = 0; column < Columns_; column++) {
      out[row][column] = bits[row * Columns_ + column] ? discrete_pixel_type{ 0, 0, 0 } : discrete_pixel_type{ 65535, 65535, 65535 };
    }
  }

  return out;
}

template <std::size_t Rows_, std::size_t Columns_>
static inline void make_image_from_array(const std::array<std::array<discrete_pixel_type, Columns_>, Rows_>& matrix, const std::string& file_name) {
  Image image(Geometry(Columns_, Rows_), Color(0, 0, 0, 0));

  image.type(TrueColorType);
  image.modifyImage();

  // Allocate pixel view
  Pixels view(image);

  // auto pixels = image.setPixels(2, 2, 1, 3);
  auto pixels = view.get(0, 0, Columns_, Rows_);

  for (std::size_t row = 0; row < Rows_; row++) {
    for (std::size_t column = 0; column < Columns_; column++) {
      *pixels++ = Color(matrix[row][column][0], matrix[row][column][1], matrix[row][column][2]);
    }
  }

  view.sync();
  image.write(file_name);
}

int main(const int argc, const char *const *const argv) {
  InitializeMagick(nullptr);

  using hilbert_type = Hilbert<3>;

  hilbert_type hilbert = hilbert_type::make_full();

  auto draw_bits = hilbert.draw();

  auto padded_bits = pad<hilbert_type::SIDE_LENGTH, hilbert_type::SIDE_LENGTH, 1>(draw_bits);

  auto image_matrix = black_and_white<hilbert_type::SIDE_LENGTH + 2, hilbert_type::SIDE_LENGTH + 2>(padded_bits);

  make_image_from_array<hilbert_type::SIDE_LENGTH + 2, hilbert_type::SIDE_LENGTH + 2>(image_matrix, "matrix.png");

  return 0;
}
