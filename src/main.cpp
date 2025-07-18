#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <Magick++.h>

using namespace Magick;

using discrete_pixel_type = std::array<std::uint16_t, 3>;

template <std::size_t N_ = 0>
class Hilbert {
private:
  using Self = Hilbert;

public:
  template <std::size_t N2_ = N_>
  static constexpr inline std::size_t get_side_length(void) {
    if constexpr (N2_ == 0) {
      return 5;
    } else {
      // keeps all line lengths the same
      return ((get_side_length<N2_ - 1>() - 2) * 3);
    }
  }

  static constexpr inline const std::size_t SIDE_LENGTH = Self::get_side_length();

  using draw_bits_type = std::bitset<SIDE_LENGTH * SIDE_LENGTH>;

  inline void draw(draw_bits_type& bits) const {
    bits.reset();

    if constexpr (N_ == 0) {
      for (std::size_t x = 1; x < SIDE_LENGTH - 1; x++) {
        for (std::size_t y = 1; y < SIDE_LENGTH - 1; y++) {
          if ((x == 1 || (SIDE_LENGTH - x - 1 == 1)) || (y == 1)) {
            bits.set(x + SIDE_LENGTH * y);
          }
        }
      }
    } else {
      using child_type           = Hilbert<N_ - 1>;
      using child_draw_bits_type = typename child_type::draw_bits_type;

      child_type::SIDE_LENGTH;

      child_draw_bits_type child_bits;

      // TODO: use children
    }
  }
};

template <std::size_t Rows_, std::size_t Columns_>
static inline std::array<std::array<discrete_pixel_type, Rows_>, Columns_> black_and_white(const std::bitset<Rows_ * Columns_>& bits) {
  std::array<std::array<discrete_pixel_type, Rows_>, Columns_> out;

  for (std::size_t row = 0; row < Rows_; row++) {
    for (std::size_t column = 0; column < Columns_; column++) {
      out[row][column] = bits[row * Columns_ + column] ? discrete_pixel_type{ 0, 0, 0 } //  : discrete_pixel_type{255, 255, 255};
                                                       : discrete_pixel_type{ 65535, 65535, 65535 };
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

  Hilbert<0> hilbert;

  std::bitset<25> bits;

  hilbert.draw(bits);

  auto image_matrix = black_and_white<5, 5>(bits);

  make_image_from_array<5, 5>(image_matrix, "matrix.png");

  return -1;
}
