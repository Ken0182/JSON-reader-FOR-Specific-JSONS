#include "json.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <variant>
#include <set> // For tracking keys
#include <iomanip> // For setprecision
#include <sstream> // For stringstream
#include <ctime> // For time functions

using namespace std;
using json = nlohmann::json;

// Helper: convert string to lowercase
string lower(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return tolower(c); });
    return out;
}

// Helper: split string by delimiter
vector<string> split(const string& s, char delim) {
    vector<string> result;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

// Defensive: get vector<string>
vector<string> getStringVec(const json& j, const string& ctx = "") {
    vector<string> out;
    if (j.is_array()) {
        for (size_t i = 0; i < j.size(); ++i) {
            if (j[i].is_string()) out.push_back(j[i].get<string>());
            else if (j[i].is_number()) out.push_back(to_string(j[i].get<float>()));
            else cerr << "[TypeError] Non-string in string array at " << ctx << "[" << i << "]: " << j[i] << "\n";
        }
    } else if (j.is_string()) {
        out.push_back(j.get<string>());
    } else if (j.is_number()) {
        out.push_back(to_string(j.get<float>()));
    }
    return out;
}

// Enhanced: parse float or string with units (ms, s, Hz)
// Defensive: Get float or 0 if not parsable, print errors
float getFlexibleFloat(const json& j, const string& ctx = "") {
    try {
        if (j.is_number()) return j.get<float>();
        if (j.is_array() && j.size() > 0 && j[0].is_number()) return j[0].get<float>(); // Use first element
        if (j.is_string()) {
            string val = j.get<string>();
            size_t pos;
            if ((pos = val.find("ms")) != string::npos)
                return stof(val.substr(0, pos));
            if ((pos = val.find("s")) != string::npos && val.find("ms") == string::npos)
                return stof(val.substr(0, pos)) * 1000.0f;
            if ((pos = val.find("Hz")) != string::npos)
                return stof(val.substr(0, pos));
            // Allow some "AI-driven" etc to parse as zero
            vector<string> allowed = {"AI-dynamic", "AI-driven", "random", "automated"};
            if (find(allowed.begin(), allowed.end(), val) != allowed.end())
                return 0.0f;
            return stof(val);
        }
        if (j.is_null()) return 0.0f;
    } catch (...) {
        cerr << "[TypeError] Can't parse float in field " << ctx << ": " << j << endl;
    }
    cerr << "[TypeError] Can't parse float (not number/string) at " << ctx << ": " << j << endl;
    return 0.0f;
}

// Defensive: get vector<float> or empty if not all numbers
vector<float> getFloatVec(const json& j, const string& ctx = "") {
    vector<float> out;
    if (j.is_array()) {
        for (size_t i = 0; i < j.size(); ++i) {
            try {
                out.push_back(getFlexibleFloat(j[i], ctx + "[" + to_string(i) + "]"));
            } catch (...) {
                cerr << "[TypeError] Can't parse float in array at " << ctx << "[" << i << "]: " << j[i] << "\n";
            }
        }
    } else {
        try {
            out.push_back(getFlexibleFloat(j, ctx));
        } catch (...) {
            cerr << "[TypeError] Can't parse float (single) at " << ctx << ": " << j << "\n";
        }
    }
    return out;
}

// Get string or float as string
string getStringOrFloat(const json& j) {
    if (j.is_string()) {
        return j.get<string>();
    }
    if (j.is_number()) {
        return to_string(j.get<float>());
    }
    return "";
}

// Join vector<string> with delimiter
string join(const vector<string>& v, const string& delim = ",") {
    string s;
    for (size_t i = 0; i < v.size(); ++i) {
        s += v[i];
        if (i != v.size() - 1) {
            s += delim;
        }
    }
    return s;
}

// Range struct for handling [min, max] or single values
struct Range {
    float min, max;
    void from_json(const json& j) {
        if (j.is_array() && j.size() == 2)
            min = getFlexibleFloat(j[0]), max = getFlexibleFloat(j[1]);
        else
            min = max = getFlexibleFloat(j);
    }
    json to_json() const { return (min == max) ? json(min) : json::array({min, max}); }
};

inline void to_json(json& j, const Range& r) { j = r.to_json(); }
inline void from_json(const json& j, Range& r) { r.from_json(j); }

// SoundCharacteristics struct
struct SoundCharacteristics {
    string timbral, material, dynamic;
    vector<pair<string, float>> emotional;

    void from_json(const json& j) {
        if (j.is_object()) {
            if (j.contains("timbral") && j["timbral"].is_string())
                timbral = j["timbral"].get<string>();

            if (j.contains("material") && j["material"].is_string())
                material = j["material"].get<string>();

            if (j.contains("dynamic") && j["dynamic"].is_string())
                dynamic = j["dynamic"].get<string>();

            if (j.contains("emotional") && j["emotional"].is_array()) {
                emotional.clear(); // Clear previous data
                for (const auto& emotion : j["emotional"]) {
                    if (emotion.is_string()) {
                        emotional.emplace_back(emotion.get<string>(), 1.0f);
                    } else if (emotion.is_object() && emotion.contains("tag") && emotion["tag"].is_string()) {
                        emotional.emplace_back(emotion["tag"].get<string>(), emotion.value("weight", 1.0f));
                    } else {
                        cerr << "[TypeError] Invalid emotional entry: " << emotion << endl;
                    }
                }
            }
        } else {
            cerr << "[TypeError] Expected object for SoundCharacteristics, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        json j;
        j["timbral"] = timbral;
        j["material"] = material;
        j["dynamic"] = dynamic;

        json emo = json::array();
        for (const auto& [tag, weight] : emotional) {
            emo.push_back({{"tag", tag}, {"weight", weight}});
        }
        j["emotional"] = emo;

        return j;
    }

    // Added the missing getEmotionalTags() method
    vector<string> getEmotionalTags() const {
        vector<string> tags;
        for (const auto& [tag, _] : emotional) {
            tags.push_back(tag);
        }
        return tags;
    }
};


// TopologicalMetadata struct
struct TopologicalMetadata {
    string damping, spectralComplexity, manifoldPosition;

    void from_json(const json& j) {
        if (j.is_object()) {
            if (j.contains("damping") && j["damping"].is_string()) damping = j["damping"].get<string>();
            if (j.contains("spectral_complexity") && j["spectral_complexity"].is_string()) spectralComplexity = j["spectral_complexity"].get<string>();
            if (j.contains("manifold_position") && j["manifold_position"].is_string()) manifoldPosition = j["manifold_position"].get<string>();
        } else {
            cerr << "[TypeError] Expected object for TopologicalMetadata, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        json j;
        j["damping"] = damping;
        j["spectral_complexity"] = spectralComplexity;
        j["manifold_position"] = manifoldPosition;
        return j;
    }
};

// ParamMeta for schema/validation/UI
struct ParamMeta {
    string displayName;
    float minVal = 0.0f, maxVal = 1.0f;
    string units = "";
    string description = ""; // Optional for docs
    bool required = false;
    string paramType = "float"; // e.g., "float", "bool", "string", "vector<float>", "vector<string>"

    void from_json(const json& j) {
        if (j.is_object()) {
            if (j.contains("displayName") && j["displayName"].is_string()) displayName = j["displayName"].get<string>();
            if (j.contains("minVal") && j["minVal"].is_number()) minVal = j["minVal"].get<float>();
            if (j.contains("maxVal") && j["maxVal"].is_number()) maxVal = j["maxVal"].get<float>();
            if (j.contains("units") && j["units"].is_string()) units = j["units"].get<string>();
            if (j.contains("description") && j["description"].is_string()) description = j["description"].get<string>();
            if (j.contains("required") && j["required"].is_boolean()) required = j["required"].get<bool>();
            if (j.contains("paramType") && j["paramType"].is_string()) paramType = j["paramType"].get<string>();
        } else {
            cerr << "[TypeError] Expected object for ParamMeta, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        json j;
        j["displayName"] = displayName;
        j["minVal"] = minVal;
        j["maxVal"] = maxVal;
        j["units"] = units;
        j["description"] = description;
        j["required"] = required;
        j["paramType"] = paramType;
        return j;
    }
};

// BaseParamStruct: Map-based for params, with schema, validation, and discovery
struct BaseParamStruct {
    map<string, float> floatParams;
    map<string, bool> boolParams;
    map<string, string> stringParams;
    map<string, vector<float>> vectorParams;
    map<string, vector<string>> stringVectorParams; // For vector<string> like "types"
    map<string, variant<float, bool, string, vector<float>, vector<string>>> unifiedParams; // Updated unified for string vectors

    map<string, ParamMeta> paramSchema; // Schema for validation/UI, now versioned

    // Static registry for types (e.g., FX) (C. Dynamic Registration)
    static map<string, map<string, ParamMeta>> registeredSchemas;
    static string schemaVersion; // Versioned schema

    static void registerSchema(const string& type, const map<string, ParamMeta>& schema) {
        registeredSchemas[type] = schema;
    }

    // Helper getters with defaults (added getStringVector)
    float getFloat(const string& key, float defaultVal = 0.0f) const {
        auto it = floatParams.find(key);
        return (it != floatParams.end()) ? it->second : defaultVal;
    }

    bool getBool(const string& key, bool defaultVal = false) const {
        auto it = boolParams.find(key);
        return (it != boolParams.end()) ? it->second : defaultVal;
    }

    string getString(const string& key, const string& defaultVal = "") const {
        auto it = stringParams.find(key);
        return (it != stringParams.end()) ? it->second : defaultVal;
    }

    vector<float> getVector(const string& key, const vector<float>& defaultVal = {}) const {
        auto it = vectorParams.find(key);
        return (it != vectorParams.end()) ? it->second : defaultVal;
    }

    vector<string> getStringVector(const string& key, const vector<string>& defaultVal = {}) const {
        auto it = stringVectorParams.find(key);
        return (it != stringVectorParams.end()) ? it->second : defaultVal;
    }

    // Runtime type detection, storage, validation/clamping (D. In-Place Validation)
    void storeParam(const string& key, const json& val, const string& ctx = "", const string& type = "") {
        if (!val.is_null() && !val.is_object()) { // Avoid storing nested objects as params
            ParamMeta meta;
            bool hasMeta = false;
            if (paramSchema.count(key)) {
                meta = paramSchema[key];
                hasMeta = true;
            } else if (!type.empty() && registeredSchemas.count(type) && registeredSchemas[type].count(key)) {
                meta = registeredSchemas[type][key];
                hasMeta = true;
            } else {
                // Auto-discovery for new param
                meta.paramType = val.is_number() ? "float" : val.is_boolean() ? "bool" : val.is_string() ? "string" : val.is_array() ? (val[0].is_string() ? "vector<string>" : "vector<float>") : "unknown";
                paramSchema[key] = meta;
                cerr << "[AutoDiscovery] New param '" << key << "' detected at " << ctx << ". Schema update suggested: Define displayName/units/required for type " << meta.paramType << endl;
            }

            // Type-specific storage with checks
            if (val.is_number_float() && meta.paramType == "float") {
                float v = val.get<float>();
                if (hasMeta && meta.minVal != meta.maxVal) {
                    v = max(meta.minVal, min(v, meta.maxVal)); // Clamp
                }
                floatParams[key] = v;
                unifiedParams[key] = v;
            } else if (val.is_boolean() && meta.paramType == "bool") {
                boolParams[key] = val.get<bool>();
                unifiedParams[key] = val.get<bool>();
            } else if (val.is_string() && meta.paramType == "string") {
                stringParams[key] = val.get<string>();
                unifiedParams[key] = val.get<string>();
            } else if (val.is_array() && meta.paramType == "vector<float>") {
                vector<float> vec = getFloatVec(val, ctx);
                vectorParams[key] = vec;
                unifiedParams[key] = vec;
            } else if (val.is_array() && meta.paramType == "vector<string>") {
                vector<string> vec = getStringVec(val, ctx);
                stringVectorParams[key] = vec;
                unifiedParams[key] = vec;
            } else if (val.is_number_integer() && meta.paramType == "float") {
                float v = static_cast<float>(val.get<int>());
                if (hasMeta && meta.minVal != meta.maxVal) {
                    v = max(meta.minVal, min(v, meta.maxVal));
                }
                floatParams[key] = v;
                unifiedParams[key] = v;
            } else {
                cerr << "[TypeError] Type mismatch for key '" << key << "' at " << ctx << ": expected " << meta.paramType << ", got " << val.type_name() << " value: " << val << "\n";
            }
        }
    }

    // Serialize by looping over maps
    json paramsToJson() const {
        json j;
        for (const auto& [k, v] : floatParams) j[k] = v;
        for (const auto& [k, v] : boolParams) j[k] = v;
        for (const auto& [k, v] : stringParams) j[k] = v;
        for (const auto& [k, v] : vectorParams) j[k] = v;
        for (const auto& [k, v] : stringVectorParams) j[k] = v;
        return j;
    }

    // Load params by looping over JSON keys with strict checks and flagging
    void paramsFromJson(const json& j_obj, const string& type = "") {
        if (!j_obj.is_object()) {
            cerr << "[TypeError] Expected object for paramsFromJson, got " << j_obj.type_name() << endl;
            return;
        }
        set<string> handledKeys;
        for (auto& [key, val] : j_obj.items()) {
            if (key == "type") {
                handledKeys.insert(key);
                continue; // Skip type for params
            }
            if (val.is_null()) {
                handledKeys.insert(key);
                continue;
            }
            storeParam(key, val, type + "." + key, type);
            handledKeys.insert(key);
        }
        // Flag unhandled (unused in JSON but expected in schema)
        for (const auto& [schemaKey, meta] : paramSchema) {
            if (handledKeys.find(schemaKey) == handledKeys.end() && meta.required) {
                cerr << "[Warning] Missing required param '" << schemaKey << "' in " << type << ". Defaulting if possible." << endl;
                // Auto-complete default based on type
                if (meta.paramType == "float") floatParams[schemaKey] = 0.0f;
                else if (meta.paramType == "bool") boolParams[schemaKey] = false;
                else if (meta.paramType == "string") stringParams[schemaKey] = "";
                // ... extend for other types
            }
        }
        // Flag unknown (in JSON but not in schema)
        for (const auto& [jsonKey, _] : j_obj.items()) {
            if (handledKeys.find(jsonKey) != handledKeys.end() && paramSchema.find(jsonKey) == paramSchema.end() && jsonKey != "type") {
                cerr << "[Warning] Unknown field '" << jsonKey << "' in " << type << ". Stored but suggest schema update." << endl;
            }
        }
    }

    // E. Automated Parameter Discovery
    vector<string> getAllParamKeys() const {
        vector<string> keys;
        for (const auto& [k, _] : floatParams) keys.push_back(k);
        for (const auto& [k, _] : boolParams) keys.push_back(k);
        for (const auto& [k, _] : stringParams) keys.push_back(k);
        for (const auto& [k, _] : vectorParams) keys.push_back(k);
        for (const auto& [k, _] : stringVectorParams) keys.push_back(k);
        return keys;
    }
    

    // Schema loader (from JSON or code), now versioned
    void loadSchema(const json& j_schema) {
        if (j_schema.is_object()) {
            if (j_schema.contains("version") && j_schema["version"].is_string()) {
                schemaVersion = j_schema["version"].get<string>();
            }
            for (auto& [key, meta_json] : j_schema.items()) {
                if (key == "version") continue;
                if (meta_json.is_object()) {
                    ParamMeta meta;
                    meta.from_json(meta_json);
                    paramSchema[key] = meta;
                } else {
                    cerr << "[TypeError] Schema entry for " << key << " is not an object: " << meta_json << endl;
                }
            }
        } else {
            cerr << "[TypeError] Schema is not an object: " << j_schema << endl;
        }
    }

    json schemaToJson() const {
        json j;
        for (const auto& [key, meta] : paramSchema) {
            j[key] = meta.to_json();
        }
        j["version"] = schemaVersion;
        return j;
    }
};

// Static registry init
map<string, map<string, ParamMeta>> BaseParamStruct::registeredSchemas;
string BaseParamStruct::schemaVersion = "1.1"; // Initial version

// Oscillator derived from BaseParamStruct
struct Oscillator : public BaseParamStruct {
    void from_json(const json& j) {
        if (j.is_object()) {
            paramsFromJson(j);
        } else {
            cerr << "[TypeError] Expected object for Oscillator, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        return paramsToJson();
    }
};

// Envelope derived from BaseParamStruct
struct Envelope : public BaseParamStruct {
    void from_json(const json& j) {
        if (j.is_object()) {
            paramsFromJson(j);
        } else if (j.is_array()) { // Auto-infer compacted ADSR array, e.g. [0.1, 0.2, 0.5, 0.3]
            vector<string> adsrKeys = {"attack", "decay", "sustain", "release"};
            if (j.size() == 4) {
                for (size_t i = 0; i < j.size(); ++i) {
                    if (j[i].is_number()) {
                        floatParams[adsrKeys[i]] = j[i].get<float>();
                    }
                }
                cerr << "[AutoInfer] Compacted ADSR array detected—mapped to attack/decay/sustain/release." << endl;
            } else if (j.size() == 6) { // ADHSR
                vector<string> adhshrKeys = {"attack", "decay", "hold", "sustain", "release", "delay"};
                for (size_t i = 0; i < j.size(); ++i) {
                    if (j[i].is_number()) {
                        floatParams[adhshrKeys[i]] = j[i].get<float>();
                    }
                }
                cerr << "[AutoInfer] Compacted ADHSR array detected—mapped to attack/decay/hold/sustain/release/delay." << endl;
            } else {
                cerr << "[TypeError] Unknown envelope array length: " << j.size() << "—skipped." << endl;
            }
        } else {
            cerr << "[TypeError] Expected object or array for Envelope, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        return paramsToJson();
    }
};

// Filter derived from BaseParamStruct
struct Filter : public BaseParamStruct {
    void from_json(const json& j) {
        if (j.is_object()) {
            paramsFromJson(j);
        } else {
            cerr << "[TypeError] Expected object for Filter, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        return paramsToJson();
    }
};

// Fx derived from BaseParamStruct
struct Fx : public BaseParamStruct {
    string type;

    void from_json(const json& j) {
        if (j.is_object()) {
            if (j.contains("type") && j["type"].is_string()) {
                type = j["type"].get<string>();
            } else {
                type = "none";
                cerr << "[TypeError] Missing or non-string 'type' in Fx: " << j << endl;
            }
            paramsFromJson(j, type); // Pass type for schema lookup
        } else {
            cerr << "[TypeError] Expected object for Fx, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        json j = paramsToJson();
        j["type"] = type;
        return j;
    }
};

// Metadata remains as-is
struct Metadata {
    string description, namingConvention, version;

    void from_json(const json& j) {
        if (j.is_object()) {
            if (j.contains("description") && j["description"].is_string()) description = j["description"].get<string>();
            if (j.contains("naming_convention") && j["naming_convention"].is_string()) namingConvention = j["naming_convention"].get<string>();
            if (j.contains("version") && j["version"].is_string()) version = j["version"].get<string>();
        } else {
            cerr << "[TypeError] Expected object for Metadata, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        json j;
        j["description"] = description;
        j["naming_convention"] = namingConvention;
        j["version"] = version;
        return j;
    }
};

// GroupConfig updated with map-based structs
struct GroupConfig {
    string synthesisType;
    Oscillator oscillator;
    Envelope envelope;
    Filter filter;
    vector<Fx> fx;
    SoundCharacteristics soundCharacteristics;
    TopologicalMetadata topologicalMetadata;
    Metadata metadata;

    void from_json(const json& j) {
        if (j.is_object()) {
            if (j.contains("metadata") && j["metadata"].is_object()) metadata.from_json(j["metadata"]);
            if (j.contains("synthesis_type") && j["synthesis_type"].is_string()) synthesisType = j["synthesis_type"].get<string>();
            if (j.contains("oscillator") && j["oscillator"].is_object()) oscillator.from_json(j["oscillator"]);
            if (j.contains("envelope") && j["envelope"].is_object()) envelope.from_json(j["envelope"]);
            if (j.contains("filter") && j["filter"].is_object()) filter.from_json(j["filter"]);
            if (j.contains("fx") && j["fx"].is_array()) {
                fx.clear();
                for (const auto& fxItem : j["fx"]) {
                    if (fxItem.is_object()) {
                        Fx fxStruct;
                        fxStruct.from_json(fxItem);
                        fx.push_back(fxStruct);
                    } else {
                        cerr << "[TypeError] Fx item is not an object: " << fxItem << endl;
                    }
                }
            }
            if (j.contains("sound_characteristics") && j["sound_characteristics"].is_object()) soundCharacteristics.from_json(j["sound_characteristics"]);
            if (j.contains("topological_metadata") && j["topological_metadata"].is_object()) topologicalMetadata.from_json(j["topological_metadata"]);
        } else {
            cerr << "[TypeError] Expected object for GroupConfig, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        json j;
        j["metadata"] = metadata.to_json();
        j["synthesis_type"] = synthesisType;
        j["oscillator"] = oscillator.to_json();
        j["envelope"] = envelope.to_json();
        j["filter"] = filter.to_json();
        j["fx"] = json::array();
        for (const auto& fxItem : fx) {
            j["fx"].push_back(fxItem.to_json());
        }
        j["sound_characteristics"] = soundCharacteristics.to_json();
        j["topological_metadata"] = topologicalMetadata.to_json();
        return j;
    }
};

// GuitarParams derived from BaseParamStruct
struct GuitarParams : public BaseParamStruct {
    SoundCharacteristics soundCharacteristics;
    TopologicalMetadata topologicalMetadata;

    void from_json(const json& j) {
        if (j.is_object()) {
            paramsFromJson(j);
            if (j.contains("soundCharacteristics") && j["soundCharacteristics"].is_object()) soundCharacteristics.from_json(j["soundCharacteristics"]);
            if (j.contains("topologicalMetadata") && j["topologicalMetadata"].is_object()) topologicalMetadata.from_json(j["topologicalMetadata"]);
        } else {
            cerr << "[TypeError] Expected object for GuitarParams, got " << j.type_name() << endl;
        }
    }

    json to_json() const {
        json j = paramsToJson();
        j["soundCharacteristics"] = soundCharacteristics.to_json();
        j["topologicalMetadata"] = topologicalMetadata.to_json();
        return j;
    }
};

// SoundConfig updated (fixed "fxect" typo to "effects")
struct SoundConfig {
    string instrumentType;
    map<string, vector<string>> oscTypes;
    map<string, map<string, Range>> adsr;
    vector<Fx> effects; // Fixed from "fxect"
    bool useDynamicGate = false;
    float gateThreshold = 0.0f, gateDecaySec = 0.0f;
    string emotion;
    string topology;
    GuitarParams guitarParams;
    SoundCharacteristics soundCharacteristics;
    TopologicalMetadata topologicalMetadata;

    json to_json() const {
        json j;
        j["instrumentType"] = instrumentType;
        j["oscTypes"] = oscTypes;
        for (const auto& [context, params] : adsr) {
            for (const auto& [param, range] : params) {
                j["adsr"][context][param] = range.to_json();
            }
        }
        json fxArr = json::array();
        for (const auto& fx : effects) {
            fxArr.push_back(fx.to_json());
        }
        j["effects"] = fxArr; // Fixed from "fxect"
        j["useDynamicGate"] = useDynamicGate;
        j["gateThreshold"] = gateThreshold;
        j["gateDecaySec"] = gateDecaySec;
        j["emotion"] = emotion;
        j["topology"] = topology;
        j["guitarParams"] = guitarParams.to_json();
        j["soundCharacteristics"] = soundCharacteristics.to_json();
        j["topologicalMetadata"] = topologicalMetadata.to_json();
        return j;
    }
};

// Alias mapping for ambiguous fields (e.g., "adsr" -> "envelope")
map<string, string> fieldAliases = {
    {"adsr", "envelope"},
    {"osc", "oscillator"},
    {"effects", "fx"}
    // Add more as needed
};

void resolveAliases(json& section, const string& contextName) {
    if (!section.is_object()) return;
    for (const auto& [alias, canonical] : fieldAliases) {
        if (section.contains(alias) && !section.contains(canonical)) {
            section[canonical] = section[alias];
            section.erase(alias);
            cerr << "[Mapping] Renamed alias '" << alias << "' to canonical '" << canonical << "' for context: " << contextName << endl;
        }
    }
    // Recurse for nested objects
    for (auto& [key, val] : section.items()) {
        if (val.is_object()) {
            resolveAliases(val, contextName + "." + key);
        }
    }
}

// Semantic Keyword Database (SKD) as global json
json skd = R"(
{
  "warm": {"category": "timbral", "aliases": ["soft", "mellow"], "score": 0.9},
  "lush": {"category": "timbral", "aliases": ["rich", "full"], "score": 0.85},
  "nostalgic": {"category": "emotional", "aliases": ["sentimental", "bittersweet"], "score": 0.95},
  "calm": {"category": "emotional", "aliases": ["peaceful", "relaxed"], "score": 0.8},
  "bright": {"category": "timbral", "aliases": ["shiny", "clear"], "score": 0.75},
  "aggressive": {"category": "emotional", "aliases": ["intense", "fierce"], "score": 0.9},
  "punchy": {"category": "dynamic", "aliases": ["sharp", "impactful"], "score": 0.85},
  "gritty": {"category": "timbral", "aliases": ["rough", "distorted"], "score": 0.8},
  "percussive": {"category": "dynamic", "aliases": ["strike", "hit"], "score": 0.7},
  "sustained": {"category": "dynamic", "aliases": ["long", "held"], "score": 0.75},
  "driving": {"category": "emotional", "aliases": ["energetic", "motivating"], "score": 0.85},
  "playful": {"category": "emotional", "aliases": ["fun", "lighthearted"], "score": 0.7},
  "reflective": {"category": "emotional", "aliases": ["thoughtful", "introspective"], "score": 0.8},
  "hypnotic": {"category": "emotional", "aliases": ["mesmerizing", "trance-like"], "score": 0.75},
  "chaotic": {"category": "timbral", "aliases": ["disordered", "unpredictable"], "score": 0.85},
  "ethereal": {"category": "timbral", "aliases": ["airy", "heavenly"], "score": 0.8},
  "vintage": {"category": "timbral", "aliases": ["retro", "old-school"], "score": 0.75},
  "organic": {"category": "material", "aliases": ["natural", "acoustic"], "score": 0.8},
  "cybernetic": {"category": "material", "aliases": ["digital", "synthetic"], "score": 0.75},
  "solid": {"category": "material", "aliases": ["dense", "robust"], "score": 0.7},
  "glass": {"category": "material", "aliases": ["fragile", "crystal"], "score": 0.7},
  "plastic": {"category": "material", "aliases": ["synthetic", "cheap"], "score": 0.65},
  "metal": {"category": "material", "aliases": ["hard", "resonant"], "score": 0.75},
  "wood": {"category": "material", "aliases": ["natural", "warm"], "score": 0.8},
  "string": {"category": "material", "aliases": ["vibrating", "plucked"], "score": 0.7},
  "air": {"category": "material", "aliases": ["windy", "breath"], "score": 0.65},
  "acid": {"category": "timbral", "aliases": ["squelchy", "resonant"], "score": 0.8},
  "fat": {"category": "timbral", "aliases": ["thick", "full"], "score": 0.85},
  "jittery": {"category": "timbral", "aliases": ["unstable", "vibrating"], "score": 0.7},
  "bell-like": {"category": "timbral", "aliases": ["ringing", "metallic"], "score": 0.75},
  "gritty": {"category": "timbral", "aliases": ["rough", "distorted"], "score": 0.8},
  "hard": {"category": "timbral", "aliases": ["solid", "dense"], "score": 0.75},
  "thin": {"category": "timbral", "aliases": ["narrow", "light"], "score": 0.7},
  "riveting": {"category": "dynamic", "aliases": ["engaging", "captivating"], "score": 0.8},
  "bouncy": {"category": "dynamic", "aliases": ["springy", "elastic"], "score": 0.75},
  "rhythmic": {"category": "dynamic", "aliases": ["pulsed", "beat-driven"], "score": 0.8},
  "steady": {"category": "dynamic", "aliases": ["consistent", "stable"], "score": 0.75},
  "tribal": {"category": "dynamic", "aliases": ["primitive", "ritual"], "score": 0.7},
  "sour": {"category": "timbral", "aliases": ["acidic", "sharp"], "score": 0.65},
  "unpredictable": {"category": "material", "aliases": ["random", "chaotic"], "score": 0.8},
  "evolving": {"category": "dynamic", "aliases": ["changing", "developing"], "score": 0.85},
  "development": {"category": "emotional", "aliases": ["progressive", "building"], "score": 0.8},
  "surprising": {"category": "emotional", "aliases": ["unexpected", "shocking"], "score": 0.75},
  "unstable": {"category": "emotional", "aliases": ["erratic", "volatile"], "score": 0.7},
  "intense": {"category": "emotional", "aliases": ["strong", "powerful"], "score": 0.9},
  "dreamy": {"category": "emotional", "aliases": ["ethereal", "surreal"], "score": 0.85},
  "clear": {"category": "timbral", "aliases": ["transparent", "crisp"], "score": 0.8},
  "delicate": {"category": "emotional", "aliases": ["fragile", "gentle"], "score": 0.75}
}
)"_json;

void loadSKD(const string& file = "skd.json") {
    ifstream inFile(file);
    json loadedSKD;
    if (inFile) {
        inFile >> loadedSKD;
        if (loadedSKD.is_object()) {
            skd = loadedSKD;
            cerr << "[Info] Loaded SKD from " << file << endl;
        } else {
            cerr << "[TypeError] SKD file not an object—using hardcoded fallback." << endl;
        }
    } else {
        cerr << "[Warn] SKD file not found—using hardcoded fallback." << endl;
    }
    // Validate SKD structure
    for (auto& [key, entry] : skd.items()) {
        if (!entry.is_object() || !entry.contains("category") || !entry["category"].is_string()) {
            cerr << "[TypeError] Invalid SKD entry for '" << key << "': " << entry << endl;
        }
    }
}

// Group configs by category for narrowing (Step 2)
map<string, vector<string>> groupByCategory(const vector<string>& userTags) {
    map<string, vector<string>> groups;
    for (const string& tag : userTags) {
        string ltag = lower(tag);
        bool matched = false;
        for (auto& [skdKey, entry] : skd.items()) {
            string lskd = lower(skdKey);
            if (entry.is_object() && entry.contains("category") && entry["category"].is_string()) {
                string cat = entry["category"].get<string>();
                if (ltag == lskd) {
                    groups[cat].push_back(skdKey);
                    matched = true;
                } else if (entry.contains("aliases") && entry["aliases"].is_array()) {
                    vector<string> aliases = getStringVec(entry["aliases"]);
                    for (const string& alias : aliases) {
                        if (lower(alias) == ltag) {
                            groups[cat].push_back(skdKey);
                            matched = true;
                            break;
                        }
                    }
                }
            }
        }
        if (!matched) {
            // Handle unrecognized (stub for now; expand later)
            groups["unmatched"].push_back(tag);
        }
    }
    return groups;
}
// Simple cosine similarity stub (for future vectorization; use a library like Eigen for real impl)
double cosineSimilarity(const vector<double>& v1, const vector<double>& v2) {
    if (v1.size() != v2.size()) return 0.0;
    double dot = 0.0, mag1 = 0.0, mag2 = 0.0;
    for (size_t i = 0; i < v1.size(); ++i) {
        dot += v1[i] * v2[i];
        mag1 += v1[i] * v1[i];
        mag2 += v2[i] * v2[i];
    }
    return dot / (sqrt(mag1) * sqrt(mag2) + 1e-6);
}

// Precomputed vectors (stub; in real, use word2vec or similar for keywords)
map<string, vector<double>> keywordVectors = {
    {"warm", {0.9, 0.2, 0.1}},
    {"lush", {0.85, 0.3, 0.15}},
    {"nostalgic", {0.95, 0.4, 0.05}},
    // ... add for all SKD keys and aliases
};

// Function to compute semantic score for a config based on user tags
double computeSemanticScore(const SoundConfig& cfg, const vector<string>& userTags, const string& mood = "", const string& synthType = "") {
    double score = 0.0;
    vector<string> configKeywords = cfg.soundCharacteristics.getEmotionalTags();
    configKeywords.push_back(cfg.soundCharacteristics.timbral);
    configKeywords.push_back(cfg.soundCharacteristics.material);
    configKeywords.push_back(cfg.soundCharacteristics.dynamic);
    configKeywords.push_back(cfg.topologicalMetadata.damping);
    configKeywords.push_back(cfg.topologicalMetadata.spectralComplexity);
    configKeywords.push_back(cfg.topologicalMetadata.manifoldPosition);
    configKeywords.push_back(cfg.emotion);
    configKeywords.push_back(cfg.topology);
    configKeywords.push_back(cfg.instrumentType);

    for (const string& tag : userTags) {
        string ltag = lower(tag);
        double maxMatch = 0.0;
        for (const string& ckw : configKeywords) {
            string lckw = lower(ckw);
            if (skd.contains(lckw)) {
                double tagScore = skd[lckw].value("score", 0.0);
                if (ltag == lckw) {
                    maxMatch = max(maxMatch, tagScore);
                } else {
                    vector<string> aliases = getStringVec(skd[lckw].value("aliases", json::array()));
                    for (const string& alias : aliases) {
                        if (lower(alias) == ltag) {
                            maxMatch = max(maxMatch, tagScore * 0.8); // Reduced for alias
                            break;
                        }
                    }
                }
                // Vectorization layer (incremental)
                if (keywordVectors.find(ltag) != keywordVectors.end() && keywordVectors.find(lckw) != keywordVectors.end()) {
    maxMatch *= cosineSimilarity(keywordVectors[ltag], keywordVectors[lckw]);
               }
            }
        }
        score += maxMatch;
    }

    // Mood matching (new)
    if (!mood.empty() && !cfg.emotion.empty()) {
        score += (lower(mood) == lower(cfg.emotion)) ? 1.0 : 0.0; // Exact bonus
    }

    // Synth-profile matching (new)
    if (!synthType.empty() && !cfg.instrumentType.empty()) {
        score += (lower(synthType) == lower(cfg.instrumentType)) ? 1.0 : 0.0; // Exact bonus
    }

    return score / (userTags.size() + 1e-6); // Normalize
}

// SoundEngineeringQueue (complete loaders with fixes)
class SoundEngineeringQueue {
public:
    void loadAndMerge() {
        // Example registrations (C. Dynamic Registration)
        map<string, ParamMeta> reverbSchema;
        reverbSchema["decay"] = {"Decay Time", 0.0f, 10.0f, "s", "Reverb decay time", true, "float"};
        reverbSchema["wet"] = {"Wet Mix", 0.0f, 1.0f, "", "Dry/wet balance", true, "float"};
        reverbSchema["ai_control"] = {"AI Control", 0.0f, 0.0f, "", "Enable AI modulation", false, "bool"};
        BaseParamStruct::registerSchema("reverb", reverbSchema);

        map<string, ParamMeta> distortionSchema;
        distortionSchema["gain"] = {"Gain Level", 0.0f, 1.0f, "", "Distortion gain", true, "float"};
        distortionSchema["wet"] = {"Wet Mix", 0.0f, 1.0f, "", "Dry/wet balance", true, "float"};
        distortionSchema["ai_control"] = {"AI Control", 0.0f, 0.0f, "", "Enable AI modulation", false, "bool"};
        BaseParamStruct::registerSchema("distortion", distortionSchema);

        // Load ONLY real, renderable instruments
        load_guitar("guitar.json");
        load_group("group.json");
        
        // Load scoring/filtering data (NOT as instruments)
        load_moods_for_scoring("moods.json");
        load_synth_for_scoring("Synthesizer.json");
        load_structure("structure.json");
        
        cout << "\n=== Loaded Real Instruments ===\n";
        cout << "Guitar configs: " << countByType("guitar") << "\n";
        cout << "Synthesizer configs: " << countByType("synth") << "\n";
        cout << "Total renderable configs: " << configs.size() << "\n";
        
        for (const auto& [key, cfg] : configs) {
            reportLoaded(key); // Print loaded/missing report
        }
        saveConfig("config.json");
    }
    

private:
    map<string, SoundConfig> configs;
    map<string, GroupConfig> groupConfigs;

    void load_guitar(const string& file) {
        ifstream inFile(file);
        if (!inFile) { cerr << "[Warn] Couldn't open " << file << endl; return; }
        json j; inFile >> j;
        if (!j.is_object()) {
            cerr << "[TypeError] Expected object for guitar.json root, got " << j.type_name() << endl;
            return;
        }
        if (j.contains("guitar_types") && j["guitar_types"].is_object()) {
            for (auto& [gtype, gval] : j["guitar_types"].items()) {
                if (gval.is_object() && gval.contains("groups") && gval["groups"].is_object()) {
                    for (auto& [gname, params] : gval["groups"].items()) {
                        string configKey = lower(gname);
                        resolveAliases(params, configKey);
                        SoundConfig cfg;
                        cfg.instrumentType = gtype;

                        // Oscillator types
                        if (params.contains("oscillator") && params["oscillator"].is_object() && params["oscillator"].contains("types") && params["oscillator"]["types"].is_array()) {
                            cfg.oscTypes["osc1"] = getStringVec(params["oscillator"]["types"], configKey + ".oscillator.types");
                        }

                        // Envelope
                        if (params.contains("envelope") && params["envelope"].is_object()) {
                            auto& e = params["envelope"];
                            if (e.contains("type") && e["type"].is_string()) {
                                cfg.guitarParams.stringParams["type"] = e["type"].get<string>();
                            }
                            if (e.contains("curve") && e["curve"].is_string()) {
                                cfg.guitarParams.stringParams["curve"] = e["curve"].get<string>();
                            }
                            for (string param : {"attack", "decay", "sustain", "release", "delay", "hold"}) {
                                if (e.contains(param)) {
                                    cfg.adsr["osc"][param].from_json(e[param]);
                                }
                            }
                        }

                        // Filter
                        if (params.contains("filter") && params["filter"].is_object()) {
                            auto& f = params["filter"];
                            if (f.contains("cutoff")) {
                                cfg.guitarParams.storeParam("cutoff", f["cutoff"], configKey + ".filter.cutoff");
                            }
                            if (f.contains("resonance")) {
                                cfg.guitarParams.storeParam("resonance", f["resonance"], configKey + ".filter.resonance");
                            }
                            if (f.contains("envelope_amount")) {
                                cfg.guitarParams.storeParam("envelope_amount", f["envelope_amount"], configKey + ".filter.envelope_amount");
                            }
                            if (f.contains("slope") && f["slope"].is_string()) {
                                cfg.guitarParams.stringParams["slope"] = f["slope"].get<string>();
                            }
                            if (f.contains("type") && f["type"].is_string()) {
                                cfg.guitarParams.stringParams["filter_type"] = f["type"].get<string>();
                            }
                        }

                        // Strings
                        if (params.contains("strings") && params["strings"].is_object()) {
                            auto& s = params["strings"];
                            if (s.contains("material") && s["material"].is_string()) {
                                cfg.guitarParams.stringParams["material"] = s["material"].get<string>();
                            }
                            if (s.contains("gauge") && s["gauge"].is_string()) {
                                cfg.guitarParams.stringParams["gauge"] = s["gauge"].get<string>();
                            }
                            if (s.contains("tension") && s["tension"].is_string()) {
                                cfg.guitarParams.stringParams["tension"] = s["tension"].get<string>();
                            }
                            if (s.contains("num_strings") && s["num_strings"].is_number()) {
                                cfg.guitarParams.stringParams["num_strings"] = to_string(s["num_strings"].get<int>());
                            }
                            if (s.contains("ai_control") && s["ai_control"].is_boolean()) {
                                cfg.guitarParams.boolParams["ai_control"] = s["ai_control"].get<bool>();
                            }
                            if (s.contains("tuning") && s["tuning"].is_array()) {
                                cfg.guitarParams.stringParams["tuning"] = join(getStringVec(s["tuning"], configKey + ".strings.tuning"), ",");
                            }
                            if (s.contains("detune_range")) {
                                vector<float> detune = getFloatVec(s["detune_range"], configKey + ".strings.detune_range");
                                if (!detune.empty()) {
                                    cfg.guitarParams.vectorParams["detune_range"] = detune;
                                }
                            }
                        }

                        // Harmonics
                        if (params.contains("harmonics") && params["harmonics"].is_object()) {
                            auto& h = params["harmonics"];
                            if (h.contains("vibe_set") && h["vibe_set"].is_array()) {
                                cfg.guitarParams.vectorParams["vibe_set"] = getFloatVec(h["vibe_set"], configKey + ".harmonics.vibe_set");
                            }
                            if (h.contains("decay_rate") && h["decay_rate"].is_array()) {
                                cfg.guitarParams.vectorParams["decay_rate"] = getFloatVec(h["decay_rate"], configKey + ".harmonics.decay_rate");
                            }
                            if (h.contains("sympathetic_resonance") && h["sympathetic_resonance"].is_object()) {
                                auto& sr = h["sympathetic_resonance"];
                                if (sr.contains("harmonics") && sr["harmonics"].is_array()) {
                                    cfg.guitarParams.vectorParams["sympathetic_harmonics"] = getFloatVec(sr["harmonics"], configKey + ".sympathetic_resonance.harmonics");
                                }
                                if (sr.contains("volume") && sr["volume"].is_array()) {
                                    cfg.guitarParams.vectorParams["sympathetic_volume"] = getFloatVec(sr["volume"], configKey + ".sympathetic_resonance.volume");
                                }
                                if (sr.contains("num_layers") && sr["num_layers"].is_number()) {
                                    cfg.guitarParams.vectorParams["sympathetic_num_layers"] = {static_cast<float>(sr["num_layers"].get<int>())};
                                }
                                if (sr.contains("randomize_range") && sr["randomize_range"].is_array()) {
                                    cfg.guitarParams.vectorParams["sympathetic_randomize_range"] = getFloatVec(sr["randomize_range"], configKey + ".sympathetic_resonance.randomize_range");
                                }
                            }
                        }

                        // Body resonance
                        if (params.contains("body_resonance") && params["body_resonance"].is_object()) {
                            auto& br = params["body_resonance"];
                            if (br.contains("mix")) {
                                cfg.guitarParams.storeParam("mix", br["mix"], configKey + ".body_resonance.mix");
                            }
                            if (br.contains("ir_file") && br["ir_file"].is_string()) {
                                cfg.guitarParams.stringParams["ir_file"] = br["ir_file"].get<string>();
                            }
                        }

                        // Attack noise
                        if (params.contains("attack_noise") && params["attack_noise"].is_object()) {
                            auto& a = params["attack_noise"];
                            if (a.contains("intensity")) {
                                cfg.guitarParams.storeParam("intensity", a["intensity"], configKey + ".attack_noise.intensity");
                            }
                            if (a.contains("probability")) {
                                cfg.guitarParams.storeParam("probability", a["probability"], configKey + ".attack_noise.probability");
                            }
                            if (a.contains("burst_length")) {
                                cfg.guitarParams.storeParam("burst_length", a["burst_length"], configKey + ".attack_noise.burst_length");
                            }
                            if (a.contains("noise_type") && a["noise_type"].is_string()) {
                                cfg.guitarParams.stringParams["noise_type"] = a["noise_type"].get<string>();
                            }
                        }

                        // Pick
                        if (params.contains("pick") && params["pick"].is_object()) {
                            auto& p = params["pick"];
                            if (p.contains("position")) {
                                cfg.guitarParams.storeParam("position", p["position"], configKey + ".pick.position");
                            }
                            if (p.contains("noiseProbability")) {
                                cfg.guitarParams.storeParam("noiseProbability", p["noiseProbability"], configKey + ".pick.noiseProbability");
                            }
                            if (p.contains("noiseIntensity")) {
                                cfg.guitarParams.storeParam("noiseIntensity", p["noiseIntensity"], configKey + ".pick.noiseIntensity");
                            }
                            if (p.contains("stiffness") && p["stiffness"].is_string()) {
                                cfg.guitarParams.stringParams["stiffness"] = p["stiffness"].get<string>();
                            }
                        }

                        // Vibrato
                        if (params.contains("vibrato") && params["vibrato"].is_object()) {
                            auto& v = params["vibrato"];
                            if (v.contains("vibrato_hz")) {
                                cfg.guitarParams.storeParam("vibratoHz", v["vibrato_hz"], configKey + ".vibrato.vibrato_hz");
                            }
                            if (v.contains("depth_cents")) {
                                cfg.guitarParams.storeParam("depth", v["depth_cents"], configKey + ".vibrato.depth_cents");
                            }
                            if (v.contains("freq_range")) {
                                cfg.guitarParams.storeParam("freq_range", v["freq_range"], configKey + ".vibrato.freq_range");
                            }
                        }

                        // Effects
                        if (params.contains("fx") && params["fx"].is_array()) {
                            for (const auto& fx : params["fx"]) {
                                if (fx.is_object()) {
                                    Fx fxStruct;
                                    fxStruct.from_json(fx);
                                    cfg.effects.push_back(fxStruct);
                                } else {
                                    cerr << "[TypeError] Fx item is not an object in " << configKey << ": " << fx << endl;
                                }
                            }
                        }

                        // Metadata
                        if (params.contains("sound_characteristics") && params["sound_characteristics"].is_object()) {
                            cfg.guitarParams.soundCharacteristics.from_json(params["sound_characteristics"]);
                            cfg.soundCharacteristics = cfg.guitarParams.soundCharacteristics;
                        }
                        if (params.contains("topological_metadata") && params["topological_metadata"].is_object()) {
                            cfg.guitarParams.topologicalMetadata.from_json(params["topological_metadata"]);
                            cfg.topologicalMetadata = cfg.guitarParams.topologicalMetadata;
                        }

                        configs[configKey] = cfg;
                    }
                } else {
                    cerr << "[TypeError] 'groups' not an object in guitar_types." << gtype << endl;
                }
            }
        } else {
            cerr << "[TypeError] 'guitar_types' not found or not an object in guitar.json" << endl;
        }
    }

    string getUserInput(const string& prompt) {
    cout << prompt;
    string input;
    getline(cin, input);
    return lower(input);
}
    
    public:
    // Menu queuing order: Sequential, with back option
void interactiveMenu() {
    cout << "\n=== AI-Driven Audio Configuration Platform ===\n";
    cout << "Progressive Narrowing Workflow\n\n";
    
    vector<string> selectedTags;
    map<string, string> userChoices;
    map<string, vector<string>> availableChoices;
    
    // Stage 1: Section Selection with AI suggestions
    cout << "1. Musical Section Selection\n";
    availableChoices["sections"] = {"intro", "verse", "pre-chorus", "chorus", "drop", "bridge", "outro", "hook"};
    displayChoicesWithAI("section", availableChoices["sections"], selectedTags);
    string section = getUserInputWithValidation("Select section: ", availableChoices["sections"]);
    selectedTags.push_back(section);
    userChoices["section"] = section;
    
    // Stage 2: Mood Selection with contextual filtering
    cout << "\n2. Mood Selection (contextual to " << section << ")\n";
    availableChoices["moods"] = filterMoodsForSection(section);
    displayChoicesWithAI("mood", availableChoices["moods"], selectedTags);
    string mood = getUserInputWithValidation("Select mood: ", availableChoices["moods"]);
    selectedTags.push_back(mood);
    userChoices["mood"] = mood;
    
    // Stage 3: Timbre Selection with AI narrowing
    cout << "\n3. Timbre Selection (filtered for " << section << " + " << mood << ")\n";
    availableChoices["timbres"] = filterTimbresForContext(selectedTags);
    displayChoicesWithAI("timbre", availableChoices["timbres"], selectedTags);
    string timbre = getUserInputWithValidation("Select timbre: ", availableChoices["timbres"]);
    selectedTags.push_back(timbre);
    userChoices["timbre"] = timbre;
    
    // Stage 4: Instrument Category Selection
    cout << "\n4. Instrument Category Selection\n";
    availableChoices["instruments"] = {"guitar", "synth", "hybrid", "ensemble"};
    displayChoicesWithAI("instrument", availableChoices["instruments"], selectedTags);
    string instrument = getUserInputWithValidation("Select instrument category: ", availableChoices["instruments"]);
    selectedTags.push_back(instrument);
    userChoices["instrument"] = instrument;
    
    // Stage 5: Effect Group Selection
    cout << "\n5. Effect Processing Selection\n";
    availableChoices["effects"] = filterEffectsForContext(selectedTags);
    displayChoicesWithAI("effects", availableChoices["effects"], selectedTags);
    string effectGroup = getUserInputWithValidation("Select effect group: ", availableChoices["effects"]);
    selectedTags.push_back(effectGroup);
    userChoices["effects"] = effectGroup;
    
    // Stage 6: Synthesis Type (if applicable)
    string synthType = "";
    if (instrument == "synth" || instrument == "hybrid") {
        cout << "\n6. Synthesis Type Selection\n";
        availableChoices["synthesis"] = {"subtractive", "fm", "additive", "wavetable", "granular", "physical_modeling"};
        displayChoicesWithAI("synthesis", availableChoices["synthesis"], selectedTags);
        synthType = getUserInputWithValidation("Select synthesis type: ", availableChoices["synthesis"]);
        selectedTags.push_back(synthType);
        userChoices["synthesis"] = synthType;
    }
    
    // AI Analysis and Recommendations
    cout << "\n=== AI Analysis Results ===\n";
    auto groupedTags = groupByCategory(selectedTags);
    cout << "Categorized Selections:\n" << json(groupedTags).dump(2) << "\n\n";
    
    // Generate AI-scored recommendations
    cout << "🎵 AI Recommendations (ranked by compatibility):\n";
    multimap<double, string, greater<double>> scoredConfigs;
    for (const auto& [key, cfg] : configs) {
        double score = computeSemanticScore(cfg, selectedTags, mood, synthType);
        if (score > 0.1) { // Filter out very low scores
            scoredConfigs.insert({score, key});
        }
    }
    
    // Display top recommendations with details
    int rank = 1;
    vector<string> topRecommendations;
    for (const auto& [score, key] : scoredConfigs) {
        if (rank > 8) break; // Top 8 recommendations
        
        const auto& cfg = configs.at(key);
        cout << rank << ". " << key << " (score: " << fixed << setprecision(2) << score << ")\n";
        cout << "   Type: " << cfg.instrumentType;
        cout << " | Timbre: " << cfg.soundCharacteristics.timbral;
        cout << " | Material: " << cfg.soundCharacteristics.material;
        cout << " | Dynamic: " << cfg.soundCharacteristics.dynamic << "\n";
        
        if (!cfg.soundCharacteristics.emotional.empty()) {
            cout << "   Emotions: ";
            for (const auto& [tag, weight] : cfg.soundCharacteristics.emotional) {
                cout << tag << "(" << weight << ") ";
            }
            cout << "\n";
        }
        cout << "\n";
        
        topRecommendations.push_back(key);
        rank++;
    }
    
    // Advanced Configuration Menu
    cout << "\n=== Configuration Options ===\n";
    cout << "Choose your next action:\n";
    cout << "1. Select a specific recommendation\n";
    cout << "2. Generate layered composition\n";
    cout << "3. Advanced tuning/harmonics configuration\n";
    cout << "4. Back to modify selections\n";
    cout << "5. Save current configuration\n";
    cout << "6. Exit\n";
    
    int choice = getUserInputInt("Your choice (1-6): ");
    
    switch (choice) {
        case 1: {
            handleSpecificSelection(topRecommendations, userChoices);
            break;
        }
        case 2: {
            generateEnhancedSectionPatch(selectedTags, userChoices, topRecommendations);
            break;
        }
        case 3: {
            handleAdvancedConfiguration(topRecommendations, userChoices);
            break;
        }
        case 4: {
            // Recursive call to restart with option to modify
            cout << "\nRestarting selection process...\n";
            interactiveMenu();
            return;
        }
        case 5: {
            saveUserConfiguration(userChoices, topRecommendations);
            break;
        }
        case 6:
        default: {
            cout << "Exiting...\n";
            return;
        }
    }
    
    // Ask if user wants to continue iterating
    cout << "\nWould you like to:\n";
    cout << "1. Make another configuration\n";
    cout << "2. Refine current configuration\n";
    cout << "3. Exit\n";
    
    int continueChoice = getUserInputInt("Your choice (1-3): ");
    if (continueChoice == 1) {
        interactiveMenu();
    } else if (continueChoice == 2) {
        // TODO: Implement refinement workflow
        cout << "Refinement workflow not yet implemented.\n";
    }
}

json generateLayeredOutput(const vector<string>& userChoices, const string& mood, const string& synthType) {
    json layered = json::object();
    vector<string> layers = {"background_texture", "ambient_pad", "supportive_harmony", "rhythmic_motion", "main_melodic", "lead_foreground"};
    for (const string& layer : layers) {
        layered["layers"][layer] = json::object(); // Object for module params
    }

    // Optional base (check map or menu flag)
    bool useBase = getUserInput("Use base instrument? [y/n]: ") == "y"; // Or from JSON via getBool("use_base", true)
    if (useBase) {
        string baseKey;
        double maxBaseScore = 0.0;
        for (const auto& [key, cfg] : configs) {
            if (cfg.instrumentType.find("guitar") != string::npos || cfg.instrumentType == "synth") { // Filter to instruments
                double score = computeSemanticScore(cfg, userChoices, mood, synthType);
                if (score > maxBaseScore) {
                    maxBaseScore = score;
                    baseKey = key;
                }
            }
        }
        if (!baseKey.empty()) {
            layered["base_instrument"] = configs[baseKey].to_json(); // Base always optional but included if on
        } else {
            cerr << "[Warning] No suitable base instrument found—proceeding without." << endl;
        }
    }

    // Layer suggestions as additive modules (no delete; merge if override flagged)
    multimap<double, string, greater<double>> scoredConfigs;
    for (const auto& [key, cfg] : configs) {
        double score = computeSemanticScore(cfg, userChoices, mood, synthType);
        scoredConfigs.insert({score, key});
    }
    double threshold = 0.49;
    if (scoredConfigs.size() < 3) {
        threshold = 0.5; // Dynamic adjustment
        cerr << "[Info] Few matches—lowered threshold to 0.5 for broader suggestions." << endl;
    }
    for (const auto& [score, key] : scoredConfigs) {
        if (score >= threshold) {
            // Assign to layer based on traits (stub; real: if slow attack -> "ambient_pad")
            string assignedLayer = "main_melodic"; // Example; expand with envelope/timbre checks
            layered["layers"][assignedLayer] = configs[key].to_json(); // Additive
        }
    }

    // Apply Context-Aware Gain Balancing
    balanceLayerGains(layered, mood, userChoices[0]); // userChoices[0] = section for context

    return layered;
}

void balanceLayerGains(json& layered, const string& mood, const string& section) {
    map<string, float> baseGains = {
        {"background_texture", 0.2f},
        {"ambient_pad", 0.4f},
        {"supportive_harmony", 0.5f},
        {"rhythmic_motion", 0.6f},
        {"main_melodic", 0.7f},
        {"lead_foreground", 0.9f}
    };
    map<string, float> moodOffsets = {
        {"calm", 0.8f}, // Softer overall
        {"energetic", 1.2f} // Boost
        // Add more from moods.json
    };
    float moodOffset = moodOffsets.count(lower(mood)) ? moodOffsets[lower(mood)] : 1.0f;

    map<string, float> sectionFactors = {
        {"intro", 0.9f}, // Softer
        {"chorus", 1.1f} // Louder
        // Add from Synthesizer.json
    };
    float sectionFactor = sectionFactors.count(lower(section)) ? sectionFactors[lower(section)] : 1.0f;

    for (auto& [layer, module] : layered["layers"].items()) {
        if (module.is_object()) {
            float gain = baseGains[layer] * moodOffset * sectionFactor;
            // Transient/envelope sensitivity (stub: if fast attack, boost transient)
            if (module.contains("attack") && module["attack"] < 0.05) gain *= 1.1f; // Percussive boost
            module["layer_gain"] = gain; // Stored in map (extensible)
            // Cross-layer (stub: duck ambient if lead present)
            if (layer == "ambient_pad" && layered.contains("lead_foreground")) gain *= 0.9f;
        }
    }
}

json generateGroupedOutput(const vector<string>& userChoices) {
    json groupedOutput = json::object();
    for (const auto& [key, cfg] : configs) {
        double score = computeSemanticScore(cfg, userChoices);
        if (score >= 0.12) { // Threshold for relevance
            groupedOutput["suggested_configs"][key] = cfg.to_json();
        }
    }
    return groupedOutput;
    }

    // Helper functions for enhanced interactive menu
    string getUserInputWithValidation(const string& prompt, const vector<string>& validOptions) {
        while (true) {
            string input = getUserInput(prompt);
            input = lower(input);
            
            // Check if input is valid
            for (const string& option : validOptions) {
                if (lower(option) == input) {
                    return option; // Return original case
                }
            }
            
            cout << "Invalid choice. Please select from: ";
            for (size_t i = 0; i < validOptions.size(); ++i) {
                cout << validOptions[i];
                if (i < validOptions.size() - 1) cout << ", ";
            }
            cout << "\n";
        }
    }

    int getUserInputInt(const string& prompt) {
        while (true) {
            string input = getUserInput(prompt);
            try {
                return stoi(input);
            } catch (...) {
                cout << "Please enter a valid number.\n";
            }
        }
    }

    void displayChoicesWithAI(const string& category, const vector<string>& choices, const vector<string>& currentTags) {
        cout << "Available " << category << " options:\n";
        
        // Score each choice based on current context
        multimap<double, string, greater<double>> scoredChoices;
        
        for (const string& choice : choices) {
            double score = 0.0;
            vector<string> testTags = currentTags;
            testTags.push_back(choice);
            
            // Calculate contextual score based on existing configs
            for (const auto& [key, cfg] : configs) {
                score += computeSemanticScore(cfg, testTags, "", "") * 0.1;
            }
            
            scoredChoices.insert({score, choice});
        }
        
        // Display choices with AI recommendations
        int rank = 1;
        for (const auto& [score, choice] : scoredChoices) {
            cout << rank << ". " << choice;
            if (rank <= 3) cout << " ⭐"; // Mark top 3 as AI recommended
            cout << "\n";
            rank++;
        }
        cout << "\n";
    }

    vector<string> filterMoodsForSection(const string& section) {
        vector<string> baseMoods = {"calm", "energetic", "nostalgic", "bright", "warm", "aggressive", "dreamy", "tense", "playful", "reflective"};
        vector<string> filtered;
        
        // Context-specific mood filtering
        if (section == "intro" || section == "outro") {
            for (const string& mood : baseMoods) {
                if (mood == "calm" || mood == "reflective" || mood == "dreamy" || mood == "warm") {
                    filtered.push_back(mood);
                }
            }
        } else if (section == "chorus" || section == "drop") {
            for (const string& mood : baseMoods) {
                if (mood == "energetic" || mood == "bright" || mood == "aggressive" || mood == "playful") {
                    filtered.push_back(mood);
                }
            }
        } else {
            filtered = baseMoods; // All moods available for other sections
        }
        
        return filtered;
    }

    vector<string> filterTimbresForContext(const vector<string>& tags) {
        vector<string> allTimbres = {"warm", "bright", "gritty", "smooth", "harsh", "mellow", "crisp", "fat", "thin", "lush", "ethereal", "crystalline"};
        vector<string> filtered;
        
        // Filter based on mood and section context
        bool hasEnergetic = find(tags.begin(), tags.end(), "energetic") != tags.end();
        bool hasCalm = find(tags.begin(), tags.end(), "calm") != tags.end();
        bool hasIntro = find(tags.begin(), tags.end(), "intro") != tags.end();
        bool hasChorus = find(tags.begin(), tags.end(), "chorus") != tags.end();
        
        for (const string& timbre : allTimbres) {
            bool include = true;
            
            // Context-based filtering logic
            if (hasCalm && (timbre == "harsh" || timbre == "aggressive")) include = false;
            if (hasEnergetic && (timbre == "mellow" || timbre == "ethereal")) include = false;
            if (hasIntro && (timbre == "gritty" || timbre == "harsh")) include = false;
            if (hasChorus && (timbre == "thin" || timbre == "mellow")) include = false;
            
            if (include) filtered.push_back(timbre);
        }
        
        return filtered.empty() ? allTimbres : filtered;
    }

    vector<string> filterEffectsForContext(const vector<string>& tags) {
        vector<string> allEffects = {"reverb", "delay", "distortion", "chorus", "flanger", "phaser", "compression", "eq", "filter", "modulation"};
        vector<string> filtered;
        
        // Filter effects based on context
        bool hasGuitar = find(tags.begin(), tags.end(), "guitar") != tags.end();
        bool hasSynth = find(tags.begin(), tags.end(), "synth") != tags.end();
        bool hasCalm = find(tags.begin(), tags.end(), "calm") != tags.end();
        bool hasAggressive = find(tags.begin(), tags.end(), "aggressive") != tags.end();
        
        for (const string& effect : allEffects) {
            bool include = true;
            
            // Context-specific effect filtering
            if (hasCalm && effect == "distortion") include = false;
            if (hasGuitar && (effect == "flanger" || effect == "phaser")) include = true; // Guitar works well with modulation
            if (hasSynth && (effect == "filter" || effect == "modulation")) include = true; // Synth benefits from filtering
            if (hasAggressive && effect == "reverb") include = false; // Less reverb for aggressive sounds
            
            if (include) filtered.push_back(effect);
        }
        
        return filtered.empty() ? allEffects : filtered;
    }

    void handleSpecificSelection(const vector<string>& recommendations, const map<string, string>& userChoices) {
        cout << "\nSelect a specific configuration:\n";
        for (size_t i = 0; i < recommendations.size(); ++i) {
            cout << (i+1) << ". " << recommendations[i] << "\n";
        }
        
        int choice = getUserInputInt("Enter choice (1-" + to_string(recommendations.size()) + "): ");
        if (choice > 0 && choice <= recommendations.size()) {
            string selectedConfig = recommendations[choice-1];
            cout << "\nSelected: " << selectedConfig << "\n";
            
            // Display detailed configuration
            if (configs.count(selectedConfig)) {
                const auto& cfg = configs.at(selectedConfig);
                cout << "\nDetailed Configuration:\n";
                cout << cfg.to_json().dump(2) << "\n";
                
                // Ask if user wants to save this configuration
                string save = getUserInput("Save this configuration? (y/n): ");
                if (lower(save) == "y" || lower(save) == "yes") {
                    json userConfig = {
                        {"selected_config", selectedConfig},
                        {"user_choices", userChoices},
                        {"timestamp", time(nullptr)},
                        {"configuration", cfg.to_json()}
                    };
                    
                    ofstream file("user_selection.json");
                    if (file) {
                        file << userConfig.dump(4);
                        file.close();
                        cout << "Configuration saved to user_selection.json\n";
                    }
                }
            }
        }
    }

    void generateEnhancedSectionPatch(const vector<string>& selectedTags, const map<string, string>& userChoices, const vector<string>& topRecommendations) {
        cout << "\n=== Enhanced Patcher System ===\n";
        string sectionName = userChoices.count("section") ? userChoices.at("section") : "intro";
        
        // Create section patch using the patcher system
        SectionPatch patch = createSectionPatch(sectionName, selectedTags, userChoices, 6);
        
        // Display patch information
        cout << "Generated patch for section: " << patch.sectionName << "\n";
        cout << "Total layers: " << patch.layers.size() << "\n\n";
        
        json patchOutput = json::object();
        patchOutput["patch_metadata"] = patch.patchMetadata;
        patchOutput["section_name"] = patch.sectionName;
        patchOutput["renderable_layers"] = json::array();
        
        for (const auto& layer : patch.layers) {
            cout << "Layer: " << layer.layerRole << " (" << layer.configKey << ")\n";
            cout << "  Base Gain: " << layer.baseGain << " → Final Gain: " << layer.finalGain << "\n";
            cout << "  Ready for Synthesis: " << (layer.renderableConfig["ready_for_synthesis"].get<bool>() ? "YES" : "NO") << "\n";
            cout << "  Tunable Properties: " << layer.tunableProperties.size() << " parameters\n\n";
            
            patchOutput["renderable_layers"].push_back(layer.renderableConfig);
        }
        
        // Save the renderable patch
        ofstream file("section_patch_" + sectionName + ".json");
        if (file) {
            file << patchOutput.dump(4);
            file.close();
            cout << "Renderable section patch saved to section_patch_" << sectionName << ".json\n";
        }
        
        cout << "\n✅ All " << patch.layers.size() << " layers are ready for direct synthesis!\n";
    }
    
    void generateLayeredComposition(const vector<string>& selectedTags, const map<string, string>& userChoices, const vector<string>& topRecommendations) {
        cout << "\n=== Generating Layered Composition ===\n";
        
        json layeredComposition = json::object();
        vector<string> layers = {"background_texture", "ambient_pad", "supportive_harmony", "rhythmic_motion", "main_melodic", "lead_foreground"};
        
        // Assign top recommendations to layers based on their characteristics
        for (size_t i = 0; i < min(layers.size(), topRecommendations.size()); ++i) {
            string configKey = topRecommendations[i];
            if (configs.count(configKey)) {
                const auto& cfg = configs.at(configKey);
                
                json layerConfig = cfg.to_json();
                layerConfig["layer_role"] = layers[i];
                layerConfig["config_key"] = configKey;
                
                // Calculate layer-specific gain
                float baseGain = 1.0f - (i * 0.15f); // Decreasing gain for background layers
                layerConfig["layer_gain"] = baseGain;
                
                layeredComposition["layers"][layers[i]] = layerConfig;
            }
        }
        
        // Add composition metadata
        layeredComposition["metadata"] = {
            {"composition_type", "layered"},
            {"user_choices", userChoices},
            {"selected_tags", selectedTags},
            {"total_layers", layeredComposition["layers"].size()},
            {"timestamp", time(nullptr)}
        };
        
        // Apply context-aware balancing
        balanceLayerGains(layeredComposition, userChoices.count("mood") ? userChoices.at("mood") : "", 
                         userChoices.count("section") ? userChoices.at("section") : "");
        
        // Save layered composition
        ofstream file("layered_composition.json");
        if (file) {
            file << layeredComposition.dump(4);
            file.close();
            cout << "Layered composition saved to layered_composition.json\n";
            cout << "Layers created: " << layeredComposition["layers"].size() << "\n";
        }
        
        cout << "\nLayered Composition Structure:\n";
        cout << layeredComposition.dump(2) << "\n";
    }

    void handleAdvancedConfiguration(const vector<string>& recommendations, const map<string, string>& userChoices) {
        cout << "\n=== Advanced Configuration ===\n";
        cout << "Advanced features:\n";
        cout << "1. Tuning adjustments\n";
        cout << "2. Sympathetic harmonics\n";
        cout << "3. ADSR envelope fine-tuning\n";
        cout << "4. Effect parameter adjustment\n";
        cout << "5. Back to main menu\n";
        
        int choice = getUserInputInt("Select advanced feature (1-5): ");
        
        switch (choice) {
            case 1:
                cout << "Tuning adjustment feature coming soon...\n";
                break;
            case 2:
                cout << "Sympathetic harmonics configuration coming soon...\n";
                break;
            case 3:
                cout << "ADSR envelope fine-tuning coming soon...\n";
                break;
            case 4:
                cout << "Effect parameter adjustment coming soon...\n";
                break;
            default:
                return;
        }
    }

    void saveUserConfiguration(const map<string, string>& userChoices, const vector<string>& recommendations) {
        json userSession = {
            {"session_type", "user_configuration"},
            {"user_choices", userChoices},
            {"ai_recommendations", recommendations},
            {"timestamp", time(nullptr)},
            {"version", "2.0"}
        };
        
        ofstream file("user_session.json");
        if (file) {
            file << userSession.dump(4);
            file.close();
            cout << "User session saved to user_session.json\n";
        }
    }
    
    // Enhanced Patcher System - Core Implementation
    struct LayerAssignment {
        string configKey;
        string layerRole;
        float baseGain;
        float finalGain;
        json renderableConfig;
        map<string, float> tunableProperties;
    };
    
    struct SectionPatch {
        string sectionName;
        vector<LayerAssignment> layers;
        map<string, float> globalProperties;
        json patchMetadata;
    };
    
    // Layer roles with their characteristics
    map<string, map<string, float>> layerRoleProperties = {
        {"background_texture", {{"baseGain", 0.15}, {"priority", 1}, {"stereoWidth", 0.8}, {"reverb", 0.6}}},
        {"ambient_pad", {{"baseGain", 0.25}, {"priority", 2}, {"stereoWidth", 0.7}, {"reverb", 0.5}}},
        {"supportive_harmony", {{"baseGain", 0.35}, {"priority", 3}, {"stereoWidth", 0.6}, {"reverb", 0.4}}},
        {"rhythmic_motion", {{"baseGain", 0.45}, {"priority", 4}, {"stereoWidth", 0.5}, {"reverb", 0.3}}},
        {"main_melodic", {{"baseGain", 0.65}, {"priority", 5}, {"stereoWidth", 0.4}, {"reverb", 0.2}}},
        {"lead_foreground", {{"baseGain", 0.85}, {"priority", 6}, {"stereoWidth", 0.3}, {"reverb", 0.1}}}
    };
    
    SectionPatch createSectionPatch(const string& sectionName, const vector<string>& userTags, 
                                   const map<string, string>& userChoices, int maxLayers = 6) {
        SectionPatch patch;
        patch.sectionName = sectionName;
        
        // Get scored and ranked configs for this section
        multimap<double, string, greater<double>> scoredConfigs;
        for (const auto& [key, cfg] : configs) {
            double score = computeEnhancedSemanticScore(cfg, userTags, userChoices, sectionName);
            if (score > 0.05) { // Only consider reasonably compatible configs
                scoredConfigs.insert({score, key});
            }
        }
        
        // Assign configs to layers based on their characteristics
        vector<string> layerRoles = {"background_texture", "ambient_pad", "supportive_harmony", 
                                   "rhythmic_motion", "main_melodic", "lead_foreground"};
        
        int layerCount = 0;
        for (const auto& [score, configKey] : scoredConfigs) {
            if (layerCount >= maxLayers) break;
            
            const auto& cfg = configs.at(configKey);
            LayerAssignment assignment;
            assignment.configKey = configKey;
            assignment.layerRole = assignOptimalLayer(cfg, layerRoles[layerCount], sectionName);
            assignment.baseGain = layerRoleProperties[assignment.layerRole]["baseGain"];
            assignment.renderableConfig = createRenderableConfig(cfg, assignment.layerRole, userChoices);
            assignment.tunableProperties = extractTunableProperties(cfg);
            
            patch.layers.push_back(assignment);
            layerCount++;
        }
        
        // Apply context-aware gain balancing
        applyContextualGainBalancing(patch, userChoices);
        
        // Add patch metadata
        patch.patchMetadata = {
            {"section", sectionName},
            {"user_choices", userChoices},
            {"total_layers", patch.layers.size()},
            {"timestamp", time(nullptr)},
            {"patch_version", "2.0"}
        };
        
        return patch;
    }
    
    string assignOptimalLayer(const SoundConfig& cfg, const string& defaultRole, const string& section) {
        // Smart layer assignment based on instrument characteristics
        
        // Check attack time for layer assignment
        float avgAttack = 0.0f;
        int attackCount = 0;
        for (const auto& [context, params] : cfg.adsr) {
            if (params.count("attack")) {
                avgAttack += params.at("attack").min;
                attackCount++;
            }
        }
        if (attackCount > 0) avgAttack /= attackCount;
        
        // Fast attack = rhythmic/foreground, slow attack = pad/background
        if (avgAttack < 50.0f) { // Fast attack (< 50ms)
            if (cfg.instrumentType.find("guitar") != string::npos) return "main_melodic";
            if (cfg.soundCharacteristics.dynamic == "percussive") return "rhythmic_motion";
            return "lead_foreground";
        } else if (avgAttack > 500.0f) { // Slow attack (> 500ms)
            if (cfg.soundCharacteristics.timbral.find("warm") != string::npos) return "ambient_pad";
            return "background_texture";
        }
        
        // Check emotional characteristics
        for (const auto& [tag, weight] : cfg.soundCharacteristics.emotional) {
            if (tag == "lush" || tag == "warm") return "ambient_pad";
            if (tag == "bright" || tag == "energetic") return "lead_foreground";
            if (tag == "rhythmic" || tag == "driving") return "rhythmic_motion";
        }
        
        // Default assignment based on instrument type
        if (cfg.instrumentType.find("guitar") != string::npos) return "main_melodic";
        if (cfg.instrumentType == "subtractive" && cfg.soundCharacteristics.timbral == "warm") return "supportive_harmony";
        
        return defaultRole; // Fallback to provided default
    }
    
    json createRenderableConfig(const SoundConfig& cfg, const string& layerRole, const map<string, string>& userChoices) {
        json renderable = cfg.to_json();
        
        // Add layer-specific properties
        renderable["layer_role"] = layerRole;
        renderable["layer_properties"] = layerRoleProperties[layerRole];
        
        // Expose ALL tunable properties for rendering
        json tunableProps = json::object();
        
        // ADSR parameters (always tunable)
        for (const auto& [context, params] : cfg.adsr) {
            for (const auto& [param, range] : params) {
                tunableProps["adsr"][context][param] = {
                    {"min", range.min}, 
                    {"max", range.max}, 
                    {"current", (range.min + range.max) / 2.0f},
                    {"tunable", true}
                };
            }
        }
        
        // Guitar-specific parameters
        if (!cfg.guitarParams.floatParams.empty()) {
            for (const auto& [param, value] : cfg.guitarParams.floatParams) {
                tunableProps["guitar"][param] = {
                    {"value", value},
                    {"tunable", true},
                    {"type", "float"}
                };
            }
        }
        
        // Effects parameters
        for (const auto& effect : cfg.effects) {
            json effectParams = effect.to_json();
            for (auto& [param, value] : effectParams.items()) {
                if (param != "type") {
                    tunableProps["effects"][effect.type][param] = {
                        {"value", value},
                        {"tunable", true}
                    };
                }
            }
        }
        
        renderable["tunable_properties"] = tunableProps;
        renderable["ready_for_synthesis"] = true;
        
        return renderable;
    }
    
    map<string, float> extractTunableProperties(const SoundConfig& cfg) {
        map<string, float> tunable;
        
        // Extract key tunable parameters for quick access
        for (const auto& [context, params] : cfg.adsr) {
            for (const auto& [param, range] : params) {
                tunable["adsr_" + param] = (range.min + range.max) / 2.0f;
            }
        }
        
        // Guitar parameters
        for (const auto& [param, value] : cfg.guitarParams.floatParams) {
            tunable["guitar_" + param] = value;
        }
        
        return tunable;
    }
    
    void applyContextualGainBalancing(SectionPatch& patch, const map<string, string>& userChoices) {
        string mood = userChoices.count("mood") ? userChoices.at("mood") : "";
        string section = userChoices.count("section") ? userChoices.at("section") : "";
        
        // Mood-based gain adjustments
        float moodMultiplier = 1.0f;
        if (mood == "calm" || mood == "reflective") moodMultiplier = 0.8f;
        else if (mood == "energetic" || mood == "aggressive") moodMultiplier = 1.2f;
        
        // Section-based gain adjustments
        float sectionMultiplier = 1.0f;
        if (section == "intro" || section == "outro") sectionMultiplier = 0.9f;
        else if (section == "chorus" || section == "drop") sectionMultiplier = 1.1f;
        
        // Apply adjustments to each layer
        for (auto& layer : patch.layers) {
            layer.finalGain = layer.baseGain * moodMultiplier * sectionMultiplier;
            
            // Special adjustments based on layer role
            if (layer.layerRole == "background_texture" && mood == "energetic") {
                layer.finalGain *= 0.7f; // Reduce background in energetic sections
            }
            if (layer.layerRole == "lead_foreground" && section == "intro") {
                layer.finalGain *= 0.8f; // Softer lead in intros
            }
            
            // Ensure gains stay within reasonable bounds
            layer.finalGain = max(0.05f, min(1.0f, layer.finalGain));
        }
    }
    
    double computeEnhancedSemanticScore(const SoundConfig& cfg, const vector<string>& userTags, 
                                       const map<string, string>& userChoices, const string& section) {
        double score = computeSemanticScore(cfg, userTags, 
                                          userChoices.count("mood") ? userChoices.at("mood") : "",
                                          userChoices.count("synthesis") ? userChoices.at("synthesis") : "");
        
        // Bonus for section compatibility using scoring data
        if (sectionScoringData.count(section)) {
            const auto& sectionData = sectionScoringData[section];
            
            // Check emotional compatibility
            if (sectionData.contains("emotion")) {
                string sectionEmotion = sectionData["emotion"].get<string>();
                for (const auto& [tag, weight] : cfg.soundCharacteristics.emotional) {
                    if (lower(sectionEmotion).find(lower(tag)) != string::npos) {
                        score += 0.2; // Bonus for emotional match
                    }
                }
            }
            
            // Check topology compatibility
            if (sectionData.contains("topology")) {
                string sectionTopology = sectionData["topology"].get<string>();
                if (lower(sectionTopology).find(lower(cfg.topologicalMetadata.damping)) != string::npos) {
                    score += 0.1; // Bonus for topological match
                }
            }
        }
        
        return score;
    }

    private:

    void load_group(const string& file) {
        ifstream inFile(file);
        if (!inFile) {
            cerr << "[Warn] Couldn't open " << file << endl;
            return;
        }
        json j;
        inFile >> j;
        if (!j.is_object()) {
            cerr << "[TypeError] Expected object for group.json root, got " << j.type_name() << endl;
            return;
        }
        if (j.contains("groups") && j["groups"].is_object()) {
            for (auto& [name, group] : j["groups"].items()) {
                string configKey = lower(name);
                resolveAliases(group, configKey);
                GroupConfig gCfg;
                gCfg.from_json(group);
                groupConfigs[configKey] = gCfg;

                SoundConfig cfg;
                cfg.instrumentType = "synth";
                if (configs.count(configKey)) {
                    cfg = configs[configKey]; // Merge with existing config
                }

                // Synthesis type
                cfg.instrumentType = gCfg.synthesisType;

                // Oscillator
                if (group.contains("oscillator") && group["oscillator"].is_object()) {
                    auto& osc = group["oscillator"];
                    if (osc.contains("types") && osc["types"].is_array()) {
                        cfg.oscTypes["osc1"] = getStringVec(osc["types"], configKey + ".oscillator.types");
                    }
                    if (osc.contains("mix_ratios") && osc["mix_ratios"].is_array()) {
                        cfg.guitarParams.vectorParams["mix_ratios"] = getFloatVec(osc["mix_ratios"], configKey + ".oscillator.mix_ratios");
                    }
                    if (osc.contains("detune")) {
                        cfg.guitarParams.floatParams["detune"] = getFlexibleFloat(osc["detune"], configKey + ".oscillator.detune");
                    }
                    if (osc.contains("morph_rate")) {
                        cfg.guitarParams.stringParams["morph_rate"] = getStringOrFloat(osc["morph_rate"]);
                    }
                    if (osc.contains("table_index")) {
                        cfg.guitarParams.stringParams["table_index"] = getStringOrFloat(osc["table_index"]);
                    }
                    // Store other oscillator params generally
                    gCfg.oscillator.from_json(osc); // To handle any additional like harmonics, modulation_index
                    // Transfer to cfg.guitarParams maps
                    for (const auto& [k, v] : gCfg.oscillator.floatParams) cfg.guitarParams.floatParams[k] = v;
                    for (const auto& [k, v] : gCfg.oscillator.boolParams) cfg.guitarParams.boolParams[k] = v;
                    for (const auto& [k, v] : gCfg.oscillator.stringParams) cfg.guitarParams.stringParams[k] = v;
                    for (const auto& [k, v] : gCfg.oscillator.vectorParams) cfg.guitarParams.vectorParams[k] = v;
                    for (const auto& [k, v] : gCfg.oscillator.stringVectorParams) cfg.guitarParams.stringVectorParams[k] = v;
                }

                // Envelope
                if (group.contains("envelope") && group["envelope"].is_object()) {
                    auto& e = group["envelope"];
                    if (e.contains("type") && e["type"].is_string()) {
                        cfg.guitarParams.stringParams["type"] = e["type"].get<string>();
                    }
                    if (e.contains("curve") && e["curve"].is_string()) {
                        cfg.guitarParams.stringParams["curve"] = e["curve"].get<string>();
                    }
                    for (string param : {"attack", "decay", "sustain", "release", "delay", "hold"}) {
                        if (e.contains(param)) {
                            cfg.adsr["group"][param].from_json(e[param]);
                        }
                    }
                    // Transfer to cfg.guitarParams maps
                    gCfg.envelope.from_json(e);
                    for (const auto& [k, v] : gCfg.envelope.floatParams) cfg.guitarParams.floatParams[k] = v;
                    for (const auto& [k, v] : gCfg.envelope.boolParams) cfg.guitarParams.boolParams[k] = v;
                    for (const auto& [k, v] : gCfg.envelope.stringParams) cfg.guitarParams.stringParams[k] = v;
                    for (const auto& [k, v] : gCfg.envelope.vectorParams) cfg.guitarParams.vectorParams[k] = v;
                    for (const auto& [k, v] : gCfg.envelope.stringVectorParams) cfg.guitarParams.stringVectorParams[k] = v;
                }

                // Filter
                if (group.contains("filter") && group["filter"].is_object()) {
                    gCfg.filter.from_json(group["filter"]);
                    cfg.guitarParams.floatParams["cutoff"] = gCfg.filter.getFloat("cutoff");
                    cfg.guitarParams.floatParams["resonance"] = gCfg.filter.getFloat("resonance");
                    cfg.guitarParams.floatParams["envelope_amount"] = gCfg.filter.getFloat("envelope_amount");
                    cfg.guitarParams.stringParams["slope"] = gCfg.filter.getString("slope");
                    cfg.guitarParams.stringParams["filter_type"] = gCfg.filter.getString("type");
                    // Transfer all filter params
                    for (const auto& [k, v] : gCfg.filter.floatParams) cfg.guitarParams.floatParams[k] = v;
                    for (const auto& [k, v] : gCfg.filter.boolParams) cfg.guitarParams.boolParams[k] = v;
                    for (const auto& [k, v] : gCfg.filter.stringParams) cfg.guitarParams.stringParams[k] = v;
                    for (const auto& [k, v] : gCfg.filter.vectorParams) cfg.guitarParams.vectorParams[k] = v;
                    for (const auto& [k, v] : gCfg.filter.stringVectorParams) cfg.guitarParams.stringVectorParams[k] = v;
                }

                // Effects
                if (group.contains("fx") && group["fx"].is_array()) {
                    cfg.effects.clear();
                    for (const auto& fxItem : group["fx"]) {
                        if (fxItem.is_object()) {
                            Fx fxStruct;
                            fxStruct.from_json(fxItem);
                            cfg.effects.push_back(fxStruct);
                        } else {
                            cerr << "[TypeError] Fx item is not an object in " << configKey << ": " << fxItem << endl;
                        }
                    }
                }

                // Metadata
                if (group.contains("sound_characteristics") && group["sound_characteristics"].is_object()) {
                    cfg.soundCharacteristics.from_json(group["sound_characteristics"]);
                }
                if (group.contains("topological_metadata") && group["topological_metadata"].is_object()) {
                    cfg.topologicalMetadata.from_json(group["topological_metadata"]);
                }

                configs[configKey] = cfg;
            }
        } else {
            cerr << "[TypeError] 'groups' not found or not an object in group.json" << endl;
        }
    }

    // REMOVED: load_moods - Now using load_moods_for_scoring instead

    // REMOVED: load_synth - Now using load_synth_for_scoring instead

    void load_structure(const string& file) {
        ifstream inFile(file);
        if (!inFile) {
            cerr << "[Warn] Couldn't open " << file << endl;
            return;
        }
        json j;
        inFile >> j;
        if (!j.is_object()) {
            cerr << "[TypeError] Expected object for structure.json root, got " << j.type_name() << endl;
            return;
        }
        if (j.contains("sections") && j["sections"].is_array()) {
            for (const auto& sec : j["sections"]) {
                if (sec.is_object() && sec.contains("group") && sec["group"].is_string()) {
                    string configKey = lower(sec["group"].get<string>());
                    resolveAliases((json&)sec, configKey);
                    if (!configs.count(configKey)) {
                        cerr << "[Warn] Structure section group '" << configKey << "' not found in configs, skipping\n";
                        continue;
                    }
                    SoundConfig& cfg = configs[configKey];
                    if (sec.contains("useDynamicGate") && sec["useDynamicGate"].is_boolean()) {
                        cfg.useDynamicGate = sec["useDynamicGate"].get<bool>();
                    }
                    if (sec.contains("gateThreshold")) {
                        cfg.gateThreshold = getFlexibleFloat(sec["gateThreshold"], configKey + ".gateThreshold");
                    }
                    if (sec.contains("gateDecaySec")) {
                        cfg.gateDecaySec = getFlexibleFloat(sec["gateDecaySec"], configKey + ".gateDecaySec");
                    }
                    // Apply multipliers to ADSR
                    for (string param : {"attack", "decay", "sustain", "release"}) {
                        string mulKey = param + "Mul";
                        if (sec.contains(mulKey)) {
                            float mul = getFlexibleFloat(sec[mulKey], configKey + "." + mulKey);
                            for (auto& [context, params] : cfg.adsr) {
                                if (params.find(param) != params.end()) {
                                    params[param].min *= mul;
                                    params[param].max *= mul;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            cerr << "[TypeError] 'sections' not found or not an array in structure.json" << endl;
        }
    }

    void reportLoaded(const string& key) const {
        const SoundConfig& cfg = configs.at(key);
        cout << "Report for " << key << " (" << cfg.instrumentType << "):" << endl;
        cout << "  Loaded params (guitarParams): " << json(cfg.guitarParams.getAllParamKeys()).dump() << endl;
        cout << "  Loaded oscTypes: " << json(cfg.oscTypes).dump() << endl;
        cout << "  Loaded ADSR: " << json(cfg.adsr).dump() << endl;
        cout << "  Loaded effects count: " << cfg.effects.size() << endl;
        cout << "  Loaded emotion: " << cfg.emotion << endl;
        cout << "  Loaded topology: " << cfg.topology << endl;
        cout << "  Loaded soundCharacteristics: " << cfg.soundCharacteristics.to_json().dump() << endl;
        cout << "  Loaded topologicalMetadata: " << cfg.topologicalMetadata.to_json().dump() << endl;
        // Extend with missing/unhandled from schema comparisons (already logged in paramsFromJson)
    }

    void saveConfig(const string& filename) {
    json output = json::object();
    json guitar = json::object();
    json group = json::object();
    json sections = json::object();
    json schemaSection = json::object();

    // Collect global schema
    for (const auto& [type, schema] : BaseParamStruct::registeredSchemas) {
        json typeSchemaJson = json::object();
        for (const auto& [param, meta] : schema) {
            typeSchemaJson[param] = meta.to_json();
        }
        schemaSection[type] = typeSchemaJson;
    }
    schemaSection["version"] = BaseParamStruct::schemaVersion;

    // ONLY REAL INSTRUMENTS - NO SCORING DATA AS CONFIGS
    cout << "\n=== Processing Real Instruments Only ===\n";
    
    for (const auto& [key, cfg] : configs) {
        // Ensure this is a real, renderable instrument
        if (cfg.instrumentType.empty()) {
            cout << "[Skip] Empty instrument type: " << key << "\n";
            continue;
        }
        
        // Create renderable config with ALL tunable properties exposed
        json renderableConfig = createRenderableConfig(cfg, "default", {});
        renderableConfig["config_key"] = key;
        
        // Categorize by instrument type
        if (cfg.instrumentType.find("guitar") != string::npos || 
            cfg.instrumentType == "acoustic" || 
            cfg.instrumentType == "electric" || 
            cfg.instrumentType == "classical" || 
            cfg.instrumentType == "bass") {
            guitar[key] = renderableConfig;
            cout << "[Guitar] Added: " << key << " (" << cfg.instrumentType << ")\n";
        } else if (cfg.instrumentType == "subtractive" || 
                   cfg.instrumentType == "fm" || 
                   cfg.instrumentType == "additive" || 
                   cfg.instrumentType == "wavetable" ||
                   cfg.instrumentType == "granular" ||
                   cfg.instrumentType == "modular" ||
                   cfg.instrumentType == "hybrid_ai" ||
                   cfg.instrumentType == "physical_modeling" ||
                   cfg.instrumentType == "ensemble_chorus") {
            group[key] = renderableConfig;
            cout << "[Synth] Added: " << key << " (" << cfg.instrumentType << ")\n";
        }
    }
    
    // Create section-based groupings with ranked, renderable configs
    vector<string> sectionNames = {"intro", "verse", "chorus", "bridge", "outro"};
    for (const string& sectionName : sectionNames) {
        json sectionArray = json::array();
        
        // Create patches for each section using patcher system
        map<string, string> dummyChoices = {{"section", sectionName}};
        SectionPatch patch = createSectionPatch(sectionName, {sectionName}, dummyChoices, 10);
        
        for (const auto& layer : patch.layers) {
            json sectionConfig = layer.renderableConfig;
            sectionConfig["ai_score"] = 0.5; // Placeholder score
            sectionConfig["optimal_layer"] = layer.layerRole;
            sectionConfig["section_gain"] = layer.finalGain;
            sectionArray.push_back(sectionConfig);
        }
        
        if (!sectionArray.empty()) {
            sections[sectionName] = sectionArray;
        }
    }

    // Build final structure - ONLY REAL INSTRUMENTS
    output["guitar"] = guitar;
    output["group"] = group;
    output["sections"] = sections;
    output["schema"] = schemaSection;
    
    // Add metadata
    output["metadata"] = {
        {"version", "3.0"},
        {"generator", "Enhanced Patcher System"},
        {"description", "Only real, renderable instruments - ready for synthesis"},
        {"note", "moods.json and Synthesizer.json used for scoring/filtering only"},
        {"total_guitar_configs", guitar.size()},
        {"total_synth_configs", group.size()},
        {"total_sections", sections.size()},
        {"all_configs_renderable", true},
        {"generation_timestamp", time(nullptr)}
    };

    ofstream file(filename);
    if (file) {
        file << output.dump(4);
        file.close();
        cout << "\n✅ Enhanced configuration saved to " << filename << "\n";
        cout << "📁 Structure: " << guitar.size() << " guitar configs, " << group.size() << " synth configs, " << sections.size() << " sections\n";
        cout << "🎵 ALL configs are ready for direct synthesis!\n";
    } else {
        cerr << "Failed to save configuration to " << filename << endl;
    }
}

    // Helper function to count configs by type
    int countByType(const string& type) {
        int count = 0;
        for (const auto& [key, cfg] : configs) {
            if (cfg.instrumentType.find(type) != string::npos) count++;
        }
        return count;
    }
    
    // Scoring/filtering data structures (NOT real instruments)
    map<string, json> moodScoringData;
    map<string, json> sectionScoringData;
    
    void load_moods_for_scoring(const string& file) {
        ifstream inFile(file);
        if (!inFile) {
            cerr << "[Warn] Couldn't open " << file << endl;
            return;
        }
        json j;
        inFile >> j;
        if (!j.is_object()) {
            cerr << "[TypeError] Expected object for moods.json root" << endl;
            return;
        }
        if (j.contains("moods") && j["moods"].is_array()) {
            for (auto& mood : j["moods"]) {
                if (mood.is_object() && mood.contains("name") && mood["name"].is_string()) {
                    string name = lower(mood["name"].get<string>());
                    moodScoringData[name] = mood;
                    cout << "[Scoring] Loaded mood filter: " << name << "\n";
                }
            }
        }
    }

    void load_synth_for_scoring(const string& file) {
        ifstream inFile(file);
        if (!inFile) {
            cerr << "[Warn] Couldn't open " << file << endl;
            return;
        }
        json j;
        inFile >> j;
        if (!j.is_object()) {
            cerr << "[TypeError] Expected object for Synthesizer.json root" << endl;
            return;
        }
        if (j.contains("sections") && j["sections"].is_object()) {
            for (auto& [secName, sec] : j["sections"].items()) {
                string sectionKey = lower(secName);
                sectionScoringData[sectionKey] = sec;
                cout << "[Scoring] Loaded section filter: " << sectionKey << "\n";
            }
        }
    }
};

int main() {
    srand(time(nullptr));
    SoundEngineeringQueue queue;
    queue.loadAndMerge();
    queue.interactiveMenu();
    return 0;
}