#include "dsmr_parser/fields.h"
#include "dsmr_parser/parser.h"
#include <doctest.h>
#include <iostream>

using namespace dsmr_parser;
using namespace fields;

struct Printer {
  template <typename Item>
  void apply(Item& i) {
    if (i.present()) {
      std::cout << Item::name << ": " << i.val() << Item::unit() << std::endl;
    }
  }
};

TEST_CASE("Should parse all fields in the DSMR message correctly") {
  const auto& msg = "/KFM5KAIFA-METER\r\n"
                    "\r\n"
                    "1-3:0.2.8(40)\r\n"
                    "0-0:1.0.0(150117185916W)\r\n"
                    "0-0:96.1.1(0000000000000000000000000000000000)\r\n"
                    "1-0:1.8.1(000671.578*kWh)\r\n"
                    "1-0:1.8.2(000842.472*kWh)\r\n"
                    "1-0:2.8.1(000000.000*kWh)\r\n"
                    "1-0:2.8.2(000000.000*kWh)\r\n"
                    "0-0:96.14.0(0001)\r\n"
                    "1-0:1.7.0(00.333*kW)\r\n"
                    "1-0:2.7.0(00.000*kW)\r\n"
                    "0-0:17.0.0(999.9*kW)\r\n"
                    "0-0:96.3.10(1)\r\n"
                    "0-0:96.7.21(00008)\r\n"
                    "0-0:96.7.9(00007)\r\n"
                    "1-0:99.97.0(1)(0-0:96.7.19)(000101000001W)(2147483647*s)\r\n"
                    "0-0:98.1.0(2)(1-0:1.6.0)(1-0:1.6.0)(230201000000W)(230117224500W)(04.329*kW)(230202000000W)(230214224500W)(04529*W)\r\n"
                    "1-0:32.32.0(00000)\r\n"
                    "1-0:32.36.0(00000)\r\n"
                    "0-0:96.13.1()\r\n"
                    "0-0:96.13.0()\r\n"
                    "1-0:31.7.0(001*A)\r\n"
                    "1-0:21.7.0(00.332*kW)\r\n"
                    "1-0:22.7.0(00.000*kW)\r\n"
                    "0-1:24.1.0(003)\r\n"
                    "0-1:96.1.0(0000000000000000000000000000000000)\r\n"
                    "0-1:24.2.1(150117180000W)(00473.789*m3)\r\n"
                    "0-1:24.4.0(1)\r\n"
                    "!f2C9\r\n";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version,
      /* String */ timestamp,
      /* String */ equipment_id,
      /* FixedValue */ energy_delivered_tariff1,
      /* FixedValue */ energy_delivered_tariff2,
      /* FixedValue */ energy_returned_tariff1,
      /* FixedValue */ energy_returned_tariff2,
      /* String */ electricity_tariff,
      /* FixedValue */ power_delivered,
      /* FixedValue */ power_returned,
      /* FixedValue */ electricity_threshold,
      /* uint8_t */ electricity_switch_position,
      /* uint32_t */ electricity_failures,
      /* uint32_t */ electricity_long_failures,
      /* String */ electricity_failure_log,
      /* uint32_t */ electricity_sags_l1,
      /* uint32_t */ electricity_sags_l2,
      /* uint32_t */ electricity_sags_l3,
      /* uint32_t */ electricity_swells_l1,
      /* uint32_t */ electricity_swells_l2,
      /* uint32_t */ electricity_swells_l3,
      /* String */ message_short,
      /* String */ message_long,
      /* FixedValue */ voltage_l1,
      /* FixedValue */ voltage_l2,
      /* FixedValue */ voltage_l3,
      /* FixedValue */ current_l1,
      /* FixedValue */ current_l2,
      /* FixedValue */ current_l3,
      /* FixedValue */ power_delivered_l1,
      /* FixedValue */ power_delivered_l2,
      /* FixedValue */ power_delivered_l3,
      /* FixedValue */ power_returned_l1,
      /* FixedValue */ power_returned_l2,
      /* FixedValue */ power_returned_l3,
      /* uint16_t */ gas_device_type,
      /* String */ gas_equipment_id,
      /* uint8_t */ gas_valve_position,
      /* TimestampedFixedValue */ gas_delivered,
      /* uint16_t */ thermal_device_type,
      /* String */ thermal_equipment_id,
      /* uint8_t */ thermal_valve_position,
      /* TimestampedFixedValue */ thermal_delivered,
      /* uint16_t */ water_device_type,
      /* String */ water_equipment_id,
      /* uint8_t */ water_valve_position,
      /* TimestampedFixedValue */ water_delivered,
      /* AveragedFixedField */ active_energy_import_maximum_demand_last_13_months>
      data;

  auto res = P1Parser::parse(&data, msg, std::size(msg), true);
  REQUIRE(res.err == nullptr);

  // Print all values
  data.applyEach(Printer());

  // Check that all fields have correct values
  REQUIRE(data.identification == "KFM5KAIFA-METER");
  REQUIRE(data.p1_version == "40");
  REQUIRE(data.timestamp == "150117185916W");
  REQUIRE(data.equipment_id == "0000000000000000000000000000000000");
  REQUIRE(data.energy_delivered_tariff1 == 671.578f);
  REQUIRE(data.energy_delivered_tariff2 == 842.472f);
  REQUIRE(data.energy_returned_tariff1 == 0.0f);
  REQUIRE(data.energy_returned_tariff2 == 0.0f);
  REQUIRE(data.electricity_tariff == "0001");
  REQUIRE(data.power_delivered == 0.333f);
  REQUIRE(data.power_returned == 0.0f);
  REQUIRE(data.electricity_threshold == 999.9f);
  REQUIRE(data.electricity_switch_position == 1);
  REQUIRE(data.electricity_failures == 8);
  REQUIRE(data.electricity_long_failures == 7);
  REQUIRE(data.electricity_failure_log == "(1)(0-0:96.7.19)(000101000001W)(2147483647*s)");
  REQUIRE(data.electricity_sags_l1 == 0);
  REQUIRE(data.electricity_swells_l1 == 0);
  REQUIRE(data.message_short.empty());
  REQUIRE(data.message_long.empty());
  REQUIRE(data.current_l1 == 1.0f);
  REQUIRE(data.power_delivered_l1 == 0.332f);
  REQUIRE(data.power_returned_l1 == 0.0f);
  REQUIRE(data.gas_device_type == 3);
  REQUIRE(data.gas_equipment_id == "0000000000000000000000000000000000");
  REQUIRE(data.gas_valve_position == 1);
  REQUIRE(data.gas_delivered == 473.789f);
  REQUIRE(data.active_energy_import_maximum_demand_last_13_months.val() == 4.429f);
}

