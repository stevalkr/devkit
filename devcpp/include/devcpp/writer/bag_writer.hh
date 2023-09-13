#pragma once

#include "devcpp/writer/writer.hh"

#include <rosbag/bag.h>
#include <rosbag/view.h>

namespace dev::writer {

class BagWriter : public Writer {
public:
  BagWriter(const std::string &bag_name_, mode mode_) {
    switch (mode_) {
    case mode::Write:
      bag_.open(bag_name_, rosbag::bagmode::Write);
      break;
    case mode::Append:
      bag_.open(bag_name_, rosbag::bagmode::Append);
      break;
    }
  }

  ~BagWriter() { bag_.close(); }

  void write(const std::string &topic, const auto &msg) {
    bag_.write(topic, ros::Time::now(), msg);
  }

  void write(const std::string &topic, const auto &msg, const t::time &time) {
    bag_.write(topic, time, msg);
  }

private:
  rosbag::Bag bag_;
};

} // namespace dev::writer

#include "doctest.h"
#include <std_msgs/String.h>
TEST_SUITE("testing bag_writer") {
  TEST_CASE("write") {
    std_msgs::String str;
    str.data = "test!";

    auto bag =
        dev::writer::BagWriter{"./test_bag.bag", dev::writer::mode::Write};
    bag.write("/string", str, t::time{12, 34});
  }
}
