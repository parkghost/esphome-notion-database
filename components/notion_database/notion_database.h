#pragma once
/**
 * @file notion_database.h
 * @brief Header for Notion Database component.
 */

#include <set>
#include <sstream>  // add if not already included
#include <string>
#include <unordered_map>
#include <vector>

#include "allocator.h"
#include "esphome.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome {
namespace notion_database {

std::string tm_to_date(const std::tm &tm_time);
std::string tm_to_iso8601(const std::tm &tm_time);

enum class NotionPropertyType {
  TITLE,
  RICH_TEXT,
  NUMBER,
  DATE,
  CHECKBOX,
  SELECT,
  MULTI_SELECT,
  CREATED_BY,
  CREATED_TIME,
  EMAIL,
  FILES,
  FORMULA,
  LAST_EDITED_BY,
  LAST_EDITED_TIME,
  PEOPLE,
  PHONE_NUMBER,
  RELATION,
  ROLLUP,
  STATUS,
  URL,
  UNKNOWN
};

/**
 * @brief Represents a Notion property.
 */
struct NotionProperty {
  NotionPropertyType type;
  // Use these members depending on which type is active.
  std::string string_value;
  double number_value;
  bool bool_value;
  std::vector<std::string> vector_value;
  std::tm time_value;

  NotionProperty() : type(NotionPropertyType::UNKNOWN), number_value(0.0), bool_value(false) {
    // Initialize time_value to epoch (1970-01-01)
    std::memset(&time_value, 0, sizeof(time_value));
  }

  // Helper method to parse ISO8601 date strings into time_value
  bool parse_time_from_iso8601(const std::string &iso_time);
};

// Common Notion page properties
const static std::string NOTION_ID_KEY = "ID";
const static std::string NOTION_CREATED_TIME_KEY = "Created Time";
const static std::string NOTION_LAST_EDITED_TIME_KEY = "Last Edited Time";
const static std::string NOTION_ARCHIVED_KEY = "Archived";
const static std::string NOTION_IN_TRASH_KEY = "In Trash";

struct Page {
  std::vector<std::pair<uint32_t, NotionProperty>> properties;

  uint32_t hash_key(const std::string &key) const { return static_cast<uint32_t>(std::hash<std::string>{}(key)); }

  const NotionProperty *get_property(const std::string &key) const {
    uint32_t h = hash_key(key);
    for (const auto &kv : properties) {
      if (kv.first == h) return &kv.second;
    }
    return nullptr;
  }

