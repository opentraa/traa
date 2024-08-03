#include <gtest/gtest.h>

#include "base/thread/callback.h"

#include <functional>
#include <memory>

class weak_callback_test : public testing::Test {
protected:
  void SetUp() override {
    flag_ = std::make_shared<traa::base::anonymous_flag>();
    callback_ =
        traa::base::weak_callback<std::function<int(int)>>(flag_, [](int x) { return x * 2; });
  }

  std::shared_ptr<traa::base::anonymous_flag> flag_;
  traa::base::weak_callback<std::function<int(int)>> callback_;
};

TEST_F(weak_callback_test, invoke) { EXPECT_EQ(callback_(5), 10); }

TEST_F(weak_callback_test, expired) {
  flag_.reset();
  EXPECT_EQ(callback_(5), 0);
}

TEST_F(weak_callback_test, is_expired) {
  EXPECT_FALSE(callback_.is_expired());
  flag_.reset();
  EXPECT_TRUE(callback_.is_expired());
}

TEST(support_weak_callback_test, to_weak_callback) {
  traa::base::support_weak_callback obj;
  auto callback = obj.to_weak_callback([](int x) { return x * 3; });
  EXPECT_EQ(callback(5), 15);
}

TEST(support_weak_callback_test, get_weak_flags) {
  traa::base::support_weak_callback obj1;
  traa::base::support_weak_callback obj2;
  auto flag1 = obj1.get_weak_flags();
  auto flag2 = obj2.get_weak_flags();
  EXPECT_EQ(flag1.use_count(), 1);
  EXPECT_EQ(flag2.use_count(), 1);
}

TEST(support_weak_callback_test, weak_callback_expired) {
  std::shared_ptr<traa::base::support_weak_callback> obj =
      std::make_shared<traa::base::support_weak_callback>();
  auto callback = obj->to_weak_callback([](int x) { return x * 3; });
  EXPECT_FALSE(callback.is_expired());
  EXPECT_EQ(callback(5), 15);
  obj.reset();
  EXPECT_TRUE(callback.is_expired());
  EXPECT_EQ(callback(5), 0);
}

TEST(weak_callback_flag_test, cancel_and_has_used) {
  traa::base::weak_callback_flag flag;
  EXPECT_FALSE(flag.has_used());
  auto callback = flag.to_weak_callback([]() {});
  EXPECT_TRUE(flag.has_used());

  flag.cancel();
  EXPECT_FALSE(flag.has_used());
}

TEST(make_weak_callback_test, non_member_function) {
  auto lambda_func = []() { return 4; };

  auto callback = traa::base::make_weak_callback(lambda_func);
  EXPECT_EQ(callback(), 4);

  auto lambda_func_with_params = [](int x) { return x * 4; };

  auto callback_2 = traa::base::make_weak_callback(lambda_func_with_params, std::placeholders::_1);
  EXPECT_EQ(callback_2(5), 20);

  auto callback_3 = traa::base::make_weak_callback(lambda_func_with_params, 5);
  EXPECT_EQ(callback_3(), 20);
}

TEST(make_weak_callback_test, const_member_function) {
  class TestClass : public traa::base::support_weak_callback {
  public:
    int multiply(int x) const { return x * 5; }
  };

  TestClass obj;
  auto callback = traa::base::make_weak_callback(&TestClass::multiply, &obj, std::placeholders::_1);
  EXPECT_EQ(callback(5), 25);

  auto callback_bind = traa::base::make_weak_callback(&TestClass::multiply, &obj, 5);
  EXPECT_EQ(callback_bind(), 25);
}

TEST(make_weak_callback_test, non_const_member_function) {
  class TestClass : public traa::base::support_weak_callback {
  public:
    int multiply(int x) { return x * 6; }
  };

  TestClass obj;
  auto callback = traa::base::make_weak_callback(&TestClass::multiply, &obj, std::placeholders::_1);
  EXPECT_EQ(callback(5), 30);

  auto callback_bind = traa::base::make_weak_callback(&TestClass::multiply, &obj, 5);
  EXPECT_EQ(callback_bind(), 30);
}