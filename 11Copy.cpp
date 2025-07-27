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

        // Load files
        load_guitar("guitar.json");
        load_group("group.json");
        load_moods("moods.json");
        load_synth("Synthesizer.json");
        load_structure("structure.json");
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
    vector<string> selectedTags;
    string section = getUserInput("1. Select Section (intro, verse, etc.): ");
    selectedTags.push_back(section);

    string mood = getUserInput("2. Select Mood (calm, nostalgic, etc.): ");
    selectedTags.push_back(mood);

    string timbre = getUserInput("3. Select Timbre (warm, bright, etc.): ");
    selectedTags.push_back(timbre);

    string instrument = getUserInput("4. Select Instrument (guitar, synth, bass, etc.): ");
    selectedTags.push_back(instrument);

    string effectGroup = getUserInput("5. Select Effect Group (reverb, distortion, etc.): ");
    selectedTags.push_back(effectGroup);

    string synthType = getUserInput("6. Select Synthesizer Type (subtractive, fm, additive, etc.): ");
    // Use in scoring

    // Narrowing with grouping
    auto groups = groupByCategory(selectedTags);
    cout << "Grouped Categories:\n" << json(groups).dump(2) << endl;

    // Suggestions (Step 3/4: scoring, selection)
    multimap<double, string, greater<double>> scoredConfigs;
    for (const auto& [key, cfg] : configs) {
        double score = computeSemanticScore(cfg, selectedTags, mood, synthType);
        scoredConfigs.insert({score, key});
    }
    cout << "AI Suggestions (highest score first):\n";
    int count = 0;
    for (const auto& [score, key] : scoredConfigs) {
        if (++count > 5) break; // Top 5
        cout << "- " << key << " (score: " << score << ")\n";
    }

    // Confirm/Save (stub; expand to generate layered config.json)
    cout << "Enter config to confirm (or 'none'): ";
    string chosen;
    getline(cin, chosen);
    if (chosen != "none") {
        // Generate layered output
        json layered = generateLayeredOutput(selectedTags, mood, synthType);
        ofstream file("layered_config.json");
        file << layered.dump(4);
        file.close();
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

    void load_moods(const string& file) {
        ifstream inFile(file);
        if (!inFile) {
            cerr << "[Warn] Couldn't open " << file << endl;
            return;
        }
        json j;
        inFile >> j;
        if (!j.is_object()) {
            cerr << "[TypeError] Expected object for moods.json root, got " << j.type_name() << endl;
            return;
        }
        if (j.contains("moods") && j["moods"].is_array()) {
            for (auto& mood : j["moods"]) {
                if (mood.is_object() && mood.contains("name") && mood["name"].is_string()) {
                    string name = lower(mood["name"].get<string>());
                    resolveAliases(mood, name);
                    if (configs.count(name)) {
                        SoundConfig& cfg = configs[name];
                        for (string param : {"attack", "decay", "sustain", "release"}) {
                            if (mood.contains(param)) {
                                cfg.adsr["mood"][param].from_json(mood[param]);
                            }
                        }
                        cfg.emotion = name;
                    }
                }
            }
        } else {
            cerr << "[TypeError] 'moods' not found or not an array in moods.json" << endl;
        }
    }

    void load_synth(const string& file) {
        ifstream inFile(file);
        if (!inFile) {
            cerr << "[Warn] Couldn't open " << file << endl;
            return;
        }
        json j;
        inFile >> j;
        if (!j.is_object()) {
            cerr << "[TypeError] Expected object for Synthesizer.json root, got " << j.type_name() << endl;
            return;
        }
        if (j.contains("sections") && j["sections"].is_object()) {
            for (auto& [secName, sec] : j["sections"].items()) {
                string configKey = lower(secName);
                resolveAliases(sec, configKey);
                SoundConfig cfg;
                cfg.instrumentType = "synth";
                if (configs.count(configKey)) {
                    cfg = configs[configKey]; // Merge with existing
                }

                // Oscillator
                if (sec.contains("oscillator")) {
                    cfg.oscTypes["osc1"] = getStringVec(sec["oscillator"], configKey + ".oscillator");
                }

                // ADSR
                if (sec.contains("adsr") && sec["adsr"].is_object()) {
                    for (string param : {"attack", "decay", "sustain", "release"}) {
                        if (sec["adsr"].contains(param)) {
                            cfg.adsr["synth"][param].from_json(sec["adsr"][param]);
                        }
                    }
                }

                // Effects
                if (sec.contains("fx") && sec["fx"].is_array()) {
                    for (const auto& fx : sec["fx"]) {
                        Fx fxStruct;
                        fxStruct.from_json(fx);
                        cfg.effects.push_back(fxStruct);
                    }
                }

                // Metadata
                if (sec.contains("emotion") && sec["emotion"].is_string()) {
                    cfg.emotion = sec["emotion"].get<string>();
                }
                if (sec.contains("topology") && sec["topology"].is_string()) {
                    cfg.topology = sec["topology"].get<string>();
                }
                if (sec.contains("sound_characteristics") && sec["sound_characteristics"].is_object()) {
                    cfg.soundCharacteristics.from_json(sec["sound_characteristics"]);
                }
                if (sec.contains("topological_metadata") && sec["topological_metadata"].is_object()) {
                    cfg.topologicalMetadata.from_json(sec["topological_metadata"]);
                }

                configs[configKey] = cfg;
            }
        } else {
            cerr << "[TypeError] 'sections' not found or not an object in Synthesizer.json" << endl;
        }
    }

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
    json synth = json::object();
    json schemaSection = json::object();

    // Collect global schema
    for (const auto& [type, schema] : BaseParamStruct::registeredSchemas) {
        // Serialize schema: convert each ParamMeta map to json
        json typeSchemaJson = json::object();
        for (const auto& [param, meta] : schema) {
            typeSchemaJson[param] = meta.to_json();
        }
        schemaSection[type] = typeSchemaJson;
    }
    schemaSection["version"] = BaseParamStruct::schemaVersion;

    // Main configs
    for (const auto& [key, cfg] : configs) {
        json cfgJson = cfg.to_json();  // No embedded schema here!
        if (cfg.instrumentType.find("guitar") != string::npos) {
            guitar[key] = cfgJson;
        } else if (cfg.instrumentType == "synth" && (key.find("pad_") != string::npos || key.find("bass_") != string::npos)) {
            group[key] = cfgJson;
        } else if (cfg.instrumentType == "synth") {
            synth[key] = cfgJson;
        } else {
            output[key] = cfgJson; // Fallback
        }
    }

    if (!guitar.empty()) output["guitar"] = guitar;
    if (!group.empty()) output["group"] = group;
    if (!synth.empty()) output["synthesizer"] = synth;
    output["schema"] = schemaSection; // Only one global schema at the root

    ofstream file(filename);
    if (file) {
        file << output.dump(4);
        file.close();
        cout << "Configuration saved to " << filename << " with grouped sections and a single top-level schema." << endl;
    } else {
        cerr << "Failed to save configuration to " << filename << endl;
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