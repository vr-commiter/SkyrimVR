#pragma once
#include "json.hpp"

using json = nlohmann::json;

struct CmdRequest {
	std::string Method;
	std::string Body;
    std::string ReqId;
};

struct CmdResponse {
	std::string Method;
	std::string Result;
    std::string ReqId;
};
/// ////////////
static void CmdRequest_to_json(nlohmann::json& j, const CmdRequest& p) {
    j = nlohmann::json{ {"Method", p.Method}, {"Body", p.Body}, {"ReqId", p.ReqId} };
}

static void CmdResponse_from_json(const nlohmann::json& j, CmdResponse& p) {
    j.at("Method").get_to(p.Method);
    j.at("Result").get_to(p.Result);
    if (j.contains("ReqId")) {
        j.at("ReqId").get_to(p.ReqId);
    }
}