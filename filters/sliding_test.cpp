#include <gtest/gtest.h>

void fillRandRange(const uint32_t len,
                   const float lb,
                   const float ub, float* pArr);



TEST(sliding_test, reference) {

}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
