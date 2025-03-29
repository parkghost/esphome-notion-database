import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome import automation
from esphome.automation import maybe_simple_id

DEPENDENCIES = ["network"]
AUTO_LOAD = ["json", "watchdog"]

notion_database_ns = cg.esphome_ns.namespace("notion_database")
NotionDatabase = notion_database_ns.class_("NotionDatabase", cg.PollingComponent, automation.Automation)
NotionDatabasePage = notion_database_ns.class_("Page")
FirstPageAction = notion_database_ns.class_("FirstPageAction", automation.Action)
NextPageAction = notion_database_ns.class_("NextPageAction", automation.Action)
PreviousPageAction = notion_database_ns.class_("PreviousPageAction", automation.Action)

CONF_API_TOKEN = "api_token"
CONF_DATABASE_ID = "database_id"
CONF_QUERY = "query"
CONF_PROPERTY_FILTERS = "property_filters"
CONF_ON_PAGE_CHANGE = "on_page_change"
CONF_WATCHDOG_TIMEOUT = "watchdog_timeout"
CONF_HTTP_CONNECT_TIMEOUT = "http_connect_timeout"
CONF_HTTP_TIMEOUT = "http_timeout"
CONF_JSON_PARSE_BUFFER_SIZE = "json_parse_buffer_size"
CONF_VERIFY_SSL = "verify_ssl"

CONFIG_SCHEMA = cv.All(
    cv.ensure_list(
        cv.Schema({
            cv.GenerateID(): cv.declare_id(NotionDatabase),
            cv.Optional(CONF_API_TOKEN, default=""): cv.templatable(cv.string),
            cv.Optional(CONF_DATABASE_ID, default=""): cv.templatable(cv.string),
            cv.Optional(CONF_QUERY, default=""): cv.templatable(cv.string),
            cv.Optional(CONF_PROPERTY_FILTERS, default=[]): cv.ensure_list(cv.string),
            cv.Optional(CONF_ON_PAGE_CHANGE): automation.validate_automation(),
            cv.Optional(CONF_WATCHDOG_TIMEOUT, default="15s"): cv.templatable(cv.All(
                cv.positive_not_null_time_period,
                cv.positive_time_period_milliseconds,
            )),
            cv.Optional(CONF_VERIFY_SSL, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_HTTP_CONNECT_TIMEOUT, default="5s"): cv.templatable(cv.All(
                cv.positive_not_null_time_period,
                cv.positive_time_period_milliseconds,
            )),
            cv.Optional(CONF_HTTP_TIMEOUT, default="10s"): cv.templatable(cv.All(
                cv.positive_not_null_time_period,
                cv.positive_time_period_milliseconds,
            )),
            cv.Optional(CONF_JSON_PARSE_BUFFER_SIZE, default="20kB"): cv.templatable(cv.validate_bytes),
        }).extend(cv.polling_component_schema('60s'))
    ),
    cv.only_with_arduino,
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

        if CONF_WATCHDOG_TIMEOUT in config:
            timeout_tpl = await cg.templatable(config[CONF_WATCHDOG_TIMEOUT], [], cg.uint32)
            cg.add(var.set_watchdog_timeout(timeout_tpl))
        if CONF_VERIFY_SSL in config:
            verify_ssl_tpl = await cg.templatable(config[CONF_VERIFY_SSL], [], cg.bool_)
            cg.add(var.set_verify_ssl(verify_ssl_tpl))
        if CONF_HTTP_CONNECT_TIMEOUT in config:
            timeout_tpl = await cg.templatable(config[CONF_HTTP_CONNECT_TIMEOUT], [], cg.uint32)
            cg.add(var.set_http_connect_timeout(timeout_tpl))
        if CONF_HTTP_TIMEOUT in config:
            timeout_tpl = await cg.templatable(config[CONF_HTTP_TIMEOUT], [], cg.uint32)
            cg.add(var.set_http_timeout(timeout_tpl))
        if CONF_JSON_PARSE_BUFFER_SIZE in config:
            buffer_size_tpl = await cg.templatable(config[CONF_JSON_PARSE_BUFFER_SIZE], [], cg.uint32)
            cg.add(var.set_json_parse_buffer_size(buffer_size_tpl))

    cg.add_library("WiFiClientSecure", None)
    cg.add_library("HTTPClient", None)

NOTION_DATABASE_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(NotionDatabase),
    }
)

@automation.register_action("notion_database.first_page", FirstPageAction, NOTION_DATABASE_SCHEMA)
async def notion_database_first_page_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("notion_database.next_page", NextPageAction, NOTION_DATABASE_SCHEMA)
async def notion_database_next_page_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("notion_database.prev_page", PreviousPageAction, NOTION_DATABASE_SCHEMA)
async def notion_database_prev_page_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)
