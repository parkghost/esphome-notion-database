import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components.notion_database import NotionDatabase

DEPENDENCIES = ["display", "notion_database"]

notion_database_ns = cg.esphome_ns.namespace('notion_database')
NotionDatabaseTableView = notion_database_ns.class_('NotionDatabaseTableView', cg.Component)

# Define the enum properly
TextOverflow = notion_database_ns.enum('TextOverflow')
# Register the enum values explicitly
TEXTOVERFLOW_ELLIPSIS = cg.global_ns.namespace('esphome').namespace('notion_database').namespace('TextOverflow').ELLIPSIS
TEXTOVERFLOW_CLIP = cg.global_ns.namespace('esphome').namespace('notion_database').namespace('TextOverflow').CLIP

CONF_NOTION_DATABASE_ID = "notion_database_id"
CONF_COLUMNS = "columns"
CONF_LINE_HEIGHT = "line_height"
CONF_ENABLE_HEADER = "enable_header"
CONF_ENABLE_TITLE = "enable_title"
CONF_TITLE = "title"
CONF_ENABLE_GRID_LINE = "enable_grid_line"
CONF_COLUMN_WIDTHS = "column_widths"
CONF_TEXT_OVERFLOW = "text_overflow"
CONF_INVERT_TITLE_COLOR = "invert_title_color"
CONF_INVERT_HEADER_COLOR = "invert_header_color"
CONF_DATE_FORMAT = "date_format"
CONF_DATETIME_FORMAT = "datetime_format"
CONF_LIST_STYLE_TYPE = "list_style_type"
CONF_ENABLE_LIST_STYLE = "enable_list_style"


CONFIG_SCHEMA =  cv.All(
    cv.ensure_list(
        cv.Schema({
            cv.GenerateID(): cv.declare_id(NotionDatabaseTableView),
            cv.GenerateID(CONF_NOTION_DATABASE_ID): cv.use_id(NotionDatabase),
            cv.Optional(CONF_COLUMNS, default=[]): cv.ensure_list(cv.string),
            cv.Optional(CONF_COLUMN_WIDTHS, default=[]): cv.ensure_list(cv.int_),
            cv.Optional(CONF_LINE_HEIGHT, default=40): cv.templatable(cv.int_),
            cv.Optional(CONF_TITLE, default=""): cv.templatable(cv.string),
            cv.Optional(CONF_ENABLE_TITLE, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_INVERT_TITLE_COLOR, default=True): cv.templatable(cv.boolean),
            cv.Optional(CONF_ENABLE_HEADER, default=True): cv.templatable(cv.boolean),
            cv.Optional(CONF_INVERT_HEADER_COLOR, default=True): cv.templatable(cv.boolean),
            cv.Optional(CONF_ENABLE_GRID_LINE, default=True): cv.templatable(cv.boolean),
            cv.Optional(CONF_LIST_STYLE_TYPE, default="• "): cv.templatable(cv.string),
            cv.Optional(CONF_ENABLE_LIST_STYLE, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_TEXT_OVERFLOW, default="ELLIPSIS"): cv.templatable(cv.enum({
                "ELLIPSIS": TEXTOVERFLOW_ELLIPSIS,
                "CLIP": TEXTOVERFLOW_CLIP
            }, upper=True)),
            cv.Optional(CONF_DATE_FORMAT, default="%Y-%m-%d"): cv.templatable(cv.string),
            cv.Optional(CONF_DATETIME_FORMAT, default="%Y-%m-%d %H:%M"): cv.templatable(cv.string),
        }).extend(cv.COMPONENT_SCHEMA)
    )
)

async def to_code(configs):
    for config in configs:
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)

        notion_database = await cg.get_variable(config[CONF_NOTION_DATABASE_ID])
        cg.add(var.set_database_parent(notion_database))

        for column in config[CONF_COLUMNS]:
            cg.add(var.add_column(column))
        for column_width in config[CONF_COLUMN_WIDTHS]:
            cg.add(var.add_column_width(column_width))
        if line_height := await cg.templatable(config[CONF_LINE_HEIGHT], [], cg.int_):
            cg.add(var.set_line_height(line_height))
        if title := await cg.templatable(config[CONF_TITLE], [], cg.std_string):
            cg.add(var.set_title(title))
        if enable_title := await cg.templatable(config[CONF_ENABLE_TITLE], [], cg.bool_):
            cg.add(var.set_enable_title(enable_title))
        if invert_title_color := await cg.templatable(config[CONF_INVERT_TITLE_COLOR], [], cg.bool_):
            cg.add(var.set_invert_title_color(invert_title_color))
        if enable_header := await cg.templatable(config[CONF_ENABLE_HEADER], [], cg.bool_):
            cg.add(var.set_enable_header(enable_header))
        if invert_header_color := await cg.templatable(config[CONF_INVERT_HEADER_COLOR], [], cg.bool_):
            cg.add(var.set_invert_header_color(invert_header_color))
        if enable_grid_line := await cg.templatable(config[CONF_ENABLE_GRID_LINE], [], cg.bool_):
            cg.add(var.set_enable_grid_line(enable_grid_line))
        if list_style_type := await cg.templatable(config[CONF_LIST_STYLE_TYPE], [], cg.std_string):
            cg.add(var.set_list_style_type(list_style_type))
        if enable_list_style := await cg.templatable(config[CONF_ENABLE_LIST_STYLE], [], cg.bool_):
            cg.add(var.set_enable_list_style(enable_list_style))
        if text_overflow := await cg.templatable(config[CONF_TEXT_OVERFLOW], [], TextOverflow):
            cg.add(var.set_text_overflow(text_overflow))
        if date_format := await cg.templatable(config[CONF_DATE_FORMAT], [], cg.std_string):
            cg.add(var.set_date_format(date_format))
        if datetime_format := await cg.templatable(config[CONF_DATETIME_FORMAT], [], cg.std_string):
            cg.add(var.set_datetime_format(datetime_format))