TEST_CASE("Should report an error if the crc has incorrect format") {
  const auto& msg = "/KFM5KAIFA-METER\r\n"
                    "\r\n"
                    "1-0:1.8.1(000671.578*kWh)\r\n"
                    "1-0:1.7.0(00.318*kW)\r\n"
                    "!1ED\r\n";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  auto res = P1Parser::parse(&data, msg, std::size(msg), true);
  REQUIRE(std::string(res.err) == "Incomplete or malformed checksum");
}

TEST_CASE("Should report an error if the crc of a package is incorrect") {
  const auto& msg = "/KFM5KAIFA-METER\r\n"
                    "\r\n"
                    "1-0:.8.1(000671.578*kWh)\r\n"
                    "1-0:1.7.0(00.318*kW)\r\n"
                    "!1E1D\r\n";
  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  auto res = P1Parser::parse(&data, msg, std::size(msg), true);
  REQUIRE(std::string(res.err) == "Checksum mismatch");

  const auto& fullError = res.fullError(msg, msg + std::size(msg));
  std::cout << "Full error" << std::endl << fullError << std::endl;
  REQUIRE(fullError == "!1E1D\r\n ^\r\nChecksum mismatch");
}

TEST_CASE("Should parse Wh-based integers for FixedField (fallback int_unit path)") {
  const auto& msg = "/ABC5MTR\r\n"
                    "\r\n"
                    "1-0:1.8.0(000441879*Wh)\r\n"
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ energy_delivered_lux>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(res.err == nullptr);
  REQUIRE(data.energy_delivered_lux == 441.879f); // 441,879 Wh => 441.879 kWh
  REQUIRE(fields::energy_delivered_lux::unit() == std::string("kWh"));
  REQUIRE(fields::energy_delivered_lux::int_unit() == std::string("Wh"));
}

