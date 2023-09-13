#pragma once

#include "devcpp/reader/reader.hh"

#include <rosbag/bag.h>
#include <rosbag/view.h>

namespace dev::reader {

class BagReader : public Reader {
public:
  using iterator = rosbag::View::iterator;

  BagReader(const std::string &bag_name_) {
    bag_.open(bag_name_, rosbag::bagmode::Read);
    bag_view_ = std::make_unique<rosbag::View>(bag_);
  }

  ~BagReader() {
    bag_view_.reset();
    bag_.close();
  }

  iterator begin() { return bag_view_->begin(); }
  iterator end() { return bag_view_->end(); }

  t::time get_begin_time() const { return t::time{bag_view_->getBeginTime()}; }
  t::time get_end_time() const { return t::time{bag_view_->getEndTime()}; }

private:
  rosbag::Bag bag_;
  std::unique_ptr<rosbag::View> bag_view_;
};

} // namespace dev::reader

#include "doctest.h"
#include <std_msgs/String.h>
TEST_SUITE("testing bag_reader") {
  TEST_CASE("read") {
    auto bag = dev::reader::BagReader("./test_bag.bag");

    for (const auto &m : bag) {
      CHECK(m.getTopic() == "/string");
      std_msgs::String::ConstPtr msg = m.instantiate<std_msgs::String>();
      CHECK(msg->data == "test!");
    }
  }
}
