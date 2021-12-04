/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_TEST_RANDOM_RANDOMINTEGER_H
#define RODIN_TEST_RANDOM_RANDOMINTEGER_H

#include <random>
#include <limits>
#include <cassert>
#include <type_traits>

namespace Rodin::Test::Random
{
  template <class T = int>
  class RandomInteger
  {
    static_assert(std::is_integral<T>::value,
        "Template parameter T must be an integral type");
    public:
      RandomInteger(
          T a = std::numeric_limits<T>::min(),
          T b = std::numeric_limits<T>::max(),
          unsigned int seed = std::random_device()())
        : m_distrib(a, b), m_seed(seed)
      {
        assert(a <= b);
      }

      RandomInteger& setSeed(unsigned int seed)
      {
        m_gen.seed(seed);
      }

      unsigned int getSeed() const
      {
        return m_seed;
      }

      T operator()()
      {
        return m_distrib(m_gen);
      }

    private:
      std::mt19937 m_gen;
      std::uniform_int_distribution<T> m_distrib;
      unsigned int m_seed;
  };
}

#endif
