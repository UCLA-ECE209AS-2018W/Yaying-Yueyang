#include "../dsp.h"
#include "../wavingz.h"

#include <random>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

#define CHECK_CLOSE_COLLECTION(aa, bb, tolerance) {     \
    using std::distance; \
    using std::begin; \
    using std::end; \
    auto a = begin(aa), ae = end(aa); \
    auto b = begin(bb); \
    BOOST_REQUIRE_EQUAL(distance(a, ae), distance(b, end(bb))); \
    for(; a != ae; ++a, ++b) { \
        BOOST_CHECK_CLOSE(*a, *b, tolerance); \
    } \
}

BOOST_AUTO_TEST_CASE(test_butter)
{
    // generated with octave signal package
    // > pkg load signal
    // > [b,a] = butter(6, 2*80000/2048000)
    double expected_gain = 2.18780328998614e-06;
    std::array<double, 7> expected_b({ 1, 6, 15, 20, 15, 6, 1 });
    std::array<double, 7> expected_a(
      { 1, -5.052163948341672, 10.699633740567215, -12.151435255115082,
        7.801326239249508, -2.683448745937741, 0.386227988988330 });

    double actual_gain;
    std::array<double, 7 > actual_b;
    std::array<double, 7 > actual_a;

    std::tie(actual_gain, actual_b, actual_a) = butter_lp<6>(2048000, 80000);

    BOOST_CHECK_CLOSE(expected_gain, actual_gain, 1e-12);
    CHECK_CLOSE_COLLECTION(expected_b, actual_b, 1e-12);
    CHECK_CLOSE_COLLECTION(expected_a, actual_a, 1e-12);
}

BOOST_AUTO_TEST_CASE(test_iir_filter)
{
    std::array<double, 7> signal = { 1.0 };
    // generated with octave signal package
    // >> pkg load signal;
    // >> [b,a] = butter(6, 2*40000/2048000);
    // >> v = impz(b, a, 7)
    std::array<double, 7> response = {
        4.24141395075581e-08, 4.88861571352154e-07, 2.79723456125130e-06,
        1.07425029331562e-05, 3.15672364704611e-05, 7.65594176229739e-05,
        1.60949149809997e-04
    };
    double gain;
    std::array<double, 7 > b;
    std::array<double, 7 > a;

    std::tie(gain, b, a) = butter_lp<6>(2048000, 40000);
    iir_filter<6> lp(gain, b, a);
    for(size_t counter = 0; counter != signal.size(); ++counter)
    {
        BOOST_CHECK_CLOSE(response[counter], lp(signal[counter]), 1e-12);
    }
}

BOOST_AUTO_TEST_CASE(test_encode_decode)
{

    std::vector<uint8_t> buffer = { 0xd2, 0xd6, 0x33, 0x22, 0xAA, 0x55, 13, 0xFF, 0x00, 0xFF, 0x00, 0x9f };
    buffer.push_back(wavingz::checksum(buffer.begin(), buffer.end()));

    bool called = false;
    auto wave_callback = [&](uint8_t* begin, uint8_t* end)
    {
        called = true;
        BOOST_REQUIRE((size_t)(end-begin) >= buffer.size()); // we may have some more noisy bytes in the end
        BOOST_REQUIRE(begin[6] == buffer.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(begin, begin+begin[6], buffer.begin(), buffer.end());
    };

    wavingz::demod::demod_nrz zwave(2048000, wave_callback);
    wavingz::encoder<int8_t> waver(2000000, 40000, 100);
    auto complex_bytes1 = waver(buffer.begin(), buffer.end(), 0.1);
    for(auto pair: complex_bytes1)
    {
        zwave(std::complex<double>(double(pair.first)/127.0, double(pair.second)/127.0));
    }
    BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(test_encode_decode_low_power)
{

    std::vector<uint8_t> buffer = { 0xd2, 0xd6, 0x33, 0x22, 0xAA, 0x55, 13, 0xFF, 0x00, 0xFF, 0x00, 0x9f };
    buffer.push_back(wavingz::checksum(buffer.begin(), buffer.end()));

    bool called = false;
    auto wave_callback = [&](uint8_t* begin, uint8_t* end)
    {
        called = true;
        BOOST_CHECK(end-begin >= 13); // we may have some more noisy bytes in the end
        BOOST_CHECK_EQUAL_COLLECTIONS(begin, begin+begin[6], buffer.begin(), buffer.end());
    };

    wavingz::demod::demod_nrz zwave(2048000, wave_callback);
    wavingz::encoder<int8_t> waver(2000000, 40000, 5);
    auto complex_bytes1 = waver(buffer.begin(), buffer.end(), 0.1);
    for(auto pair: complex_bytes1)
    {
        zwave(std::complex<double>(double(pair.first)/127.0, double(pair.second)/127.0));
    }
    BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(test_encode_decode_noise)
{

    std::vector<uint8_t> buffer = { 0xd2, 0xd6, 0x33, 0x22, 0xAA, 0x55, 13, 0xFF, 0x00, 0xFF, 0x00, 0x9f };
    buffer.push_back(wavingz::checksum(buffer.begin(), buffer.end()));

    bool called = false;
    auto wave_callback = [&](uint8_t* begin, uint8_t* end)
    {
        called = true;
        BOOST_CHECK(end-begin >= 13); // we may have some more noisy bytes in the end
        BOOST_CHECK_EQUAL_COLLECTIONS(begin, begin+begin[6], buffer.begin(), buffer.end());
    };

    wavingz::demod::demod_nrz zwave(2048000, wave_callback);
    wavingz::encoder<int8_t> waver(2000000, 40000, 100);
    auto complex_bytes1 = waver(buffer.begin(), buffer.end(), 0.1);

    std::default_random_engine g;
    std::normal_distribution<double> gaussian_noise(0.0, 1.0);

    for(auto pair: complex_bytes1)
    {
        zwave(std::complex<double>(0.1 * gaussian_noise(g) + 0.9 * double(pair.first)/127.0,
                                   0.1 * gaussian_noise(g) + 0.9 * double(pair.second)/127.0));
    }
    BOOST_CHECK(called);
}