TEST_CASE("Should parse TimestampedFixedField for gas_delivered_be and expose timestamp") {
  const auto& msg = "/DEF5MTR\r\n"
                    "\r\n"
                    "0-1:24.2.3(230101120000W)(00012.345*m3)\r\n"
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* TimestampedFixedValue */ gas_delivered_be>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(res.err == nullptr);
  REQUIRE(data.gas_delivered_be == 12.345f);
  REQUIRE(data.gas_delivered_be.timestamp == "230101120000W");
}

TEST_CASE("Should take the last value with LastFixedField (capacity rate history)") {
  const auto& msg = "/KFM5MTR\r\n"
                    "\r\n"
                    "0-0:98.1.0(1)(1-0:1.6.0)(1-0:1.6.0)(230201000000W)(230117224500W)(04.329*kW)\r\n"
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ active_energy_import_maximum_demand_last_13_months>
      data;

  P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(data.active_energy_import_maximum_demand_last_13_months == 4.329f);
}

TEST_CASE("Should detect duplicate fields") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.100*kW)\r\n"
                    "1-0:1.7.0(00.200*kW)\r\n"
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Duplicate field");
}

TEST_CASE("Should error on unknown field when unknown_error is true") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:2.7.0(00.000*kW)\r\n" // power_returned not part of ParsedData below
                    "!\r\n";

  ParsedData<
      /* String */ identification>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/true, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Unknown field");
}

TEST_CASE("Should report OBIS ID numbers over 255") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "256-0:1.7.0(00.100*kW)\r\n" // invalid OBIS (256)
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Obis ID has number over 255");
}

TEST_CASE("Should validate string length bounds (p1_version too short)") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-3:0.2.8(4)\r\n" // p1_version expects 2 chars
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Invalid string length");
}

TEST_CASE("Should validate string length bounds (p1_version too long)") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-3:0.2.8(123)\r\n" // p1_version expects 2 chars
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Invalid string length");
}

TEST_CASE("Should validate units for numeric fields") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.318*kVA)\r\n" // expects kW, not kVA
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Invalid unit");
}

TEST_CASE("Should report missing closing parenthesis for StringField") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-3:0.2.8(40\r\n" // missing ')'
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Last dataline not CRLF terminated");
}

TEST_CASE("Should compute FixedField with decimals and millivolt int_unit correctly") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:32.7.0(230.1*V)\r\n" // voltage_l1 (V / mV)
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ voltage_l1>
      data;

  P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(data.voltage_l1 == 230.1f);
}

TEST_CASE("all_present() should reflect presence of all requested fields") {
  SUBCASE("All fields present -> true") {
    const auto& msg = "/AAA5MTR\r\n"
                      "\r\n"
                      "1-0:1.7.0(00.123*kW)\r\n"
                      "!\r\n";

    ParsedData<
        /* String */ identification,
        /* FixedValue */ power_delivered>
        data;

    P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
    REQUIRE(data.all_present());
  }

  SUBCASE("Missing a requested field -> false") {
    const auto& msg = "/AAA5MTR\r\n"
                      "\r\n"
                      "!\r\n";

    ParsedData<
        /* String */ identification,
        /* FixedValue */ power_delivered>
        data;

    P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
    REQUIRE_FALSE(data.all_present());
  }
}

TEST_CASE("Should report last dataline not CRLF terminated") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.123*kW)" // no CRLF before '!'
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Last dataline not CRLF terminated");
}

