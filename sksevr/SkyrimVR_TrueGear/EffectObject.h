#pragma once
#pragma once
#include "json.hpp"
#include <string>
#include <vector>

enum IntensityMode { Const, Fade, FadeInAndOut };

enum ActionType { Shake, Electrical, Stop };

static std::string get_key(const nlohmann::json j, std::string key1);
static bool contains_key(const nlohmann::json j, std::string key1);

static std::string to_string(IntensityMode pattern);
static IntensityMode to_IntensityMode(std::string s);
static std::string to_string(ActionType pattern);
static ActionType to_ActionType(std::string s);

struct TrackObject {
	ActionType action_type;
	IntensityMode intensity_mode;
	bool once;
	std::string stopName;
	int interval;
	int start_time;
	int end_time;
	int start_intensity;
	int end_intensity;
	std::vector<int> index;

	TrackObject();
	// static TrackObject Copy(const TrackObject& frame);

	TrackObject(nlohmann::json& j);
	// nlohmann::json to_json();

	nlohmann::json to_json() const {
		nlohmann::json j;
		j["action_type"] = static_cast<int>(action_type); // 转换为整数表示
		j["intensity_mode"] = static_cast<int>(intensity_mode); // 转换为整数表示
		j["once"] = once;
		j["stopName"] = stopName;
		j["interval"] = interval;
		j["start_time"] = start_time;
		j["end_time"] = end_time;
		j["start_intensity"] = start_intensity;
		j["end_intensity"] = end_intensity;
		j["index"] = index;
		return j;
	}
};

struct EffectObject {
	std::string name;
	std::string uuid;
	std::vector<TrackObject> trackList;
	bool keep;

	EffectObject();
	// static EffectObject Copy(const EffectObject& frame);

	EffectObject(nlohmann::json& j);
	// nlohmann::json to_json();

	nlohmann::json to_json() const {
		nlohmann::json j;
		j["name"] = name;
		j["uuid"] = uuid;
		j["keep"] = keep;
		// Convert each TrackObject to JSON and add to trackList
		for (const auto& track : trackList) {
			// Convert track to JSON and add to j["trackList"]
			// Assuming TrackObject has a to_json() method
			j["trackList"].push_back(track.to_json());
		}
		return j;
	}
};
