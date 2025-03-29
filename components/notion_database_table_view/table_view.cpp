#include "table_view.h"

#include "esphome/components/display/display.h"
#include "esphome/components/notion_database/notion_database.h"

namespace esphome {
namespace notion_database {

void NotionDatabaseTableView::draw(display::Display &it, int x, int y, int width, int height, font::Font *font,
                                         Color color_on, Color color_off) {
  // Check if the database parent is valid
  if (this->database_parent_ == nullptr) {
    ESP_LOGW("table_view", "database_parent_ is null, skipping draw");
    return;
  }
  if (this->columns_.empty()) {
    ESP_LOGW("table_view", "Columns are empty, fetching available properties from database_parent_");
    auto available_properties = this->database_parent_->get_available_properties();
    this->columns_ = std::vector<std::string>(available_properties.begin(), available_properties.end());
  }

  auto pages = this->database_parent_->get_pages();
  std::vector<int> col_widths = calculate_column_widths_(it, width, font, pages);

  int current_y = y;

  // Draw the top grid line if enabled
  if (enable_grid_line_.value()) {
    it.line(x, current_y, x + width, current_y, color_on);
  }

  // Draw the title if enabled and not empty
  if (enable_title_.value() && !title_.value().empty()) {
    // Invert the title color if enabled
    if (invert_title_color_.value()) {
      it.filled_rectangle(x, current_y, width, line_height_.value(), color_on);
      it.printf(x + width / 2, current_y + line_height_.value() / 2, font, color_off, display::TextAlign::CENTER,
                title_.value().c_str());
    } else {
      it.printf(x + width / 2, current_y + line_height_.value() / 2, font, color_on, display::TextAlign::CENTER,
                title_.value().c_str());
    }
    current_y += line_height_.value();
  }

  // Draw the top grid line if enabled
  if (enable_grid_line_.value()) {
    it.line(x, current_y, x + width, current_y, color_on);
  }

  // Draw the header if enabled and columns are defined
  if (enable_header_.value() && !columns_.empty()) {
    // Invert the header color if enabled
    if (invert_header_color_.value()) {
      int saved_y = current_y;
      it.filled_rectangle(x, current_y, width, line_height_.value(), color_on);
      print_row_(it, x, current_y, width, true, columns_, col_widths, font, color_off, color_on);

      int current_x = x;
      for (size_t i = 0; i < col_widths.size() - 1; i++) {
        if (col_widths[i] == 0) continue;

        current_x += col_widths[i];
        int grid_x = (current_x > x + width) ? x + width : current_x;
        it.line(grid_x, saved_y, grid_x, current_y, color_on);
      }
    } else {
      print_row_(it, x, current_y, width, true, columns_, col_widths, font, color_on, color_off);
    }
  }

  // Draw each row of the table
  for (const auto &page : pages) {
    if (current_y + line_height_.value() > y + height) break;

    std::vector<std::string> row_texts;
    for (size_t i = 0; i < columns_.size(); i++) {
      row_texts.push_back(get_cell_text_(page, columns_[i], i == 0, false));
    }
    print_row_(it, x, current_y, width, false, row_texts, col_widths, font, color_on, color_off);
  }

  // Draw the vertical grid lines if enabled
  if (enable_grid_line_.value()) {
    it.line(x, y, x, current_y, color_on);
    it.line(x + width - 1, y, x + width - 1, current_y, color_on);
  }
}

std::vector<int> NotionDatabaseTableView::calculate_column_widths_(display::Display &it, int width, font::Font *font,
                                                                   const std::vector<Page, Allocator<Page>> &pages) {
  const int right_padding = 10;
  std::vector<int> col_widths(columns_.size(), 0);
  int total_width = 0;

  // Calculate column widths based on predefined widths or content
  if (!column_widths_.empty() && column_widths_.size() == columns_.size()) {
    col_widths = column_widths_;
    for (size_t i = 0; i < col_widths.size(); i++) {
      if (total_width + col_widths[i] <= width) {
        total_width += col_widths[i];
      } else {
        col_widths[i] = std::max(0, width - total_width);
        total_width = width;
      }
    }
  } else {
    for (size_t i = 0; i < columns_.size(); i++) {
      int max_w = enable_header_.value() ? text_width_(&it, font, columns_[i]) : 0;
      for (const auto &page : pages) {
        const std::string &cell_text = get_cell_text_(page, columns_[i], i == 0, false);
        if (!cell_text.empty()) {
          max_w = std::max(max_w, text_width_(&it, font, cell_text));
        }
      }
      max_w += right_padding;
      if (total_width + max_w <= width) {
        col_widths[i] = max_w;
        total_width += max_w;
      } else {
        col_widths[i] = std::max(0, width - total_width);
        total_width = width;
      }
    }
  }

  // Adjust the last column width to fill the remaining space
  if (total_width < width) {
    for (int i = col_widths.size() - 1; i >= 0; i--) {
      if (col_widths[i] > 0) {
        col_widths[i] += (width - total_width);
        break;
      }
    }
  }

  // Log the calculated column widths
  for (size_t i = 0; i < columns_.size(); i++) {
    ESP_LOGD("table_view", "Column %d (%s) width: %d", i + 1, columns_[i].c_str(), col_widths[i]);
  }

  return col_widths;
}

int NotionDatabaseTableView::text_width_(display::Display *it, font::Font *font, const std::string &buffer) {
  int x1 = 0;
  int y1 = 0;
  int w = 0;
  int h = 0;
  it->get_text_bounds(0, 0, buffer.c_str(), font, display::TextAlign::TOP_LEFT, &x1, &y1, &w, &h);
  return w;
}

std::string NotionDatabaseTableView::truncate_text_(const std::string &text, int column_width, display::Display &it,
                                                    font::Font *font, const std::string &suffix) {
  std::string result = text;
  // Truncate text to fit within the column width
  while (!result.empty() && text_width_(&it, font, result + suffix) > column_width) {
    size_t lastCharPos = result.size() - 1;
    while (lastCharPos > 0 && (result[lastCharPos] & 0xC0) == 0x80) {
      lastCharPos--;
    }
    result.erase(lastCharPos);
  }
  return result + suffix;
}

std::string NotionDatabaseTableView::format_text_for_column_(const std::string &text, int column_width,
                                                             display::Display &it, font::Font *font,
                                                             bool is_first_column, bool is_header_row) {
  std::string result_text = text;
  if (result_text.empty()) {
    return result_text;
  }

  int text_w = text_width_(&it, font, result_text);
  // Format text based on overflow settings
  if (text_w <= column_width) {
    return result_text;
  }

  if (text_overflow_.value() == TextOverflow::ELLIPSIS) {
    return truncate_text_(result_text, column_width, it, font, "...");
  } else if (text_overflow_.value() == TextOverflow::CLIP) {
    return truncate_text_(result_text, column_width, it, font, "");
  } else {
    return result_text;
  }
}

void NotionDatabaseTableView::print_row_(display::Display &it, int x, int &current_y, int table_width,
                                         bool is_header_row, const std::vector<std::string> &texts,
                                         const std::vector<int> &col_widths, font::Font *font, Color color_on,
                                         Color color_off) {
  int current_x = x;

  // Print each cell in the row
  for (size_t i = 0; i < texts.size(); i++) {
    if (col_widths[i] == 0) continue;

    std::string display_text = format_text_for_column_(texts[i], col_widths[i], it, font, i == 0, is_header_row);
    it.printf(current_x + 2, current_y + 2 + line_height_.value() / 2, font, color_on, display::TextAlign::CENTER_LEFT,
              display_text.c_str());
    if (i < texts.size() - 1) {
      current_x += col_widths[i];
      // Draw vertical grid lines between cells if enabled
      if (enable_grid_line_.value()) {
        int grid_x = (current_x > x + table_width) ? x + table_width : current_x;
        it.line(grid_x, current_y, grid_x, current_y + line_height_.value(), color_on);
      }
    } else {
      current_x = x + table_width;
    }
  }
  current_y += line_height_.value();
  // Draw horizontal grid line below the row if enabled
  if (enable_grid_line_.value()) {
    it.line(x, current_y, x + table_width, current_y, color_on);
  }
}

std::string NotionDatabaseTableView::get_cell_text_(const Page &page, const std::string &col, bool is_first_column,
                                                    bool is_header_row) {
  std::string result_text;

  // Retrieve the property value for the cell
  const NotionProperty *prop = page.get_property(col);
  if (prop != nullptr) {
    if (prop->type == NotionPropertyType::DATE) {
      result_text = tm_to_datetime(prop->time_value, date_format_.value());
    } else if (prop->type == NotionPropertyType::CREATED_TIME || prop->type == NotionPropertyType::LAST_EDITED_TIME) {
      result_text = tm_to_datetime(prop->time_value, datetime_format_.value());
    } else {
      result_text = notion_property_to_string(*prop);
    }
  }

  // Add symbol for the first column if enabled
  if (is_first_column && !is_header_row && enable_list_style_.value()) {
    auto list_style_type = list_style_type_.value();
    if (!list_style_type.empty()) {
      result_text = list_style_type + result_text;
    }
  }
  return result_text;
}

}  // namespace notion_database
}  // namespace esphome