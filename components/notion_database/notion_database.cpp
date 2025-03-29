#include "notion_database.h"

#include <HTTPClient.h>

#include <algorithm>
#include <cctype>
#include <set>

#include "allocator.h"
#include "esphome/components/watchdog/watchdog.h"
#include "esphome/core/helpers.h"
#include "esphome/core/time.h"
#include "stream_monitor.h"

namespace esphome {
namespace notion_database {

static const char *const TAG = "notion_database";

std::string tm_to_date(const std::tm &tm_time) {
  char buffer[10];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm_time);
  return std::string(buffer);
}

std::string tm_to_iso8601(const std::tm &tm_time) {
  char buffer[25];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_time);
  return std::string(buffer);
}

// Setup priority
float NotionDatabase::get_setup_priority() const { return setup_priority::LATE; }

// Component setup
void NotionDatabase::setup() {}

// Periodic update
void NotionDatabase::update() {
  // Validate configuration before proceeding
  if (!validate_config_()) {
    ESP_LOGE(TAG, "Configuration validation failed");
    return;
  }

  // Send request and update status
  if (send_request_()) {
    this->status_clear_warning();
  } else {
    this->status_set_warning();
  }
}

// Debug configuration
void NotionDatabase::dump_config() {
  ESP_LOGCONFIG(TAG, "Notion Database:");
  ESP_LOGCONFIG(TAG, "  API Token: %s", api_token_.value().empty() ? "not set" : "set");
  ESP_LOGCONFIG(TAG, "  Database ID: %s", database_id_.value().c_str());
  ESP_LOGCONFIG(TAG, "  Query: %s", query_.value().c_str());
  ESP_LOGCONFIG(TAG, "  Watchdog Timeout: %u", watchdog_timeout_.value());
  ESP_LOGCONFIG(TAG, "  Verify SSL: %s", verify_ssl_.value() ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  HTTP Connect Timeout: %u", http_connect_timeout_.value());
  ESP_LOGCONFIG(TAG, "  HTTP Timeout: %u", http_timeout_.value());
  ESP_LOGCONFIG(TAG, "  JSON Parser Buffer Size: %u", json_parse_buffer_size_.value());
  ESP_LOGCONFIG(TAG, "  Supported Property Types:");
  for (const auto &type : supported_property_types_) {
    ESP_LOGCONFIG(TAG, "    - %s", notion_property_type_to_string(type).c_str());
  }
  LOG_UPDATE_INTERVAL(this);
}

// Validate configuration
bool NotionDatabase::validate_config_() {
  if (api_token_.value().empty()) {
    ESP_LOGE(TAG, "API token not set");
    return false;
  }

  if (database_id_.value().empty()) {
    ESP_LOGE(TAG, "Database ID not set");
    return false;
  }

  if (!query_.value().empty()) {
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, query_.value()) != DeserializationError::Ok) {
      ESP_LOGE(TAG, "Invalid JSON query");
      return false;
    }
  }
  return true;
}

// Send HTTP request
bool NotionDatabase::send_request_() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    ESP_LOGW(TAG, "WiFi not connected");
    return false;
  }

  watchdog::WatchdogManager wdm(this->watchdog_timeout_.value());
  std::string url = "https://api.notion.com/v1/databases/" + database_id_.value() + "/query";

  HTTPClient http;
  // Disable SSL verification if configured
  if (!verify_ssl_.value()) {
    client_.setInsecure();
  }
  http.begin(client_, url.c_str());
  http.useHTTP10(true);
  http.setConnectTimeout(http_connect_timeout_.value());
  http.setTimeout(http_timeout_.value());
  http.addHeader("Authorization", ("Bearer " + api_token_.value()).c_str());
  http.addHeader("Notion-Version", "2022-06-28");
  http.addHeader("Content-Type", "application/json");

  std::string payload = query_.value();
  if (!current_cursor_.empty()) {
    if (!add_pagination_cursor_to_query_(payload)) {
      ESP_LOGE(TAG, "Failed to add pagination cursor to query");
      return false;
    }
  }
  ESP_LOGD(TAG, "Sending query: %s", payload.c_str());

  ESP_LOGD(TAG, "Free heap(internal) before request: %u", ESP.getFreeHeap());
  App.feed_wdt();
  int http_code = http.POST(payload.c_str());
  ESP_LOGD(TAG, "Free heap(internal) after request: %u", ESP.getFreeHeap());

  // Process successful HTTP response
  if (http_code == HTTP_CODE_OK) {
    App.feed_wdt();
    std::vector<Page, Allocator<Page>> new_pages;
    uint32_t new_pages_hash = process_response_(http.getStream(), http.getSize(), new_pages);
    http.end();
    // Check for changes if parsing was successful
    if (new_pages_hash != 0) {
      check_changes_(new_pages, new_pages_hash);
    }
    ESP_LOGD(TAG, "Free heap(internal) after parse json: %u", ESP.getFreeHeap());
    return true;
  } else {
    // Handle HTTP request failure
    ESP_LOGE(TAG, "HTTP request failed, code: %d, error: %s", http_code, http.getString().c_str());
    http.end();
    return false;
  }
}

