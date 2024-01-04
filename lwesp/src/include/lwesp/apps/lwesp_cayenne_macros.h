/**
 * \file            lwesp_cayenne_macros.h
 * \brief           MQTT Cayenne list of macros
 */

/*
 * Copyright (c) 2024 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.1.2-dev
 */

#if 0
/*****************************************************************/
/* List of different data types, constants and respective values */
/*****************************************************************/
/* Actuators */
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Analog actuator",           ANALOG_ACTUATOR,        "analog_actuator")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Digital actuator",          DIGITAL_ACTUATOR,       "digital_actuator")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("HVAC.Change State",         HVAC_CHANGE_STATE,      "hvac_state")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("HVAC.Change Temperature",   HVAC_CHANGE_TEMP,       "hvac_temp")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("HVAC.Off/On",               HVAC_OFF_ON,            "hvac_off_on")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Light Switch",              LIGHT_SWITCH_ACT,       "lt_switch_act")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Lighting.Color",            LIGHTING_COLOR,         "lt_color")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Lighting.Luminosity",       LIGHTING_LUMINOSITY,    "lt_lum")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Motor",                     MOTOR,                  "motor")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Relay",                     RELAY,                  "relay")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Switch",                    SWITCH,                 "switch")
LWESP_CAYENNE_DATA_TYPE_CONSTANT_VALUE("Valve",                     VALVE,                  "valve")

/* Sensors */

/*****************************************************************/
/* List of different data units, constants and respective values */
/*****************************************************************/
/* Copied from actuators first */
LWESP_CAYENNE_DATA_TYPE_DEFINE("Analog", ANALOG, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Digital (0/1)", DIGITAL, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("State, User defined", NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Fahrenheit", FAHRENHEIT, "f")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Celsius", CELSIUS, "c")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Off/On", OFF_ON, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Low/High", LOW_HIGH, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Hexadecimal", HEX, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lux", LUX, "lux")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Volts", VOLTS, "v")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ratio", RATIO, "r")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Degree Angle", DEGREE, "deg")

/* Added sensors then */
#endif