TEST_CASE("Should report an error if checksum is not found") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.123*kW)"
                    "!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/true);
  REQUIRE(std::string(res.err) == "No checksum found");
}

TEST_CASE("Doesn't crash for an empty packet") {
  const auto& msg = "";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  auto res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/true);
  REQUIRE(std::string(res.err) == "Data should start with /");

  res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Data should start with /");
}

TEST_CASE("Doesn't crash for a small packet") {
  const auto& msg = "/!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  auto res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/true);
  REQUIRE(std::string(res.err) == "No checksum found");

  res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(res.err == nullptr);
}

TEST_CASE("Doesn't crash for a small packet 2") {
  const auto& msg = "/a!";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  auto res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/true);
  REQUIRE(std::string(res.err) == "No checksum found");

  res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Last dataline not CRLF terminated");
}

TEST_CASE("Doesn't crash for a partial checksum") {
  const auto& msg = "/!A1";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/true);
  REQUIRE(std::string(res.err) == "No checksum found");
}

TEST_CASE("Doesn't crash for a packet that doesn't end with '!' symbol") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.123*kW)";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/true);
  REQUIRE(std::string(res.err) == "Data should end with !");
}

TEST_CASE("Trailing characters on data line") {
  const auto& msg = "/AAA5MTR\r\n\r\n"
                    "1-0:1.7.0(00.123*kW) trailing\r\n"
                    "!\r\n";
  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = P1Parser::parse(&data, msg, std::size(msg), false, false);
  REQUIRE(std::string(res.err) == "Trailing characters on data line");
}

TEST_CASE("Unknown field ignored when unknown_error is false") {
  const auto& msg = "/AAA5MTR\r\n\r\n"
                    "1-0:2.7.0(00.000*kW)\r\n"
                    "!\r\n";
  ParsedData</*String*/ identification> data;
  auto res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(res.err == nullptr);
}

TEST_CASE("Missing unit when required") {
  const auto& msg = "/AAA5MTR\r\n\r\n"
                    "1-0:1.7.0(00.123)\r\n"
                    "!\r\n";
  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = P1Parser::parse(&data, msg, std::size(msg), false, false);
  REQUIRE(std::string(res.err) == "Missing unit");
}

TEST_CASE("Unit present when not expected") {
  const auto& msg = "/AAA5MTR\r\n\r\n"
                    "0-0:96.7.21(00008*s)\r\n"
                    "!\r\n";
  ParsedData</*String*/ identification, /*uint32_t*/ electricity_failures> data;
  const auto& res = P1Parser::parse(&data, msg, std::size(msg), false, false);
  REQUIRE(std::string(res.err) == "Extra data");
}

TEST_CASE("Malformed packet that starts with ')'") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-3:0.2.8)40(\r\n"
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* String */ p1_version>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Unexpected ')' symbol");
}

TEST_CASE("Non-digit in numeric part") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(00.A23*kW)\r\n"
                    "!\r\n";

  ParsedData<
      /* String */ identification,
      /* FixedValue */ power_delivered>
      data;

  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Invalid number");
}

TEST_CASE("OBIS id empty line") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "garbage\r\n"
                    "!\r\n";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "OBIS id Empty");
}

TEST_CASE("Accepts LF-only line endings") {
  const auto& msg = "/AAA5MTR\n"
                    "\n"
                    "1-0:1.7.0(00.123*kW)\n"
                    "!\n";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(data.power_delivered == 0.123f);
}

TEST_CASE("Unit matching is case-insensitive") {
  const auto& msg = "/ABC5MTR\r\n"
                    "\r\n"
                    "1-0:1.8.1(000001.000*kwh)\r\n"
                    "!\r\n";

  ParsedData</*String*/ identification, /*FixedValue*/ energy_delivered_tariff1> data;
  P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(data.energy_delivered_tariff1 == 1.000f);
}