bool NotionDatabase::add_pagination_cursor_to_query_(std::string &payload) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, payload) != DeserializationError::Ok) {
    ESP_LOGE(TAG, "Failed to parse query JSON for adding pagination cursor");
    return false;
  }

  doc["start_cursor"] = current_cursor_;

  payload.clear();
  serializeJson(doc, payload);
  return true;
}

struct JsonAllocator {
  void *allocate(size_t n) { return ALLOCATOR.allocate(n); }

  void deallocate(void *p) { ALLOCATOR.deallocate(static_cast<uint8_t *>(p), 0); }

  void *reallocate(void *p, size_t new_size) {
    return ALLOCATOR.reallocate(static_cast<uint8_t *>(p), new_size, MALLOC_CAP_SPIRAM);
  }
};

typedef BasicJsonDocument<JsonAllocator> CustomDynamicJsonDocument;

// Process HTTP response
uint32_t NotionDatabase::process_response_(WiFiClient &stream, size_t content_size,
                                           std::vector<Page, Allocator<Page>> &new_pages) {
  StreamMonitor stream_monitor(stream);

  auto free_heap = ALLOCATOR.get_max_free_block_size();
  ESP_LOGD(TAG, "Content Size: %d, Free heap: %u, JSON Parse Buffer Size: %u", content_size, free_heap,
           json_parse_buffer_size_.value());

  size_t doc_size =
      std::min({static_cast<size_t>(free_heap - 2048), static_cast<size_t>(json_parse_buffer_size_.value()),
                content_size > 0 ? static_cast<size_t>(content_size * 1.5f) : SIZE_MAX});

  ESP_LOGD(TAG, "Allocating %u bytes for JSON document", doc_size);

  if (doc_size < json_parse_buffer_size_.value() / 2) {
    ESP_LOGW(TAG,
             "Calculated JSON document size is significantly smaller than configured buffer size. "
             "Consider reducing json_parse_buffer_size or increasing available heap.");
  }

  CustomDynamicJsonDocument doc(doc_size);
  DeserializationError error = deserializeJson(doc, stream_monitor);
  ESP_LOGD(TAG, "Stream read bytes: %u", stream_monitor.get_bytes_read());
  if (error) {
    ESP_LOGE(TAG, "JSON parsing failed: %s", error.c_str());
    doc.clear();
    return 0;
  }

  JsonArray results = doc["results"];
  ESP_LOGD(TAG, "Processing %u results", results.size());
  new_pages.reserve(results.size());

  uint32_t pages_hash = 17;
  int i = 0;
  for (JsonObject result : results) {
    Page page;
    pages_hash = pages_hash * 31 + parse_page_(result, page);
    new_pages.push_back(std::move(page));
    App.feed_wdt();
#if ESP_LOG_LEVEL >= ESP_LOG_VERBOSE
    ESP_LOGV(TAG, "Free heap(internal) after parse_page %d: %u", ++i, ESP.getFreeHeap());
#endif
  }

  has_more_ = doc["has_more"] | false;
  if (has_more_) {
    std::string new_next_currsor = doc["next_cursor"] | "";
    if (new_next_currsor != next_cursor_) {
      current_cursor_ = next_cursor_;
      next_cursor_ = new_next_currsor;
    }
  }

  ESP_LOGD(TAG, "Parsed %zu Pages", new_pages.size());
  if (!current_cursor_.empty()) {
    ESP_LOGD(TAG, "Pagination: Currnet cursor: %s", current_cursor_.c_str());
  }
  ESP_LOGD(TAG, "Pagination: Has more: %s", has_more_ ? "true" : "false");
  if (has_more_) {
    ESP_LOGD(TAG, "Pagination: Next cursor: %s", next_cursor_.c_str());
  }
  doc.clear();
  stream.stop();
  return pages_hash;
}