/* Fast Copy/paste from Excel */
LWESP_CAYENNE_DATA_TYPE_DEFINE("Analog Actuator", ANALOG_ACTUATOR, "analog_actuator", "Analog", ANALOG, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Digital Actuator", DIGITAL_ACTUATOR, "digital_actuator", "Digital (0/1)", DIGITAL, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("HVAC.Change State", HVAC_CHANGE_STATE, "hvac_state", "State", USER_DEFINED, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("HVAC.Change Temperature", HVAC_CHANGE_TEMP, "hvac_temp", "Fahrenheit", FAHRENHEIT, "f")
LWESP_CAYENNE_DATA_TYPE_DEFINE("HVAC.Change Temperature", HVAC_CHANGE_TEMP, "hvac_temp", "* Celsius", CELSIUS, "c")
LWESP_CAYENNE_DATA_TYPE_DEFINE("HVAC.Off/On", HVAC_OFF_ON, "hvac_off_on", "Off/On", OFF_ON, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Light Switch", LIGHT_SWITCH_ACT, "lt_switch_act", "* Off/On", OFF_ON, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Light Switch", LIGHT_SWITCH_ACT, "lt_switch_act", "Digital (0/1)", DIGITAL, "d")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Light Switch", LIGHT_SWITCH_ACT, "lt_switch_act", "Low/High", LOW_HIGH, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting.Color", LIGHTING_COLOR, "lt_color", "Hexadecimal", HEX, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting.Luminosity", LIGHTING_LUMINOSITY, "lt_lum", "* % (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting.Luminosity", LIGHTING_LUMINOSITY, "lt_lum", "Lux", LUX, "lux")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting.Luminosity", LIGHTING_LUMINOSITY, "lt_lum", "Volts", VOLTS, "v")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting.Luminosity", LIGHTING_LUMINOSITY, "lt_lum", "Ratio", RATIO, "r")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Motor", MOTOR, "motor", "* Off/On", OFF_ON, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Motor", MOTOR, "motor", "Low/High", LOW_HIGH, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Motor", MOTOR, "motor", "Degree Angle", DEGREE, "deg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Relay", RELAY, "relay", "* Off/On", OFF_ON, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Relay", RELAY, "relay", "Low/High", LOW_HIGH, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Switch", SWITCH, "switch", "* Off/On", OFF_ON, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Switch", SWITCH, "switch", "Low/High", LOW_HIGH, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Switch", SWITCH, "switch", "Digital (0/1)", DIGITAL, "d")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Valve", VALVE, "valve", "* Off/On", OFF_ON, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Valve", VALVE, "valve", "Low/High", LOW_HIGH, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Valve", VALVE, "valve", "Digital (0/1)", DIGITAL, "d")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Digital Sensor", DIGITAL_SENSOR, "digital_sensor", "Digital (0/1)", DIGITAL, "d")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Analog Sensor", ANALOG_SENSOR, "analog_sensor", "Analog", ANALOG, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Absolute Humidity", ABSOLUTE_HUMIDITY, "abs_hum", "Grams per cubic meter",
                               GRAMS_PER_METER3, "gm3")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Absorbed Radiation", ABSORBED_RADIATION, "absrb_rad", "* Rad", RAD, "rad")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Absorbed Radiation", ABSORBED_RADIATION, "absrb_rad", "Gray", GRAY, "gy")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Acceleration.gx axis", ACCELERATION_GX, "gx", "Meters per second squared",
                               METER_PER_SEC_SQ, "ms2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Acceleration.gy axis", ACCELERATION_GY, "gy", "Meters per second squared",
                               METER_PER_SEC_SQ, "ms2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Acceleration.gz axis", ACCELERATION_GZ, "gz", "Meters per second squared",
                               METER_PER_SEC_SQ, "ms2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Altitude", ALTITUDE, "alt", "* Meters above sea level", METER, "m")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Altitude", ALTITUDE, "alt", "Feet above sea level", FEET, "ft")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Amount of substance", AMOUNT_SUBSTANCE, "amount", "Mole", MOLE, "mol")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Area", AREA, "area", "Square meter", METER2, "m2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Barometric pressure", BAROMETRIC_PRESSURE, "bp", "Pascal", PASCAL, "pa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Barometric pressure", BAROMETRIC_PRESSURE, "bp", "* Hecto Pascal", HECTOPASCAL, "hpa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Battery", BATTERY, "batt", "* % (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Battery", BATTERY, "batt", "Ratio", RATIO, "r")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Battery", BATTERY, "batt", "Volts", VOLTS, "v")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Biometric", BIOMETRIC, "bio", "Byte Array", BYTE_ARRAY, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Blood Count", BLOOD, "blood", "* Cells by cubic millimeter", CELLS_MM3, "cmm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Blood Count", BLOOD, "blood", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Bytes", BYTES, "bytes", "Bits", BIT, "bit")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Bytes", BYTES, "bytes", "* Bytes", BYTE, "byte")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Bytes", BYTES, "bytes", "Kilobytes", KB_BYTE, "kb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Bytes", BYTES, "bytes", "Megabytes", MB_BYTE, "mb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Bytes", BYTES, "bytes", "Gigabytes", GB_BYTE, "gb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Bytes", BYTES, "bytes", "Terabytes", TB_BYTE, "tb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Capacitance", CAPACITANCE, "cap", "Farad", FARAD, "farad")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Carbon Dioxide", CO2, "co2", "* Parts per milliion", PPM, "ppm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Carbon Dioxide", CO2, "co2", "Units of Micromole", UNITS_MICROMOLE, "wmoco2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Charge", CHARGE, "charge", "Coulomb", COULOMB, "q")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Cholesterol", CHOLESTEROL, "chol", "Millimoles/liter", MMOL_L, "mmol")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Cholesterol", CHOLESTEROL, "chol", "* Milligrams/deciliter", MG_DL, "mgdl")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Color", COLOR, "color", "* RGB", RGB, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Color", COLOR, "color", "CYMK", CYMK, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Color", COLOR, "color", "Hexadecimal", HEX, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Conductance", CONDUCTANCE, "conduct", "Siemen", SIEMEN, "s")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Counter", COUNTER, "counter", "Analog", ANALOG, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("CPU", CPU, "cpu", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Current", CURRENT, "current", "Ampere", AMP, "a")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Current density", CURRENT_DENSITY, "current_density", "Ampere per squre meter",
                               AMP_2_METER, "am2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Density", DENSITY, "density", "Kilograms per cubic meter", KGM3, "kgm3")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Effective Radiation", EFFECTIVE_RADATION, "eff_rad", "Roentgen", ROENTGEN, "roent")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Effective Radiation", EFFECTIVE_RADATION, "eff_rad", "Sievert", SIEVERT, "sv")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Effective Radiation", EFFECTIVE_RADATION, "eff_rad", "SieVert per Hour", SIEVERT_HOUR,
                               "svph")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Energy", ENERGY, "energy", "Killowatts per hour", KW_PER_H, "kwh")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Frequency", FREQUENCY, "freq", "Hertz", HERTZ, "hz")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gas", GAS, "gas", "* Pascal", PASCAL, "pa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gas", GAS, "gas", "Cubic meters", METER3, "m3")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gas", GAS, "gas", "Kilograms per cubic meter", KGM3, "kgm3")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Glucose", GLUCOSE, "glucose", "Millimoles/liter", MMOL_L, "mmol")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Glucose", GLUCOSE, "glucose", "* Milligrams/deciliter", MG_DL, "mgdl")
