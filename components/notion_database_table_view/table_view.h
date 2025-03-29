#pragma once
#include <string>
#include <vector>

#include "esphome/components/display/display.h"
#include "esphome/components/notion_database/allocator.h"
#include "esphome/components/notion_database/notion_database.h"

namespace esphome {
namespace notion_database {
class Page;
class NotionDatabase;

enum class TextOverflow { ELLIPSIS, CLIP };

class NotionDatabaseTableView : public Component {
 public:
  // Sets the line height for the table view
  template <typename T>
  void set_line_height(const T &h) {
    this->line_height_ = h;
  }

  // Enables or disables the header row
  template <typename T>
  void set_enable_header(const T &enable) {
    enable_header_ = enable;
  }

  // Enables or disables the title
  template <typename T>
  void set_enable_title(const T &enable) {
    enable_title_ = enable;
  }

  // Sets the title text
  template <typename T>
  void set_title(const T &title) {
    title_ = title;
  }

  // Enables or disables grid lines
  template <typename T>
  void set_enable_grid_line(const T &enable) {
    enable_grid_line_ = enable;
  }

  // Sets the text overflow behavior
  template <typename T>
  void set_text_overflow(const T &overflow) {
    text_overflow_ = overflow;
  }

  // Inverts the title color
  template <typename T>
  void set_invert_title_color(const T &invert) {
    invert_title_color_ = invert;
  }

  // Inverts the header color
  template <typename T>
  void set_invert_header_color(const T &invert) {
    invert_header_color_ = invert;
  }

  // Sets the date format
  template <typename T>
  void set_date_format(const T &format) {
    this->date_format_ = format;
  }

  // Sets the datetime format
  template <typename T>
  void set_datetime_format(const T &format) {
    this->datetime_format_ = format;
  }

  // Sets the list style type
  template <typename T>
  void set_list_style_type(const T &list_style_type) {
    this->list_style_type_ = list_style_type;
  }

  // Enables or disables list style
  template <typename T>
  void set_enable_list_style(const T &enable) {
    this->enable_list_style_ = enable;
  }

  // Sets the parent database
  void set_database_parent(NotionDatabase *database) { this->database_parent_ = database; }

  // Draws the table on the display
  void draw(display::Display &it, int x, int y, int width, int height, font::Font *font, Color color_on,
                  Color color_off);

  // Adds a column to the table
  void add_column(const std::string &column) {
    auto trimmed_column = column;
    trimmed_column.erase(0, trimmed_column.find_first_not_of(" \t\n\r"));
    trimmed_column.erase(trimmed_column.find_last_not_of(" \t\n\r") + 1);
    if (!trimmed_column.empty()) {
      this->columns_.push_back(column);
    }
  }

  // Adds a column width
  void add_column_width(int width) { this->column_widths_.push_back(width); }

  // Sets the columns
  void set_columns(const std::vector<std::string> &columns) { this->columns_ = columns; }

  // Sets the column widths
  void set_column_widths(const std::vector<int> &widths) { this->column_widths_ = widths; }

 protected:
  NotionDatabase *database_parent_;  // Parent database

  TemplatableValue<int> line_height_;
  TemplatableValue<bool> enable_grid_line_;
  TemplatableValue<bool> enable_header_;
  TemplatableValue<bool> enable_title_;
  TemplatableValue<std::string> title_;

  TemplatableValue<TextOverflow> text_overflow_;
  TemplatableValue<bool> invert_title_color_;
  TemplatableValue<bool> invert_header_color_;
  TemplatableValue<std::string> date_format_;
  TemplatableValue<std::string> datetime_format_;
  TemplatableValue<std::string> list_style_type_;
  TemplatableValue<bool> enable_list_style_;

  std::vector<std::string> columns_;
  std::vector<int> column_widths_;

  std::vector<int> calculate_column_widths_(display::Display &it, int width, font::Font *font,
                                            const std::vector<Page, Allocator<Page>> &pages);

  void print_row_(display::Display &it, int x, int &current_y, int table_width, bool is_header_row,
                  const std::vector<std::string> &texts, const std::vector<int> &col_widths, font::Font *font,
                  Color color_on, Color color_off);

  std::string format_text_for_column_(const std::string &text, int column_width, display::Display &it, font::Font *font,
                                      bool is_first_column, bool is_header_row);

  int text_width_(display::Display *it, font::Font *font, const std::string &buffer);

  std::string get_cell_text_(const Page &page, const std::string &col, bool is_first_column, bool is_header_row);

  std::string truncate_text_(const std::string &text, int column_width, display::Display &it, font::Font *font,
                             const std::string &suffix);
};

inline std::string tm_to_datetime(const std::tm &tm_time, const std::string &format) {
  char buffer[25];
  std::strftime(buffer, sizeof(buffer), format.c_str(), &tm_time);
  return std::string(buffer);
}

}  // namespace notion_database
}  // namespace esphome