bool NotionDatabase::parse_basic_property_(const JsonObject &property_obj, Page &page,
                                           const std::string &property_name) {
  available_properties_.insert(property_name);

  if (!property_filters_.empty() && property_filters_.find(property_name) == property_filters_.end()) {
    return false;
  }

  if (property_name == NOTION_ID_KEY) {
    NotionProperty prop;
    prop.type = NotionPropertyType::TITLE;
    prop.string_value = property_obj["id"] | "unknown_id";
    page.set_property(NOTION_ID_KEY, prop);
    return true;
  }

  if (property_name == NOTION_LAST_EDITED_TIME_KEY) {
    NotionProperty prop;
    prop.type = NotionPropertyType::LAST_EDITED_TIME;
    std::string last_edited_time_str = property_obj["last_edited_time"] | "1970-01-01T00:00:00Z";
    prop.parse_time_from_iso8601(last_edited_time_str);
    page.set_property(NOTION_LAST_EDITED_TIME_KEY, prop);
    return true;
  }

  if (property_name == NOTION_CREATED_TIME_KEY) {
    NotionProperty prop;
    prop.type = NotionPropertyType::CREATED_TIME;
    std::string created_time_str = property_obj["created_time"] | "1970-01-01T00:00:00Z";
    prop.parse_time_from_iso8601(created_time_str);
    page.set_property(NOTION_CREATED_TIME_KEY, prop);
    return true;
  }

  if (property_name == NOTION_ARCHIVED_KEY) {
    NotionProperty archived_prop;
    archived_prop.type = NotionPropertyType::CHECKBOX;
    archived_prop.bool_value = property_obj["archived"] | false;
    page.set_property(NOTION_ARCHIVED_KEY, archived_prop);
    return true;
  }

  if (property_name == NOTION_IN_TRASH_KEY) {
    NotionProperty prop;
    prop.type = NotionPropertyType::CHECKBOX;
    prop.bool_value = property_obj["in_trash"] | false;
    page.set_property(NOTION_IN_TRASH_KEY, prop);
    return true;
  }

  return false;
}

// Parse individual page
uint32_t NotionDatabase::parse_page_(const JsonObject &pageJson, Page &page) {
  parse_basic_property_(pageJson, page, NOTION_ID_KEY);
  parse_basic_property_(pageJson, page, NOTION_CREATED_TIME_KEY);
  parse_basic_property_(pageJson, page, NOTION_LAST_EDITED_TIME_KEY);
  parse_basic_property_(pageJson, page, NOTION_ARCHIVED_KEY);
  parse_basic_property_(pageJson, page, NOTION_IN_TRASH_KEY);

  JsonObject properties = pageJson["properties"].as<JsonObject>();
  for (JsonPair kv : properties) {
    std::string key = kv.key().c_str();
    JsonObject prop_obj = kv.value().as<JsonObject>();
    std::string type_str = prop_obj["type"] | "";
    NotionPropertyType np = notion_property_type_from_string(type_str);

    if (!supported_property_types_.count(np)) {
      continue;
    }
    available_properties_.insert(key);

    if (!property_filters_.empty() && property_filters_.find(key) == property_filters_.end()) {
      continue;
    }

    NotionProperty property;
    property.type = np;

    switch (np) {
      case NotionPropertyType::TITLE: {
        std::string temp_str;
        JsonArray title_arr = prop_obj["title"].as<JsonArray>();
        for (JsonObject text_obj : title_arr) {
          temp_str += (const char *)(text_obj["plain_text"] | "");
        }
        property.string_value = std::move(temp_str);
        break;
      }

      case NotionPropertyType::RICH_TEXT: {
        std::vector<std::string> texts;
        JsonArray text_arr = prop_obj["rich_text"].as<JsonArray>();
        for (JsonObject text_obj : text_arr) {
          texts.push_back((const char *)(text_obj["plain_text"] | ""));
        }
        property.vector_value = texts;
        break;
      }

      case NotionPropertyType::NUMBER: {
        if (!prop_obj["number"].isNull()) {
          property.number_value = prop_obj["number"].as<double>();
        } else {
          property.number_value = 0.0;
        }
        break;
      }

      case NotionPropertyType::DATE: {
        JsonObject date_obj = prop_obj["date"].as<JsonObject>();
        std::string temp_str = date_obj["start"] | "";
        property.parse_time_from_iso8601(temp_str);
        break;
      }

      case NotionPropertyType::CHECKBOX: {
        property.bool_value = prop_obj["checkbox"] | false;
        break;
      }

      case NotionPropertyType::SELECT: {
        JsonObject select_obj = prop_obj["select"].as<JsonObject>();
        property.string_value = (!select_obj.isNull() && select_obj.containsKey("name"))
                                    ? std::string(select_obj["name"] | "")
                                    : std::string("");
        break;
      }

      case NotionPropertyType::MULTI_SELECT: {
        std::vector<std::string> names;
        JsonArray ms_array = prop_obj["multi_select"].as<JsonArray>();
        for (JsonObject ms_obj : ms_array) {
          names.push_back((const char *)(ms_obj["name"] | ""));
        }
        property.vector_value = names;
        break;
      }

      case NotionPropertyType::CREATED_TIME: {
        std::string temp_str = std::string(prop_obj["created_time"] | "");
        property.parse_time_from_iso8601(temp_str);
        break;
      }

      case NotionPropertyType::EMAIL: {
        property.string_value = std::string(prop_obj["email"] | "");
        break;
      }

      case NotionPropertyType::LAST_EDITED_TIME: {
        std::string temp_str = std::string(prop_obj["last_edited_time"] | "");
        property.parse_time_from_iso8601(temp_str);
        break;
      }

      case NotionPropertyType::PHONE_NUMBER: {
        property.string_value = std::string(prop_obj["phone_number"] | "");
        break;
      }

      case NotionPropertyType::STATUS: {
        JsonObject status_obj = prop_obj["status"].as<JsonObject>();
        property.string_value = std::string(status_obj["name"] | "");
        break;
      }

      case NotionPropertyType::URL: {
        property.string_value = std::string(prop_obj["url"] | "");
        break;
      }

      default: {
        property.type = NotionPropertyType::UNKNOWN;
        break;
      }
    }

    page.set_property(key, property);
  }

#if ESP_LOG_LEVEL >= ESP_LOG_VERBOSE
  ESP_LOGV(TAG, "Database Page:");
  for (const auto &propertyName : available_properties_) {
    const NotionProperty *prop = page.get_property(propertyName);
    if (prop != nullptr) {
      ESP_LOGV(TAG, "  property: %s", propertyName.c_str());
      ESP_LOGV(TAG, "    type: %s", notion_property_type_to_string(prop->type).c_str());
      ESP_LOGV(TAG, "    value: %s", notion_property_to_string(*prop).c_str());
    }
  }
#endif

  std::string content =
      std::string(pageJson["id"].as<const char *>()) + pageJson["last_edited_time"].as<const char *>();
  return std::hash<std::string>{}(content);
}