TEST_CASE("Numeric without decimals is accepted (auto-padded)") {
  const auto& msg = "/AAA5MTR\r\n"
                    "\r\n"
                    "1-0:1.7.0(1*kW)\r\n"
                    "!";

  ParsedData</*String*/ identification, /*FixedValue*/ power_delivered> data;
  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(res.err == nullptr);
  REQUIRE(data.power_delivered == 1.0f);
}

TEST_CASE("Can parse a dataline if it has a break in the middle") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-1:24.3.0(120517020000)(08)(60)(1)(0-1:24.2.1)(m3)\r\n"
                    "(00124.477)\r\n"
                    "0-0:96.13.0(303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F\r\n"
                    "303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F\r\n"
                    "303132333435363738393A3B3C3D3E3F)\r\n"
                    "!";

  ParsedData<identification, gas_delivered_text, message_long> data;
  P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(data.gas_delivered_text == "(120517020000)(08)(60)(1)(0-1:24.2.1)(m3)\r\n(00124.477)");
  REQUIRE(data.message_long == "303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F\r\n303132333435363738393A3B3C3D3E3F30313233343536373"
                               "8393A3B3C3D3E3F\r\n303132333435363738393A3B3C3D3E3F");
}

TEST_CASE("Can parse a 0 value without a unit") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-1:24.2.1(000101000000W)(00000000.0000)\r\n"
                    "!";
  ParsedData<gas_delivered> data;
  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(res.err == nullptr);
  REQUIRE(data.gas_delivered == 0.0f);
}

TEST_CASE("Whitespace after OBIS ID") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-1:24.2.1 (000101000000W)(00000000.0000)\r\n"
                    "!";
  ParsedData<gas_delivered> data;
  const auto& res = P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(std::string(res.err) == "Missing (");
}

TEST_CASE("Use integer fallback unit") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-1:24.2.1(230101120000W)(00012*dm3)\r\n"
                    "1-0:14.7.0(50*Hz)\r\n"
                    "!";
  ParsedData<gas_delivered, frequency> data;
  P1Parser::parse(&data, msg, std::size(msg), /*unknown_error=*/false, /*check_crc=*/false);
  REQUIRE(data.gas_delivered == 0.012f);
  REQUIRE(data.frequency == 0.05f);
}

TEST_CASE("AveragedFixedField works properly for a long array") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-0:98.1.0(11)(1-0:1.6.0)(1-0:1.6.0)(230101000000W)(221206183000W)(06.134*kW)(230201000000W)(230127174500W)(05.644*kW)(230301000000W)("
                    "230226063000W)(04.895*kW)(230401000000S)(230305181500W)(04.879*kW)(230501000000S)(230416094500S)(04.395*kW)(230601000000S)(230522084500S)("
                    "03.242*kW)(230701000000S)(230623053000S)(01.475*kW)(230801000000S)(230724060000S)(02.525*kW)(230901000000S)(230819174500S)(02.491*kW)("
                    "231001000000S)(230911063000S)(02.342*kW)(231101000000W)(231031234500W)(02.048*kW)\r\n"
                    "!";

  ParsedData<active_energy_import_maximum_demand_last_13_months> data;
  P1Parser::parse(&data, msg, std::size(msg), /* unknown_error */ true, /* check_crc */ false);

  REQUIRE(data.active_energy_import_maximum_demand_last_13_months.val() == 3.642f);
}

TEST_CASE("AveragedFixedField works properly for an empty array") {
  const auto& msg = "/KMP5 ZABF000000000000\r\n"
                    "0-0:98.1.0(0)(garbage that will be skipped)\r\n"
                    "1-0:1.8.1(000001.000*kwh)\r\n"
                    "!";

  ParsedData<active_energy_import_maximum_demand_last_13_months, energy_delivered_tariff1> data;
  P1Parser::parse(&data, msg, std::size(msg), /* unknown_error */ true, /* check_crc */ false);

  REQUIRE(data.active_energy_import_maximum_demand_last_13_months.val() == 0.0f);
  REQUIRE(data.energy_delivered_tariff1.val() == 1.0f);
}
