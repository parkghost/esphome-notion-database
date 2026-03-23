import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.components import http_request

DEPENDENCIES = ["network", "http_request"]
AUTO_LOAD = ["json"]

notion_database_ns = cg.esphome_ns.namespace("notion_database")
NotionDatabase = notion_database_ns.class_("NotionDatabase", cg.PollingComponent)
NotionDatabasePage = notion_database_ns.class_("Page")
FirstPageAction = notion_database_ns.class_("FirstPageAction", automation.Action)
NextPageAction = notion_database_ns.class_("NextPageAction", automation.Action)
PreviousPageAction = notion_database_ns.class_("PreviousPageAction", automation.Action)

CONF_API_TOKEN = "api_token"
CONF_DATABASE_ID = "database_id"
CONF_QUERY = "query"
CONF_PROPERTY_FILTERS = "property_filters"
CONF_ON_PAGE_CHANGE = "on_page_change"
CONF_HTTP_REQUEST_ID = "http_request_id"
CONF_JSON_PARSE_BUFFER_SIZE = "json_parse_buffer_size"

CONFIG_SCHEMA = cv.All(
    cv.ensure_list(
        cv.Schema({
            cv.GenerateID(): cv.declare_id(NotionDatabase),
            cv.GenerateID(CONF_HTTP_REQUEST_ID): cv.use_id(http_request.HttpRequestComponent),
            cv.Optional(CONF_API_TOKEN, default=""): cv.templatable(cv.string),
            cv.Optional(CONF_DATABASE_ID, default=""): cv.templatable(cv.string),
            cv.Optional(CONF_QUERY, default=""): cv.templatable(cv.string),
            cv.Optional(CONF_PROPERTY_FILTERS, default=[]): cv.ensure_list(cv.string),
            cv.Optional(CONF_ON_PAGE_CHANGE): automation.validate_automation(),
            cv.Optional(CONF_JSON_PARSE_BUFFER_SIZE, default="20kB"): cv.templatable(cv.validate_bytes),
        }).extend(cv.polling_component_schema('60s'))
    ),
    cv.only_on_esp32,
    cv.require_esphome_version(2026, 2, 0)
)

async def to_code(configs):
    for config in configs:
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)

        if api_token_tpl := await cg.templatable(config[CONF_API_TOKEN], [], cg.std_string):
            cg.add(var.set_api_token(api_token_tpl))
        if database_id_tpl := await cg.templatable(config[CONF_DATABASE_ID], [], cg.std_string):
            cg.add(var.set_database_id(database_id_tpl))
        if query_tpl := await cg.templatable(config[CONF_QUERY], [], cg.std_string):
            cg.add(var.set_query(query_tpl))
        for property_filter in config[CONF_PROPERTY_FILTERS]:
            cg.add(var.add_property_filter(property_filter))
        for trigger in config.get(CONF_ON_PAGE_CHANGE, []):
            await automation.build_automation(
                    var.get_on_page_change_trigger(),
                    [],
                    trigger)

        if CONF_HTTP_REQUEST_ID in config:
            http_req = await cg.get_variable(config[CONF_HTTP_REQUEST_ID])
            cg.add(var.set_http_request(http_req))
        if CONF_JSON_PARSE_BUFFER_SIZE in config:
            buffer_size_tpl = await cg.templatable(config[CONF_JSON_PARSE_BUFFER_SIZE], [], cg.uint32)
            cg.add(var.set_json_parse_buffer_size(buffer_size_tpl))

NOTION_DATABASE_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(NotionDatabase),
    }
)

@automation.register_action("notion_database.first_page", FirstPageAction, NOTION_DATABASE_SCHEMA, synchronous=True)
async def notion_database_first_page_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("notion_database.next_page", NextPageAction, NOTION_DATABASE_SCHEMA, synchronous=True)
async def notion_database_next_page_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("notion_database.prev_page", PreviousPageAction, NOTION_DATABASE_SCHEMA, synchronous=True)
async def notion_database_prev_page_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)