// Check for page changes
void NotionDatabase::check_changes_(const std::vector<Page, Allocator<Page>> &new_pages, uint32_t new_pages_hash) {
  ESP_LOGD(TAG, "Previous pages hash: %u", pages_hash_);
  ESP_LOGD(TAG, "New pages hash: %u", new_pages_hash);

  // Compare new hash with previous hash
  if (pages_hash_ != new_pages_hash) {
    pages_ = std::move(new_pages);
    pages_hash_ = new_pages_hash;
    has_page_change_flag_ = true;
    ESP_LOGI(TAG, "Detected page changes, current count: %zu", pages_.size());
    on_page_change_trigger_.trigger();
  } else {
    // No changes detected
    has_page_change_flag_ = false;
    ESP_LOGD(TAG, "No page changes");
  }
}

void NotionDatabase::first_page() {
  ESP_LOGI(TAG, "Fetching first page");
  reset_state();
  previous_cursors_.clear();
  update();
}

void NotionDatabase::next_page() {
  ESP_LOGI(TAG, "Fetching next page");
  if (has_more_) {
    previous_cursors_.push_back(current_cursor_);
    current_cursor_ = next_cursor_;
    update();
  } else {
    ESP_LOGD(TAG, "No more pages available");
  }
}

void NotionDatabase::previous_page() {
  ESP_LOGI(TAG, "Fetching previous page");
  if (!previous_cursors_.empty()) {
    current_cursor_ = previous_cursors_.back();
    previous_cursors_.pop_back();
    update();
  } else {
    ESP_LOGD(TAG, "No previous page available");
  }
}

void NotionDatabase::reset_state() {
  pages_hash_ = 0;
  has_page_change_flag_ = false;
  pages_.clear();
  available_properties_.clear();
  has_more_ = false;
  current_cursor_ = "";
  next_cursor_ = "";
  previous_cursors_.clear();
}

// Parse ISO8601 date strings
bool NotionProperty::parse_time_from_iso8601(const std::string &iso_time) {
  if (iso_time.empty()) return false;

  std::memset(&time_value, 0, sizeof(time_value));

  int year, month, day, hour = 0, min = 0, sec = 0;

  int matched;
  if (iso_time.find('T') != std::string::npos) {
    matched = sscanf(iso_time.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec);
    if (matched < 3) return false;
  } else {
    matched = sscanf(iso_time.c_str(), "%d-%d-%d", &year, &month, &day);
    if (matched != 3) return false;
  }

  time_value.tm_year = year - 1900;
  time_value.tm_mon = month - 1;
  time_value.tm_mday = day;
  time_value.tm_hour = hour;
  time_value.tm_min = min;
  time_value.tm_sec = sec;

  int32_t tz_offset = esphome::ESPTime::timezone_offset();

  time_t epoch_time = mktime(&time_value);

  epoch_time += tz_offset;

  time_value = *localtime(&epoch_time);

  return true;
}

}  // namespace notion_database
}  // namespace esphome