  void set_property(const std::string &key, const NotionProperty &prop) {
    uint32_t h = hash_key(key);
    for (auto &kv : properties) {
      if (kv.first == h) {
        kv.second = prop;
        return;
      }
    }
    properties.push_back({h, prop});
  }
};

inline std::string notion_property_to_string(const NotionProperty &prop) {
  switch (prop.type) {
    case NotionPropertyType::TITLE:
    case NotionPropertyType::SELECT:
    case NotionPropertyType::EMAIL:
    case NotionPropertyType::PHONE_NUMBER:
    case NotionPropertyType::STATUS:
    case NotionPropertyType::URL:
      return prop.string_value;

    case NotionPropertyType::DATE:
      return tm_to_date(prop.time_value);

    case NotionPropertyType::CREATED_TIME:
    case NotionPropertyType::LAST_EDITED_TIME:
      return tm_to_iso8601(prop.time_value);

    case NotionPropertyType::RICH_TEXT: {
      std::ostringstream oss;
      for (const auto &text : prop.vector_value) {
        oss << text;
      }
      return oss.str();
    }
    case NotionPropertyType::NUMBER:
      return std::to_string(prop.number_value);
    case NotionPropertyType::CHECKBOX:
      return prop.bool_value ? "Y" : "N";
    case NotionPropertyType::MULTI_SELECT: {
      std::ostringstream oss;
      for (size_t i = 0; i < prop.vector_value.size(); ++i) {
        oss << prop.vector_value[i];
        if (i + 1 < prop.vector_value.size()) oss << ", ";
      }
      return oss.str();
    }
    default:
      return "UNKNOWN";
  }
}

inline std::string notion_property_type_to_string(NotionPropertyType type) {
  switch (type) {
    case NotionPropertyType::TITLE:
      return "TITLE";
    case NotionPropertyType::RICH_TEXT:
      return "RICH_TEXT";
    case NotionPropertyType::NUMBER:
      return "NUMBER";
    case NotionPropertyType::DATE:
      return "DATE";
    case NotionPropertyType::CHECKBOX:
      return "CHECKBOX";
    case NotionPropertyType::SELECT:
      return "SELECT";
    case NotionPropertyType::MULTI_SELECT:
      return "MULTI_SELECT";
    case NotionPropertyType::CREATED_BY:
      return "CREATED_BY";
    case NotionPropertyType::CREATED_TIME:
      return "CREATED_TIME";
    case NotionPropertyType::EMAIL:
      return "EMAIL";
    case NotionPropertyType::FILES:
      return "FILES";
    case NotionPropertyType::FORMULA:
      return "FORMULA";
    case NotionPropertyType::LAST_EDITED_BY:
      return "LAST_EDITED_BY";
    case NotionPropertyType::LAST_EDITED_TIME:
      return "LAST_EDITED_TIME";
    case NotionPropertyType::PEOPLE:
      return "PEOPLE";
    case NotionPropertyType::PHONE_NUMBER:
      return "PHONE_NUMBER";
    case NotionPropertyType::RELATION:
      return "RELATION";
    case NotionPropertyType::ROLLUP:
      return "ROLLUP";
    case NotionPropertyType::STATUS:
      return "STATUS";
    case NotionPropertyType::URL:
      return "URL";
    default:
      return "UNKNOWN";
  }
}

inline NotionPropertyType notion_property_type_from_string(const std::string &type_str) {
  if (type_str == "title") return NotionPropertyType::TITLE;
  if (type_str == "rich_text") return NotionPropertyType::RICH_TEXT;
  if (type_str == "number") return NotionPropertyType::NUMBER;
  if (type_str == "date") return NotionPropertyType::DATE;
  if (type_str == "checkbox") return NotionPropertyType::CHECKBOX;
  if (type_str == "select") return NotionPropertyType::SELECT;
  if (type_str == "multi_select") return NotionPropertyType::MULTI_SELECT;
  if (type_str == "created_by") return NotionPropertyType::CREATED_BY;
  if (type_str == "created_time") return NotionPropertyType::CREATED_TIME;
  if (type_str == "email") return NotionPropertyType::EMAIL;
  if (type_str == "files") return NotionPropertyType::FILES;
  if (type_str == "formula") return NotionPropertyType::FORMULA;
  if (type_str == "last_edited_by") return NotionPropertyType::LAST_EDITED_BY;
  if (type_str == "last_edited_time") return NotionPropertyType::LAST_EDITED_TIME;
  if (type_str == "people") return NotionPropertyType::PEOPLE;
  if (type_str == "phone_number") return NotionPropertyType::PHONE_NUMBER;
  if (type_str == "relation") return NotionPropertyType::RELATION;
  if (type_str == "rollup") return NotionPropertyType::ROLLUP;
  if (type_str == "status") return NotionPropertyType::STATUS;
  if (type_str == "url") return NotionPropertyType::URL;
  return NotionPropertyType::UNKNOWN;
}

// Add this before the NotionDatabase class definition

// Time comparison helpers for tm structures
inline bool operator==(const std::tm &lhs, const std::tm &rhs) {
  return lhs.tm_year == rhs.tm_year && lhs.tm_mon == rhs.tm_mon && lhs.tm_mday == rhs.tm_mday &&
         lhs.tm_hour == rhs.tm_hour && lhs.tm_min == rhs.tm_min && lhs.tm_sec == rhs.tm_sec;
}

inline bool operator!=(const std::tm &lhs, const std::tm &rhs) { return !(lhs == rhs); }

class NotionDatabase : public PollingComponent {
 public:
  // Returns the setup priority
  float get_setup_priority() const override;
  // Setup the component
  void setup() override;
  // Update the component
  void update() override;
  // Dump configuration
  void dump_config() override;

  // Sets the API token
  template <typename V>
  void set_api_token(V token) {
    api_token_ = token;
  }
  // Sets the database ID
  template <typename V>
  void set_database_id(V id) {
    database_id_ = id;
  }
  // Sets the query
  template <typename V>
  void set_query(V query) {
    query_ = query;
  }

  // Returns the on_page_change trigger
  Trigger<> *get_on_page_change_trigger() { return &this->on_page_change_trigger_; }

  // Sets the watchdog timeout
  template <typename V>
  void set_watchdog_timeout(V watchdog_timeout) {
    watchdog_timeout_ = watchdog_timeout;
  }

