#include "EffectObject.h"

TrackObject::TrackObject() {}

TrackObject::TrackObject(nlohmann::json& j) {
    auto onceKey = get_key(j, "once");
    once = false;
    if (j.at(onceKey).get<std::string>() == "True") {
        once = true;
    }
    auto stop_nameKey = get_key(j, "stop_name");
    stopName = j.at(stop_nameKey).get<std::string>();

    auto action_typeKey = get_key(j, "action_type");
    action_type = to_ActionType(j.at(action_typeKey).get<std::string>());
    auto intensity_modeKey = get_key(j, "intensity_mode");
    intensity_mode = to_IntensityMode(j.at(intensity_modeKey).get<std::string>());

    auto intervalKey = get_key(j, "interval");
    interval = j.at(intervalKey).get<int>();
    auto start_timeKey = get_key(j, "start_time");
    start_time = j.at(start_timeKey).get<int>();
    auto end_timeKey = get_key(j, "end_time");
    end_time = j.at(end_timeKey).get<int>();

    auto start_intensityKey = get_key(j, "start_intensity");
    start_intensity = j.at(start_intensityKey).get<int>();
    auto end_intensityKey = get_key(j, "end_intensity");
    end_intensity = j.at(end_intensityKey).get<int>();

    auto indexKey = get_key(j, "index");
    auto indexListJson = j.at(indexKey).get<std::vector<int>>();

    for (auto indexObjectJson : indexListJson) {
        index.push_back(indexObjectJson);
    }
}

EffectObject::EffectObject() {}
EffectObject::EffectObject(nlohmann::json& j) {
    auto keepKey = get_key(j, "keep");
    keep = false;
    if (j.at(keepKey).get<std::string>() == "True") {
        keep = true;
    }
    auto nameKey = get_key(j, "name");
    name = j.at(nameKey).get<std::string>();
    auto uuidKey = get_key(j, "uuid");
    uuid = j.at(uuidKey).get<std::string>();

    auto pointListKey = get_key(j, "tracks");
    auto pointListJson = j.at(pointListKey).get<std::vector<nlohmann::json>>();

    for (auto dotModeObjectJson : pointListJson) {
        TrackObject obj(dotModeObjectJson);
        trackList.push_back(obj);
    }
}
/// ////////////////////
std::string to_string(IntensityMode pattern) {
    switch (pattern) {
    case Const:
        return "Const";
    case Fade:
        return "Fade";
    case FadeInAndOut:
        return "FadeInAndOut";
    }
    return std::string();
}

IntensityMode to_IntensityMode(std::string s) {
    if (s == "Const")
        return Const;
    if (s == "Fade")
        return Fade;
    if (s == "FadeInAndOut")
        return FadeInAndOut;
    return IntensityMode();
}

std::string to_string(ActionType pattern) {
    switch (pattern) {
    case Shake:
        return "Shake";
    case Electrical:
        return "Electrical";
    case Stop:
        return "Stop";
    }
    return std::string();
}

ActionType to_ActionType(std::string s) {
    if (s == "Shake")
        return Shake;
    if (s == "Electrical")
        return Electrical;
    if (s == "Stop")
        return Stop;
    return ActionType();
}

std::string get_key(const nlohmann::json j, std::string key1) {
    if (j.count(key1) > 0) {
        return key1;
    }

    key1[0] = toupper(key1[0]);

    if (j.count(key1) > 0) {
        return key1;
    }
    return key1;
}

bool contains_key(const nlohmann::json j, std::string key1) {
    if (j.count(key1) > 0) {
        return true;
    }
    key1[0] = toupper(key1[0]);

    if (j.count(key1) > 0) {
        return true;
    }
    return false;
}
