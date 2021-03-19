#include "test_runner.h"

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

using namespace std;

struct Record {
  string id;
  string title;
  string user;
  int timestamp;
  int karma;
};

class Database {
 public:
  bool Put(const Record& record) {
    if (GetById(record.id))
	  return false;
	data_[record.id] = record;
	data_by_user_.insert({record.user, &data_[record.id]});
	data_by_timestamp_.insert({record.timestamp, &data_[record.id]});
	data_by_karma_.insert({record.karma, &data_[record.id]});
	return true;
  }

  const Record* GetById(const string& id) const {
	if (data_.find(id) == data_.end())
	  return nullptr;
	return &data_.at(id);
  }

  void RemoveFromUserBase(const Record& record) {
	auto range = data_by_user_.equal_range(record.user);
	for (auto it = range.first; it != range.second; it++)
	  if (it->second->user == record.user) {
		data_by_user_.erase(it);
		break;
	  }
  }

  void RemoveFromTimestampBase(const Record& record) {
	auto range = data_by_timestamp_.equal_range(record.timestamp);
	for (auto it = range.first; it != range.second; it++)
	  if (it->second->timestamp == record.timestamp) {
		data_by_timestamp_.erase(it);
		break;
	  }
  }

  void RemoveFromKarmaBase(const Record& record) {
	auto range = data_by_karma_.equal_range(record.karma);
	for (auto it = range.first; it != range.second; it++)
	  if (it->second->karma == record.karma) {
		data_by_karma_.erase(it);
		break;
	  }
  }

  bool Erase(const string& id) {
    if (!GetById(id))
	  return false;
    Record record = data_.extract(id).mapped();
	RemoveFromUserBase(record);
	RemoveFromTimestampBase(record);
	RemoveFromKarmaBase(record);
    return true;
  }

  template <typename Callback>
  void RangeByTimestamp(int low, int high, Callback callback) const {
    auto range_begin = data_by_timestamp_.lower_bound(low);
    auto range_end = data_by_timestamp_.upper_bound(high);
    CallRange(range_begin, range_end, callback);
  }

  template <typename Callback>
  void RangeByKarma(int low, int high, Callback callback) const {
	auto range_begin = data_by_karma_.lower_bound(low);
	auto range_end = data_by_karma_.upper_bound(high);
	CallRange(range_begin, range_end, callback);
  }

  template <typename Callback>
  void AllByUser(const string& user, Callback callback) const {
	auto range_begin = data_by_user_.begin(), range_end = data_by_user_.begin();
	tie(range_begin, range_end) = data_by_user_.equal_range(user);
	CallRange(range_begin, range_end, callback);
  }

  template<typename BiDirectionalIterator, typename Callback>
  void CallRange(BiDirectionalIterator range_begin, BiDirectionalIterator range_end, Callback callback) const {
    for (auto it = range_begin; it != range_end; it++) {
      bool con = callback(*(it->second));
      if (!con)
        break;
    }
  }
 private:
  unordered_map<string, Record> data_;
  multimap<string, Record*> data_by_user_;
  multimap<int, Record*> data_by_timestamp_;
  multimap<int, Record*> data_by_karma_;
};

void TestRangeBoundaries() {
  const int good_karma = 1000;
  const int bad_karma = -10;

  Database db;
  db.Put({"id1", "Hello there", "master", 1536107260, good_karma});
  db.Put({"id2", "O>>-<", "general2", 1536107260, bad_karma});

  int count = 0;
  db.RangeByKarma(bad_karma, good_karma, [&count](const Record&) {
	++count;
	return true;
  });

  ASSERT_EQUAL(2, count);
}

void TestSameUser() {
  Database db;
  db.Put({"id1", "Don't sell", "master", 1536107260, 1000});
  db.Put({"id2", "Rethink life", "master", 1536107260, 2000});

  int count = 0;
  db.AllByUser("master", [&count](const Record&) {
	++count;
	return true;
  });

  ASSERT_EQUAL(2, count);
}

void TestReplacement() {
  const string final_body = "Feeling sad";

  Database db;
  db.Put({"id", "Have a hand", "not-master", 1536107260, 10});
  db.Erase("id");
  db.Put({"id", final_body, "not-master", 1536107260, -10});

  auto record = db.GetById("id");
  ASSERT(record != nullptr);
  ASSERT_EQUAL(final_body, record->title);
}

int main() {
  TestRunner tr;
  RUN_TEST(tr, TestRangeBoundaries);
  RUN_TEST(tr, TestSameUser);
  RUN_TEST(tr, TestReplacement);
  return 0;
}