  // Sets the HTTP connect timeout
  template <typename V>
  void set_http_connect_timeout(V http_connect_timeout) {
    http_connect_timeout_ = http_connect_timeout;
  }

  // Sets the HTTP timeout
  template <typename V>
  void set_http_timeout(V http_timeout) {
    http_timeout_ = http_timeout;
  }

  // Sets the JSON parse buffer size
  template <typename V>
  void set_json_parse_buffer_size(V json_parse_buffer_size) {
    json_parse_buffer_size_ = json_parse_buffer_size;
  }

  // Returns the available properties
  const std::set<std::string> &get_available_properties() { return available_properties_; }
  // Returns the page count
  int get_page_count() const { return pages_.size(); }
  // Returns the has_page_change flag
  bool has_page_change() const { return has_page_change_flag_; }
  // Returns the pages
  const std::vector<Page, Allocator<Page>> &get_pages() const { return pages_; }

  // Adds a property filter
  void add_property_filter(const std::string &property_name) {
    if (this->property_filters_.find(property_name) != this->property_filters_.end()) {
      return;
    }

    this->property_filters_.insert(property_name);
    this->reset_state();
  }

  // Sets the property filters
  void set_property_filters(const std::set<std::string> &filters) {
    if (filters == this->property_filters_) {
      return;
    }

    this->property_filters_ = filters;
    this->reset_state();
  }

  // Fetches the first page
  void first_page();

  // Fetches the next page
  void next_page();

  // Fetches the previous page
  void previous_page();

  // Resets the state
  void reset_state();

 protected:
  TemplatableValue<std::string> api_token_;
  TemplatableValue<std::string> database_id_;
  TemplatableValue<std::string> query_;
  Trigger<> on_page_change_trigger_{};

  TemplatableValue<uint32_t> watchdog_timeout_;
  TemplatableValue<uint32_t> http_connect_timeout_;
  TemplatableValue<uint32_t> http_timeout_;
  TemplatableValue<uint32_t> json_parse_buffer_size_;

  std::set<std::string> available_properties_;
  std::set<std::string> property_filters_;
  std::vector<Page, Allocator<Page>> pages_;
  uint32_t pages_hash_ = 0;
  bool has_page_change_flag_{false};
  bool has_more_{false};
  std::string current_cursor_;
  std::string next_cursor_;
  std::vector<std::string> previous_cursors_;

  std::set<NotionPropertyType> supported_property_types_ = {
      NotionPropertyType::CREATED_TIME, NotionPropertyType::DATE,   NotionPropertyType::EMAIL,
      NotionPropertyType::MULTI_SELECT, NotionPropertyType::NUMBER, NotionPropertyType::PHONE_NUMBER,
      NotionPropertyType::RICH_TEXT,    NotionPropertyType::SELECT, NotionPropertyType::STATUS,
      NotionPropertyType::STATUS,       NotionPropertyType::TITLE,  NotionPropertyType::URL,
  };

  bool send_request_();
  bool add_pagination_cursor_to_query_(std::string &payload);
  uint32_t process_response_(Stream &stream, size_t content_size, std::vector<Page, Allocator<Page>> &new_pages);
  uint32_t parse_page_(const JsonObject &pageJson, Page &page);
  bool parse_basic_property_(const JsonObject &property_obj, Page &page, const std::string &property_name);
  bool validate_config_();
  void check_changes_(const std::vector<Page, Allocator<Page>> &new_pages, uint32_t new_pages_hash);
};

template <typename... Ts>
class FirstPageAction : public Action<Ts...> {
 public:
  explicit FirstPageAction(NotionDatabase *db) : db_(db) {}

  void play(Ts... x) override { this->db_->first_page(); }

 protected:
  NotionDatabase *db_;
};

template <typename... Ts>
class PreviousPageAction : public Action<Ts...> {
 public:
  explicit PreviousPageAction(NotionDatabase *db) : db_(db) {}

  void play(Ts... x) override { this->db_->previous_page(); }

 protected:
  NotionDatabase *db_;
};

template <typename... Ts>
class NextPageAction : public Action<Ts...> {
 public:
  explicit NextPageAction(NotionDatabase *db) : db_(db) {}

  void play(Ts... x) override { this->db_->next_page(); }

 protected:
  NotionDatabase *db_;
};


}  // namespace notion_database
}  // namespace esphome