LWESP_CAYENNE_DATA_TYPE_DEFINE("GPS", GPS, "gps", "* Global Positioning System", GPS, "gps")
LWESP_CAYENNE_DATA_TYPE_DEFINE("GPS", GPS, "gps", "Universal Transverse Mercator", UTM, "utm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gravity.x axis", GRAVITY_X, "grav_x", "Newtons per kilogram", NEWTON_PER_KG, "nkg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gravity.x axis", GRAVITY_X, "grav_x", "* Meters per second squared", METER_PER_SEC_SQ,
                               "ms2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gravity.y axis", GRAVITY_Y, "grav_y", "Newtons per kilogram", NEWTON_PER_KG, "nkg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gravity.y axis", GRAVITY_Y, "grav_y", "* Meters per second squared", METER_PER_SEC_SQ,
                               "ms2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gravity.z axis", GRAVITY_Z, "grav_z", "Newtons per kilogram", NEWTON_PER_KG, "nkg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gravity.z axis", GRAVITY_Z, "grav_z", "* Meters per second squared", METER_PER_SEC_SQ,
                               "ms2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gyroscope.rate of rotation around x axis", GYRO_X, "gyro_x", "* Rotation speed",
                               ROTATION, "rot")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gyroscope.rate of rotation around x axis", GYRO_X, "gyro_x",
                               "Meters per second squared", METER_PER_SEC_SQ, "mps2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gyroscope.rate of rotation around y axis", GYRO_Y, "gyro_y", "* Rotation speed",
                               ROTATION, "rot")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gyroscope.rate of rotation around y axis", GYRO_Y, "gyro_y",
                               "Meters per second squared", METER_PER_SEC_SQ, "mps2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gyroscope.rate of rotation around z axis", GYRO_Z, "gyro_z", "* Rotation speed",
                               ROTATION, "rot")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Gyroscope.rate of rotation around z axis", GYRO_Z, "gyro_z",
                               "Meters per second squared", METER_PER_SEC_SQ, "mps2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("HVAC.Humdity", HVAC_HUMIDITY, "hvac_hum", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Image", IMAGE, "image", "Byte Array", BYTE_ARRAY, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Impedance", IMPEDANCE, "imped", "Ohm", OHM, "ohm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Inductance", INDUCTANCE, "induct", "Henry", HENRY, "h")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ink Levels.Black", INK_BLACK, "ink_blk", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ink Levels.Cyan", INK_CYAN, "ink_cya", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ink Levels.Magenta", INK_MEGENTA, "ink_mag", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ink Levels.Yellow", INK_YELLOW, "ink_yel", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Intrusion", INTRUSION, "intrusion", "Digital (0/1)", DIGITAL, "d")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ionizing Radiation", IONIZING_RADIATION, "ion_rad", "* Electron Volts", ELECTRON_VOLT,
                               "ev")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ionizing Radiation", IONIZING_RADIATION, "ion_rad", "Ergs", ERGS, "erg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ionizing Radiation", IONIZING_RADIATION, "ion_rad", "Joules", JOULE, "j")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Length", LENGTH, "len", "* Meter", METER, "m")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Length", LENGTH, "len", "Digital (0/1)", DIGITAL, "d")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Length", LENGTH, "len", "Low/High", LOW_HIGH, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting", LIGHTING_SENSE, "lighting_sense", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting", LIGHTING_SENSE, "lighting_sense", "* Lux", LUX, "lux")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting", LIGHTING_SENSE, "lighting_sense", "Volts", VOLTS, "v")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Lighting", LIGHTING_SENSE, "lighting_sense", "Ratio", RATIO, "r")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Linear Acceleration.x axis", LINEAR_ACCEL_X, "lin_acc_x", "Meters per second squared",
                               METER_PER_SEC_SQ, "mps2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Linear Acceleration.y axis", LINEAR_ACCEL_Y, "lin_acc_y", "Meters per second squared",
                               METER_PER_SEC_SQ, "mps2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Linear Acceleration.z axis", LINEAR_ACCEL_Z, "lin_acc_z", "Meters per second squared",
                               METER_PER_SEC_SQ, "mps2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Liquid", LIQUID, "liquid", "* Liter", LITER, "l")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Liquid", LIQUID, "liquid", "Gallon", GALLON, "gal")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Liquid", LIQUID, "liquid", "Ounce", OUNCE, "oz")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Liquid", LIQUID, "liquid", "Cubic centimeter", CUBIC_CENT, "cc")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Location.Latitude", LOCATION_LAT, "loc_lat", "Latitude", LATITUDE, "lat")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Location.Longitude", LOCATION_LONG, "loc_lon", "Longitude", LONGITUDE, "long")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Luminosity", LUMINOSITY, "lum", "* Lux", LUX, "lux")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Luminosity", LUMINOSITY, "lum", "Volts", VOLTS, "v")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Luminosity", LUMINOSITY, "lum", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Luminosity", LUMINOSITY, "lum", "Ratio", RATIO, "r")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Magnetic field strength H", MAGNETIC_STRENGTH, "mag_str", "Amperes per meter",
                               AMP_METER, "ampm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Magnetic field.x axis", MAGNETIC_AXIS_X, "mag_x", "Tesla", TESLA, "tesla")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Magnetic field.y axis", MAGNETIC_AXIS_Y, "mag_y", "Tesla", TESLA, "tesla")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Magnetic field.z axis", MAGNETIC_AXIS_Z, "mag_z", "Tesla", TESLA, "tesla")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Magnetic flux density B", MAGNETIC_FLUX_DENSITY, "mag_flux", "Newton-meters per ampere",
                               NEWTON_METERS_AMP, "nma")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Mass", MASS, "mass", "Kilogram", KILOGRAM, "kg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Memory", MEMORY, "mem", "Kilobytes", KB_BYTE, "kb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Memory", MEMORY, "mem", "* Megabytes", MB_BYTE, "mb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Memory", MEMORY, "mem", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Motion", MOTION, "motion", "Digital (0/1)", DIGITAL, "d")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Oil", OIL, "oil", "Oil Barrel", BARREL, "bbl")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Oil", OIL, "oil", "* gallon", GALLON, "gal")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Oil", OIL, "oil", "liter", LITER, "l")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Orientation.Azimuth", ORIENT_AZIMUTH, "ori_azim", "Degree Angle", DEGREE, "deg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Orientation.Pitch", ORIENT_PITCH, "ori_pitch", "Degree Angle", DEGREE, "deg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Orientation.Roll", ORIENT_ROLL, "ori_roll", "Degree Angle", DEGREE, "deg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("pH-Acidity", ACIDITY, "acid", "Acidity", ACIDITY, "acid")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Power", POWER, "pow", "Watts", WATT, "w")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pollution.Nitrogen", POLLUTION_NO2, "no2", "Nitrogen dioxide", NO2, "no2")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pollution.Ozone", POLLUTION_O3, "o3", "Ozone", O3, "o3")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pressure", PRESSURE, "press", "* Pascal", PASCAL, "pa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pressure", PRESSURE, "press", "Hecto Pascal", HECTOPASCAL, "hpa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pressure", PRESSURE, "press", "Bar", BAR, "bar")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pressure", PRESSURE, "press", "Technical atmosphere", TECH_ATMO, "at")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pressure", PRESSURE, "press", "Standard atmosphere", STD_ATMO, "atm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pressure", PRESSURE, "press", "Torr", TORR, "torr")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Pressure", PRESSURE, "press", "Pounds per square inch", PSI, "psi")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Proximity", PROXIMITY, "prox", "* Centimeter", CENTIMETER, "cm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Proximity", PROXIMITY, "prox", "Meter", METER, "m")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Proximity", PROXIMITY, "prox", "Digital (0/1)", DIGITAL, "d")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Radioactivity", RADIOACTIVITY, "rad", "Becquerel", BECQUEREL, "bq")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Radioactivity", RADIOACTIVITY, "rad", "* Curie", CURIE, "ci")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Radiation Exposure", EXPOSURE_RADIATION, "expo_rad", "* Roentgen", ROENTGEN, "roent")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Radiation Exposure", EXPOSURE_RADIATION, "expo_rad", "Coulomb/Kilogram", COULOMB_PER_KG,
                               "ckg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rain Level", RAIN_LEVEL, "rain_level", "Centimeter", CENTIMETER, "cm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rain Level", RAIN_LEVEL, "rain_level", "* Millimeter", MILLIMETER, "mm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Relative Humidity", RELATIVE_HUMIDITY, "rel_hum", "* % (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Relative Humidity", RELATIVE_HUMIDITY, "rel_hum", "Ratio", RATIO, "r")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Resistance", RESISTANCE, "res", "Ohm", OHM, "ohm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rotation", ROTATION, "rot", "Revolutions per minute", RPM, "rpm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rotation", ROTATION, "rot", "* Revolutions per second", RPMS, "rpms")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rotation", ROTATION, "rot", "Radians per second", RAD, "radianps")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rotation Vector.scalar", ROTATION_SCALAR, "rot_scal", "Cos(0/2)", ROT_SCAL, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rotation Vector.x axis", ROTATION_X, "rot_x", "X * sin (0/2)", ROT_X, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rotation Vector.y axis", ROTATION_Y, "rot_y", "Y * sin (0/2)", ROT_Y, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Rotation Vector.z axis", ROTATION_Z, "rot_z", "Z * sin (0/2)", ROT_Z, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Seismometer", SEISMOMETER, "seis", "Microns (micrometers) /second,", MICROS_PER_SEC,
                               "micps")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Seismometer", SEISMOMETER, "seis", "* Volts", VOLTS, "v")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Seismometer", SEISMOMETER, "seis", "Spectral Amplitude", CM_HERTZ, "cmhz")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Signal Noise Ratio", SNR, "snr", "Decibels", DB, "db")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Signal Strength", SIGNAL_STRENGTH, "sig_str", "Decibels per milliwatt", DBM, "dbm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Smoke", SMOKE, "smoke", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Smoke", SMOKE, "smoke", "Photodiode", PHOTODIODE, "pz")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Smoke", SMOKE, "smoke", "* Kiloelectron Volts", KILOELEC_VOLT, "kev")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Soil Moisture", SOIL_MOISTURE, "soil_moist", "% (0 to 100)", PERCENT, "p")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Soil pH", SOIL_PH, "soil_ph", "Analog", ANALOG, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Soil Water Tension", SOIL_WATER_TENSION, "soil_w_ten", "* Kilopascal", KILOPASCAL,
                               "kpa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Soil Water Tension", SOIL_WATER_TENSION, "soil_w_ten", "Pascal", PASCAL, "pa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Solid Volume", SOLID_VOLUME, "solid_vol", "Cubic meter", CUBIC_METER, "m3")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Sound", SOUND, "sound", "Decibels per milliwatt", DBM, "dbm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Specific Humidity", SPECIFIC_HUMIDITY, "spec_hum", "Grams/Kilograms", G_PER_KG, "gkg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Speed", SPEED, "speed", "Kilometer per hour", KM_PER_H, "kmh")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Speed", SPEED, "speed", "* Miles per hour", MPH, "mph")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Steps", STEPS, "steps", "Steps", STEPS, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Storage", STORAGE, "storage", "Bytes", BYTE, "byte")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Storage", STORAGE, "storage", "Kilobytes", KB_BYTE, "kb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Storage", STORAGE, "storage", "* Megabytes", MB_BYTE, "mb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Storage", STORAGE, "storage", "Gigabytes", GB_BYTE, "gb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Storage", STORAGE, "storage", "Terabytes", TB_BYTE, "tb")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Stress", STRESS, "stress", "Stress", PASCAL, "pa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Stress", STRESS, "stress", "* Hecto Pascal", HECTOPASCAL, "hpa")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Stress", STRESS, "stress", "Pounds per square inch", PSI, "psi")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Tank Level", TANK_LEVEL, "tl", "Analog", ANALOG, NULL)
