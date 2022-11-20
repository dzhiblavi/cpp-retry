#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include <retry/retry.hpp>

namespace test {

struct FailCall {
  explicit FailCall(int num_fails) : num_fails(num_fails) {}

  bool operator()() const noexcept(false) {
    if (num_fails > 0) {
      --num_fails;
      throw std::runtime_error(std::to_string(num_fails) + " fails left");
    }
    return true;
  }

  mutable int num_fails;
};

struct CounterHandler : public retry::error_handler<CounterHandler> {
  explicit CounterHandler(int* counter) : counter_(counter) {}
  void handle(std::exception_ptr&) const { ++(*counter_); }

  int* counter_;
};

struct FlagFinalHandler : public retry::final_error_handler<FlagFinalHandler> {
  template <std::same_as<bool> R>
  R handle(std::exception_ptr&) const {
    called = true;
    return false;
  }

  mutable bool called = false;
};

}  //namespace test

TEST(retry, ok_single_call) {
  bool x = false;
  retry::run([&x] { x = true; return true; }, 0, retry::rethrow_last_exception());
  EXPECT_EQ(true, x);
}

TEST(retry, zero_retries) {
  test::FailCall fc(1);
  EXPECT_EQ(false, retry::run(fc, 0, retry::return_default_value(false)));
}

TEST(retry, default_enough) {
  test::FailCall fc(5);
  EXPECT_EQ(true, retry::run(fc, 5, retry::return_default_value(false)));
}

TEST(retry, default_not_enough) {
  test::FailCall fc(5);
  EXPECT_EQ(false, retry::run(fc, 4, retry::return_default_value(false)));
}

TEST(retry, retrow_enough) {
  test::FailCall fc(5);
  EXPECT_EQ(true, retry::run(fc, 5, retry::rethrow_last_exception()));
}

TEST(retry, rethrow_not_enough) {
  test::FailCall fc(5);
  ASSERT_THROW(retry::run(fc, 4, retry::rethrow_last_exception()), std::runtime_error);
}

TEST(retry, wait_interval) {
  test::FailCall fc(5);
  EXPECT_EQ(
      false,
      retry::run(
        fc, 4,
        retry::wait_interval(std::chrono::milliseconds(10)),
        retry::return_default_value(false)));
}

TEST(retry, custom_error_handler_enough) {
  test::FailCall fc(5);
  int counter = 0;
  EXPECT_EQ(
      true,
      retry::run(fc, 5, test::CounterHandler(&counter), retry::return_default_value(false)));
  EXPECT_EQ(5, counter);
}

TEST(retry, custom_error_handler_not_enough) {
  test::FailCall fc(5);
  int counter = 0;
  EXPECT_EQ(
      false,
      retry::run(fc, 4, test::CounterHandler(&counter), retry::return_default_value(false)));
  EXPECT_EQ(4, counter);
}

TEST(retry, custom_final_error_handler_enough) {
  test::FailCall fc(5);
  test::FlagFinalHandler custom_final_handler;
  EXPECT_EQ(true, retry::run(fc, 5, custom_final_handler));
  EXPECT_EQ(false, custom_final_handler.called);
}

TEST(retry, custom_final_error_handler_not_enough) {
  test::FailCall fc(5);
  test::FlagFinalHandler custom_final_handler;
  EXPECT_EQ(false, retry::run(fc, 4, custom_final_handler));
  EXPECT_EQ(true, custom_final_handler.called);
}

