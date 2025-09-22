import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID

AUTO_LOAD = ["climate_ir"]

climate_ir_lg_ex_ns = cg.esphome_ns.namespace("climate_ir_lg_ex")
LgIrClimateEx = climate_ir_lg_ex_ns.class_("LgIrClimateEx", climate_ir.ClimateIR)

CONF_HEADER_HIGH = "header_high"
CONF_HEADER_LOW = "header_low"
CONF_BIT_HIGH = "bit_high"
CONF_BIT_ONE_LOW = "bit_one_low"
CONF_BIT_ZERO_LOW = "bit_zero_low"
CONF_FLIP_BIT = "flip_bit_num"

CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(LgIrClimateEx).extend(
    {
        cv.Optional(
            CONF_HEADER_HIGH, default="3100us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_HEADER_LOW, default="1550us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_HIGH, default="540us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_ONE_LOW, default="1070us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_BIT_ZERO_LOW, default="290us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_FLIP_BIT, default="-1"
        ): cv.int_range(-1)
    }
)


async def to_code(config):
    var = await climate_ir.new_climate_ir(config)

    cg.add(var.set_header_high(config[CONF_HEADER_HIGH]))
    cg.add(var.set_header_low(config[CONF_HEADER_LOW]))
    cg.add(var.set_bit_high(config[CONF_BIT_HIGH]))
    cg.add(var.set_bit_one_low(config[CONF_BIT_ONE_LOW]))
    cg.add(var.set_bit_zero_low(config[CONF_BIT_ZERO_LOW]))
    cg.add(var.set_flip_bit(config[CONF_FLIP_BIT]))