LWESP_CAYENNE_DATA_TYPE_DEFINE("Temperature", TEMPERATURE, "temp", "Fahrenheit", FAHRENHEIT, "f")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Temperature", TEMPERATURE, "temp", "* Celsius", CELSIUS, "c")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Temperature", TEMPERATURE, "temp", "Kelvin", KELVIN, "k")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Time", TIME, "time", "* Second", SECOND, "sec")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Time", TIME, "time", "Milliseconds", MS, "msec")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Time", TIME, "time", "minute", MINUTE, "min")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Time", TIME, "time", "hour", HOUR, "hour")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Time", TIME, "time", "day", DAY, "day")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Time", TIME, "time", "month", MONTH, "month")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Time", TIME, "time", "year", YEAR, "year")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Torque", TORQUE, "torq", "* Newton-meter", NEWTONMETER, "newtm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Torque", TORQUE, "torq", "Joule", JOULE, "j")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Turbidity", TURBIDITY, "turb", "Nephelometric Turbidity Unit", NTU, "ntu")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Turbidity", TURBIDITY, "turb", "* Formazin Turbidity Unit", FTU, "ftu")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Ultrasonic", ULTRASONIC, "ultra", "Kilohertz", KHZ, "khz")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Velocity", VELOCITY, "velo", "Meters per second squared", METER_PER_SEC, "mps")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Viscosity", VISCOSITY, "visco", "Millipascal-second", MILLIPASCAL_SEC, "mpas")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Voltage", VOLTAGE, "voltage", "* Volts", VOLTS, "v")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Voltage", VOLTAGE, "voltage", "Millivolts", MILLIVOLTS, "mv")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Volume", VOLUME, "vol", "Cubic meter", CUBIC_METER, "m3")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Water", WATER, "h20", "* Gallons per minute", GPM, "gpm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Water", WATER, "h20", "Cubic feet per second", CUBIC_FEET_SEC, "cfs")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Wavelength", WAVELENGTH, "wave", "Meters", METER, "m")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Weight", WEIGHT, "weight", "* Pounds", POUND, "lbs")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Weight", WEIGHT, "weight", "Kilogram", KILOGRAM, "kg")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Received signal strength indicator", RSSI, "rssi", "RSSI", DBM, "dbm")
LWESP_CAYENNE_DATA_TYPE_DEFINE("Wind Speed", WIND_SPEED, "wind_speed", "Kilometer per hour", KM_PER_H, "kmh")

#undef LWESP_CAYENNE_DATA_TYPE_DEFINE